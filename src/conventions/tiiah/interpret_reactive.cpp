#include "hanabi/conventions/tiiah/interpret_reactive.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "hanabi/conventions/reactor/interpret_reactive.h"
#include "hanabi/conventions/reactor0/colour_value.h"
#include "hanabi/conventions/tiiah/buckets.h"
#include "hanabi/conventions/tiiah/dupes.h"
#include "hanabi/conventions/tiiah/superposition.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
#include "hanabi/conventions/reactor0/interpret_reactive.h"
#include "hanabi/conventions/variants/hole.h"
#include "hanabi/conventions/variants/predicates.h"
#include "hanabi/conventions/variants/reversed.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::tiiah {

namespace {

// The anchor a clue carries, which is reactor0's: the rank value for a rank
// clue, and the colour's value from the fixed table for a colour one
// (CONVENTION.md §1d). The PARITY is not read — under TIIAH every reactive clue
// is even — so this takes the value alone rather than a whole assignment.
int anchor_of(const State& state, const ClueAction& action) {
  if (action.clue.kind == ClueKind::RANK) {
    return hanabi::reactor::variants::rank_reactive_value(*state.variant,
                                                          action.clue.value);
  }
  return reactor0::colour_clue_value(*state.variant, action.clue.value);
}

// Defined with §1d's receiver reading, below.
bool finesse_from_the_card(const Game& game, ClueKind kind, const IdentitySet& could,
                           const IdentitySet& react_live, int react_order);

// The stacks as they will stand once `player` has played everything they
// already know about — §1c's "stack simulation". Walks to a fixpoint so a chain
// of known plays advances in order.
//
// Reads `common`, so the giver, the reacter and the receiver all simulate the
// same thing. Identities come from the deck, which the giver and the reacter can
// see; the receiver never runs this walk for their own hand (they decode at
// reaction time), which is the same POV rule reactor0's `play_pool` follows.
State simulate_known_plays(const Game& game, int player,
                           const std::vector<int>* base) {
  return hanabi::reactor::variants::stacks_after_queued_plays(
      game, player, std::nullopt, base);
}

// THE STACKS THE REACTER WILL FACE, which is what every test below is really
// asking. Under the REVERSE reactive the receiver goes first -- he plays the
// known play that made the clue reactive at all -- so his queued plays are in.
// Under the ordinary one the reacter answers on his very next turn and nobody
// has moved, so the simulation stands down and the shared stacks are the
// answer. One sentence, both directions.
//
// `base` is the view it is all judged against — `reacter_frame`, the stacks the
// giver and the REACTER share as the minimum across their worlds (§1.3, §1e),
// since the reacter is the seat that must act and the
// giver is the one who has to be able to predict them. Passing the shared view
// (an empty `base`) is the pre-v16.12.0 behaviour and the fallback for a seat
// that is neither of the two.
State reacter_faces(const Game& game, int receiver, bool receiver_acts_first,
                    const std::vector<int>* base) {
  if (receiver_acts_first) return simulate_known_plays(game, receiver, base);
  return base ? game.state.with_stacks(*base) : game.state.shared_view();
}

// Which bucket the receiver's target must sit in, given the reacter's card and
// the clue kind (CONVENTION.md §1d): one HIGHER for a rank clue, one LOWER for
// a colour clue, wrapping. Nullopt when the reacter's suit has no bucket, which
// is what an inverted suit has.
std::optional<int> required_target_bucket(const Variant& variant, ClueKind kind,
                                          Identity reacter_id) {
  auto from = bucket_of(variant, reacter_id.suit_index);
  if (!from) return std::nullopt;
  return kind == ClueKind::RANK ? (*from + 1) % 3 : (*from + 2) % 3;
}

// Does this pairing satisfy §1d's bucket relation?
bool bucket_relation_holds(const Variant& variant, ClueKind kind,
                           Identity reacter_id, Identity target_id) {
  auto want = required_target_bucket(variant, kind, reacter_id);
  if (!want) return false;
  auto got = bucket_of(variant, target_id.suit_index);
  return got && *got == *want;
}

// What a DOUBLE CHUCK asks of the reacter (§1d). They are pressing Discard, so
// their card is safe either because the button PLAYS it — a chuck reaches the
// stack on an inverted suit — or because losing it costs the team nothing.
// `is_critical` is already false for trash, so the one test covers both halves
// of "trash, or a card whose other copy survives".
bool safe_to_chuck(const State& state, const State& after, Identity id) {
  if (state.variant->suits[id.suit_index].suit_type.inverted &&
      after.is_playable(id)) {
    return true;
  }
  return !state.is_critical(id);
}

// WHAT A REACTIVE BLIND PLAY SAYS ABOUT ITSELF: the playables of one bucket, in
// every world the reacter's OWN earlier hole plays leave open (§1d, §1e).
//
// A seat that threw a card it could not name does not know its own stacks, so "the
// playables of bucket 0" is a different set in each world and the honest reading is
// their union. Replay 2009367 T4: will-bot69 threw an `{r1, y1}` at T2, so bucket 0
// reads `{r1, y1, r2, y2}` -- the `r2` only in the world where that card was the
// `r1`. Exactly one world, and this is the old single-state read, until somebody
// plays into the hole without knowing what they played.
//
// Two readers: the giver and the reacter at clue time, and the RECEIVER once the
// reaction has resolved (`narrow_reacter_play`). `except_order` is for the latter,
// whose card has already joined the map by the time it asks.
struct BucketReading {
  IdentitySet allowed = IdentitySet::empty();
  std::vector<std::pair<Identity, std::uint64_t>> support;
  std::vector<OpenWorld> worlds;
};

BucketReading bucket_over_worlds(const Game& game, const State& base, int holder,
                                 int bucket, int except_order = -1,
                                 bool shared = false) {
  BucketReading out;
  out.worlds = open_worlds(game, base, holder, /*cap=*/64, except_order, shared);
  for (std::size_t w = 0; w < out.worlds.size(); ++w) {
    const IdentitySet here = IdentitySet::create([&](Identity i) {
      auto b = bucket_of(*game.state.variant, i.suit_index);
      return b && *b == bucket && out.worlds[w].state.is_playable(i);
    });
    out.allowed = out.allowed.union_with(here);
    for (Identity i : here) {
      auto it = std::find_if(out.support.begin(), out.support.end(),
                             [i](const auto& p) { return p.first == i; });
      if (it == out.support.end()) {
        out.support.emplace_back(i, 1ULL << w);
      } else {
        it->second |= (1ULL << w);
      }
    }
  }
  return out;
}

// "Both players would know exactly what they are playing" — the licence that
// lets Alice give a pairing which is neither a finesse nor a bucket relation
// (§1d). Judged from THEIR views: it is their own empathy that has to settle it,
// not hers.
bool both_know_their_own(const Game& game, int react_order, int target_order) {
  const auto& react_live = game.common.thoughts[react_order].possibilities();
  const auto& target_live = game.common.thoughts[target_order].possibilities();
  return react_live.length() == 1 && target_live.length() == 1;
}

// THE BUCKET RULE AS A LEGALITY LAYER (§1d, v22.4.0, the user's ruling). Would the
// receiver, having watched the reacter play `react`, still name its target? It reads
// the bucket the relation names from that card; when the bucket offers a playable
// among `target_poss` on `faced`, it names that card -- not the target. An empty
// bucket reading leaves it its own playable, and both players know what they hold.
bool receiver_bucket_empty(const State& faced, ClueKind kind, Identity react,
                           const IdentitySet& target_poss) {
  const auto want = required_target_bucket(*faced.variant, kind, react);
  if (!want) return true;
  return !target_poss.exists([&](Identity x) {
    const auto b = bucket_of(*faced.variant, x.suit_index);
    return b && *b == *want && faced.is_playable(x);
  });
}

// Can the reacter answer a direct target LEGALLY: with a playable card of the bucket
// the relation names, or with an out-of-bucket playable the receiver would not
// misread? Judged on public information (the reacter's empathy, the target's
// possibilities), so every seat that walks reaches the same answer. A pairing that
// fails it is KNOWN illegal, and the walk goes past it (replay 2022760 T22).
bool reacter_can_legally_answer(const State& faced, ClueKind kind, Identity target_id,
                                const IdentitySet& react_playables,
                                const IdentitySet& target_poss) {
  return react_playables.exists([&](Identity c) {
    return bucket_relation_holds(*faced.variant, kind, c, target_id) ||
           receiver_bucket_empty(faced, kind, c, target_poss);
  });
}

}  // namespace

// THE FRAME A REACTIVE'S TARGET IS WALKED IN (§1e, v16.24.0): the MINIMUM, suit
// by suit, across every world the reacter can live in, from the giver's
// perspective -- neither of them can name their own hole cards, so each world of
// both seats' hole plays is a set of stacks the reacter might hold, and only a
// height every one of them reaches is one the giver can count on.
//
// For the pair itself that is the pair's row, which `advance_rows_from_own_worlds`
// floors over both seats' worlds; an outside seat floors the shared view below.
// Worlds the targeting rules rule out never enter (`world_feasible`).
//
// It replaced "assume none of the superposed cards were played", which is the
// minimum only when the worlds share no height. Replay 2011397 T14: will-bot69's
// two hole cards left worlds 10131 and 10122, whose minimum 10121 already makes
// the p2 on will-bot67's slot 2 the target -- and the T6 reaction rules out the
// second world, so the frame is 10131.
std::vector<int> reacter_frame(const Game& game, int giver, int reacter,
                               int except_order) {
  const State& s = game.state;
  const std::vector<int> base = s.stacks_known_to_both(giver, reacter);
  const int me = s.our_player_index;
  if (me == giver || me == reacter) return base;  // the pair's row: already a floor
  // An outside seat falls back to the shared view, which is not floored where it
  // is kept -- a floor written back into a view and then replayed on top of can
  // make the wrong world look strike-free -- so it is floored here, on the fly,
  // over every seat's hole cards.
  std::vector<int> everyone;
  for (int p = 0; p < s.num_players; ++p) everyone.push_back(p);
  return floor_over_worlds(game, base, everyone, s.common_evidence, /*shared=*/true,
                           except_order);
}

void record_reaction(const Game& prev, Game& game, const ReactorWC& wc,
                     int react_order) {
  if (!game.state.variant->throw_it_in_a_hole) return;
  if (wc.receiver_frame.empty()) return;  // the reverse arm: nothing reconstructible
  const auto slots = hanabi::reactor::calc_target_slot(prev, game, react_order, wc);
  if (!slots) return;
  ReactionRecord r;
  r.turn = wc.turn;
  r.receiver = wc.receiver;
  r.receiver_hand = wc.receiver_hand;
  r.called = wc.receiver_called;
  r.clued = wc.receiver_clued;
  r.frame = wc.receiver_frame;
  r.target_order = wc.receiver_hand[slots->second - 1];
  game.reaction_records.push_back(std::move(r));
}

bool confirm_reverse_reactive(Game& game, int actor, int order, bool was_play) {
  if (!game.state.variant->throw_it_in_a_hole) return false;
  if (game.waiting.empty()) return false;
  const ReactorWC wc = game.waiting.front();
  if (wc.inverted || wc.receiver != actor) return false;
  // The REVERSE arm only: its receiver is the giver's Bob, and moves first.
  if (wc.receiver != game.state.next_player_index(wc.giver)) return false;
  // One of the standing plays that MADE the position, recorded before the clue
  // (v20.3.0). Asking `is_standing_play` now, after the clue, also accepted the
  // card the clue itself made playable. Replay 2018428 T17: will-bot69 played the
  // ra1 black's 1 had just touched, an unrelated card that should have withdrawn
  // the reverse reactive, and at T18 will-bot67 blind-played its o11 into a strike.
  const bool plays_a_standing_call =
      was_play && std::find(wc.receiver_standing.begin(), wc.receiver_standing.end(),
                            order) != wc.receiver_standing.end();
  if (plays_a_standing_call) return false;  // confirmed
  game.waiting.clear();
  hanabi::reactor0::retire_pending_reaction(game, wc.reacter);
  if (wc.react_order >= 0 && wc.react_order < static_cast<int>(game.meta.size())) {
    const int turn = game.state.turn_count;
    game.with_meta(wc.react_order, [turn](ConvData& m) {
      if (m.status != CardStatus::CALLED_TO_PLAY &&
          m.status != CardStatus::CALLED_TO_DISCARD) {
        return;
      }
      m.status = CardStatus::NONE;
      m.urgent = false;
      m.by = std::nullopt;
      m.react_target_order = -1;
      m.note_mark = NoteMark::RESET;
      m.note_mark_turn = turn;
    });
  }
  hanabi::logging::log_branch("tiiah.reverse_reactive_withdrawn",
                              {{"receiver", actor}, {"order", order},
                               {"was_play", was_play}, {"reacter", wc.reacter}});
  return true;
}

std::vector<ReceiverTarget> receiver_targets(const Game& game, int receiver,
                                             bool receiver_acts_first,
                                             const std::vector<int>* base) {
  const State& s = game.state;
  const State after = reacter_faces(game, receiver, receiver_acts_first, base);
  std::vector<ReceiverTarget> direct;
  std::vector<ReceiverTarget> one_away;
  std::vector<ReceiverTarget> inverted;
  for (int o : s.hands[receiver]) {
    // A card already called to play is one of the plays we just simulated, so
    // it is not waiting on anything: never retargeted (§1c).
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
    auto id = s.deck[o].id();
    if (!id) continue;  // our own hand; the receiver decodes at reaction time
    const int away = after.playable_away(*id);
    if (away != 0 && away != 1) continue;
    // An inverted card is skipped — playable or one away alike — and kept aside
    // in case it turns out to be all the receiver has (§1d).
    if (s.variant->suits[id->suit_index].suit_type.inverted) {
      inverted.push_back({o, *id, away});
      continue;
    }
    (away == 0 ? direct : one_away).push_back({o, *id, away});
  }
  // Playables before finesses, each leftmost-first — reactor0's Phase A before
  // Phase B, which is the order every seat walks.
  direct.insert(direct.end(), one_away.begin(), one_away.end());
  // EVERY TARGET GOTTEN (v18.5.0): when nothing uncalled is left to get, the walk
  // runs again as if nothing had been gotten -- as outside TIIAH -- and takes the
  // leftmost called playable, then the leftmost called finesse target. Human
  // diagnostic 2013726 T38 (v18_human_vs_bot_diagnostics/2013726.md): black's only
  // target is a g4 an earlier reactive already called, and a human gives Brown to
  // black to get blue's n5 against it. On a frame that has simulated the
  // receiver's own called plays (the reverse arm), those cards read as played and
  // drop out here by themselves.
  if (direct.empty()) {
    std::vector<ReceiverTarget> called_direct;
    std::vector<ReceiverTarget> called_one_away;
    for (int o : s.hands[receiver]) {
      if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      auto id = s.deck[o].id();
      if (!id) continue;
      if (s.variant->suits[id->suit_index].suit_type.inverted) continue;
      const int away = after.playable_away(*id);
      if (away == 0) called_direct.push_back({o, *id, away});
      if (away == 1) called_one_away.push_back({o, *id, away});
    }
    direct = std::move(called_direct);
    direct.insert(direct.end(), called_one_away.begin(), called_one_away.end());
  }
  // "Unless they are the only playables left", judged over the RECEIVER's hand:
  // then the inverted ones are all there is, and the clue is a double chuck.
  if (direct.empty()) return inverted;
  return direct;
}

std::optional<int> receiver_target(const Game& game, int receiver,
                                   bool receiver_acts_first,
                                   const std::vector<int>* base) {
  const auto targets = receiver_targets(game, receiver, receiver_acts_first, base);
  if (targets.empty()) return std::nullopt;
  return targets.front().order;
}

std::optional<ClueInterp> interpret_reactive(const Game& prev, Game& game,
                                             const ClueAction& action,
                                             int reacter, int receiver) {
  hanabi::instr::ScopedTimer st("tiiah.interpret_reactive");
  hanabi::logging::LogScope ls(
      "tiiah.interpret_reactive",
      {{"giver", action.giver}, {"reacter", reacter}, {"receiver", receiver}});
  const State& state = game.state;
  const int anchor = anchor_of(state, action);
  const int hand_size = kHandSize[state.num_players];

  // Every TIIAH reactive is EVEN parity: the two named cards are both pitched.
  ReactorWC wc{action.giver,
               reacter,
               receiver,
               state.hands[receiver],
               to_clue(action.clue, action.target),
               /*focus_slot=*/anchor,
               /*inverted=*/false,
               state.turn_count,
               /*all_plays=*/false};
  wc.even_parity = true;
  wc.rlocks = false;  // no reactive lock in this convention
  // The frame the giver chose the target in, and the one `stamp_receiver_call`
  // builds the receiver's reading in (`reactor0/interpret_reaction.cpp:400-425`).
  // A deferral resolves later, against stacks that have moved, and under TIIAH
  // they may also have moved differently for different seats — so the reading has
  // to bind to a view that does not move with our own.
  //
  // The GIVER's and the RECEIVER's, not the shared one (§1.3, v16.18.0). The
  // shared view is what all three seats know, which one seat's ignorance holds
  // back for the whole team; this reading is a claim about the receiver's card, so
  // it only has to mean one thing to the two seats it is between. Replay 2010329
  // T14 is the cost: read against the shared `[1,0,0,0,0]` the promise came out
  // `{r2,y2}`, both of them trash by then, and Rule 5 dropped the call as a stale
  // reading -- so the receiver never played the `b2` it had been handed.
  //
  // One frame, two questions: the deferral's Rule 3
  // (`reactor0/interpret_reaction.cpp:730-744`) reads the same field to ask
  // whether the REACTER's card was playable at clue time, which wants the giver's
  // and the reacter's pair instead. Recorded in TODO.md rather than fixed here.
  //
  // Except on the REVERSE reactive (v22.8.0, the user's ruling): there the
  // target is read on the frame the walk named it in, the giver's and the REACTER's.
  // Replay 2023572 T8: green's 4 to black named black's o11 as the b1, since blue, the
  // reacter, could not name the b1 it had played into the hole; on green's and
  // black's frame the b1 was down and the bots wrote `{b2}`. Read as the b1, the call
  // is stale once every 1 is down (Rule 5), and black has no standing play at T11.
  wc.clue_play_stacks = receiver == state.next_player_index(action.giver)
                            ? reacter_frame(game, action.giver, reacter)
                            : state.stacks_known_to_both(action.giver, receiver);
  // Which seat moves first, and so which stacks everything below is judged
  // against. The receiver goes first only on the REVERSE reactive, where he
  // is the giver's Bob and the known play in his hand is what made the clue
  // reactive.
  const bool receiver_acts_first =
      receiver == state.next_player_index(action.giver);
  // What the RECEIVER can reconstruct of this walk, for world feasibility (§1e,
  // v16.24.0): the shared view, which every seat computes alike, and the cards
  // the walk passes over as already called. The reverse arm's frame rests on the
  // receiver's own queued plays, which it cannot name from the deck, so it
  // records nothing.
  if (!receiver_acts_first) {
    wc.receiver_frame = state.common_play_stacks;
    for (int o : state.hands[receiver]) {
      if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) wc.receiver_called.push_back(o);
      if (o < static_cast<int>(prev.state.deck.size()) && prev.state.deck[o].clued) {
        wc.receiver_clued.push_back(o);  // known before this clue (v20.21.0)
      }
    }
  } else {
    // The reverse arm: the standing plays that made the position, read BEFORE this
    // clue, which is what its confirmation asks the receiver to play (v20.3.0).
    for (int o : prev.state.hands[receiver]) {
      if (hanabi::reactor::variants::is_standing_play(prev, o)) {
        wc.receiver_standing.push_back(o);
      }
    }
  }
  game.waiting.clear();
  game.waiting.push_back(wc);
  if (static_cast<int>(game.pending_reactions.size()) != state.num_players) {
    game.pending_reactions.assign(state.num_players, std::nullopt);
  }
  game.pending_reactions[receiver] = wc;

  // The receiver decodes positionally at reaction time, never at clue time:
  // selection reads the deck ids of their own hand, which they cannot see.
  if (receiver == state.our_player_index) return ClueInterp::REACTIVE;

  // Walk the targets in the order every seat walks them, and take the first
  // whose reacter side works. A pairing refused on SHARED knowledge is walked
  // past — the reacter walks past it too, so nobody is left behind — while one
  // refused on what only the GIVER can see kills the clue outright (§1g): the
  // reacter cannot see it and would act on that pairing regardless.
  // WHICH SLOTS the clue names rests on what the giver and the REACTER share
  // (§1.3): the reacter is the seat that must act, and the giver is the one who
  // has to be able to predict them. The receiver needs no say -- once the
  // reacter has acted the sum rule leaves them no choice of slot, only a reading
  // of what their own card is.
  const std::vector<int> pair_view = reacter_frame(game, action.giver, reacter);
  const State after =
      reacter_faces(game, receiver, receiver_acts_first, &pair_view);
  // ASCR IN THE WALK (§1e, v20.6.0, the user's ruling; it absorbs v19.3.0's world
  // fallback). The frame is a MINIMUM over the hole's worlds, so a pairing that
  // fails on it -- the reacter's card unplayable, or a one-away target with no
  // connector the reacter can be -- may work in a world some hole card leaves open.
  // Before walking on to the next target, every seat asks whether some strike-free
  // world lets this target play AND the reacter's card play; if so this is the
  // pairing, read in those worlds, and the worlds collapse to them. Only when no
  // world works does the walk go on.
  //
  // Human diagnostic 2018541 T26 (v18_human_vs_bot_diagnostics/2018541.md): the
  // first target was will-bot69's b4, paired with will-bot67's o6 `{r4,ra4}`. On the
  // frame red was 2 -- will-bot67 could not name its own T23 r3 -- so the walk went
  // on to the g2 and called o26 into a strike. In the world where o16 was the r3,
  // the r4 plays. Replay 2017491 T13 (v19.3.0) is the one-away case: the b2 had no
  // connector o12 could be, but played in the world where o18 was the b1.
  enum class AscrOutcome { NONE, READ, REJECT };
  std::optional<std::vector<OpenWorld>> ascr_all;
  std::vector<const OpenWorld*> ascr_worlds;
  // `world_one_away`: a target beyond the frame (below) is a target in every world
  // where it is at most ONE away, not only where it plays outright, and where it is
  // one away the reacter's card is its connector (v22.6.0, the user's ruling).
  auto ascr_pairing = [&](const ReceiverTarget& target, int react_order,
                          bool world_one_away = false) -> AscrOutcome {
    if (state.variant->suits[target.id.suit_index].suit_type.inverted) {
      return AscrOutcome::NONE;  // a double chuck plays nothing
    }
    if (!ascr_all) {
      std::vector<int> everyone;
      for (int p = 0; p < state.num_players; ++p) everyone.push_back(p);
      const State world_base = state.with_stacks(pair_view).with_band(
          state.evidence_known_to_both(action.giver, reacter));
      ascr_all = open_worlds(game, world_base, everyone, 64, -1, /*shared=*/true);
      ascr_worlds = strike_free(*ascr_all);
    }
    if (ascr_worlds.size() < 2) return AscrOutcome::NONE;  // one world is the frame
    // The stacks the reacter faces in each world, after the receiver's queued plays,
    // where this target plays outright.
    std::vector<std::optional<State>> faced(ascr_worlds.size());
    std::vector<std::optional<Identity>> bridge(ascr_worlds.size());
    IdentitySet bridges = IdentitySet::empty();
    for (std::size_t w = 0; w < ascr_worlds.size(); ++w) {
      State f = reacter_faces(game, receiver, receiver_acts_first,
                              &ascr_worlds[w]->state.play_stacks);
      const int away = f.playable_away(target.id);
      if (away == 0) {
        faced[w] = std::move(f);
      } else if (world_one_away && away == 1) {
        if (auto c = hanabi::reactor::variants::connector_of(f, target.id)) {
          bridge[w] = *c;
          bridges = bridges.add(*c);
        }
      }
    }
    auto index_of = [&](const OpenWorld& w) -> std::size_t {
      for (std::size_t k = 0; k < ascr_worlds.size(); ++k) {
        if (ascr_worlds[k] == &w) return k;
      }
      return ascr_worlds.size();
    };
    auto faced_in = [&](const OpenWorld& w) -> const State* {
      const std::size_t k = index_of(w);
      return k < ascr_worlds.size() && faced[k] ? &*faced[k] : nullptr;
    };
    const IdentitySet react_live =
        hanabi::reactor::effective_possible_for(game, react_order);
    // The bucket the relation names for the reacter's card (§1d) comes first.
    const auto want = bucket_of(*state.variant, target.id.suit_index);
    const std::optional<int> from =
        want ? std::optional<int>(action.clue.kind == ClueKind::RANK ? (*want + 2) % 3
                                                                     : (*want + 1) % 3)
             : std::nullopt;
    const IdentitySet in_bucket = react_live.filter([&](Identity i) {
      auto b = bucket_of(*state.variant, i.suit_index);
      return from && b && *b == *from;
    });
    // An out-of-bucket reacter card makes a LEGAL pairing in a world only if the
    // receiver, reading the bucket from it there, would find nothing and fall back to
    // its playable (v22.4.0, the user's ruling). Otherwise the world asks for an
    // illegal reactive and is no world. Replay 2022760 T22: where green was on 2 the
    // g3 played directly, but o17 could only answer as the m1, and the receiver would
    // have read the r3. At pace <= 1 a pairing may break the relation (v18.4.0).
    const IdentitySet target_poss = game.common.thoughts[target.order].possibilities();
    auto works = [&](const OpenWorld& w, Identity i) {
      const std::size_t k = index_of(w);
      if (k < ascr_worlds.size() && bridge[k]) return i == *bridge[k];
      const State* f = faced_in(w);
      if (!f || !f->is_playable(i)) return false;
      return in_bucket.contains(i) || state.pace() <= 1 ||
             receiver_bucket_empty(*f, action.clue.kind, i, target_poss);
    };
    // The connector of a one-away world is the finesse half; it stands with the
    // bucket half, as one reading (2023126: `{r2}` where red was 1, `{g2,b2}` where 2).
    const auto found = ascr_find(ascr_worlds,
                                 {in_bucket.union_with(react_live.intersect(bridges)), react_live},
                                 works, /*require_evidence=*/false);
    if (!found) return AscrOutcome::NONE;
    if (auto actual = state.deck[react_order].id()) {
      // giver-only: §1g, as in the walk. The card we can see must work somewhere --
      // and somewhere we cannot rule out by sight: a world whose hole cards we can
      // see to be otherwise is not one the team is in, and a clue resting on it
      // splits the receiver, who can see them too, from the reacter (v22.6.0;
      // self-play 6 Suits seed 176 T10: purple was on 1, Bob's r2 worked only where
      // it was on 2, and Cathy read no pairing).
      auto seen_possible = [&](const OpenWorld* w) {
        return std::all_of(w->assignment.begin(), w->assignment.end(),
                           [&](const std::pair<int, Identity>& a) {
                             const auto seen = state.deck[a.first].id();
                             return !seen || *seen == a.second;
                           });
      };
      const bool sound = std::any_of(ascr_worlds.begin(), ascr_worlds.end(),
                                     [&](const OpenWorld* w) {
                                       return seen_possible(w) && works(*w, *actual);
                                     });
      if (!sound) return AscrOutcome::REJECT;
      if (action.giver == state.our_player_index && state.pace() > 1 &&
          !bucket_relation_holds(*state.variant, action.clue.kind, *actual, target.id) &&
          !both_know_their_own(game, react_order, target.order)) {
        return AscrOutcome::REJECT;
      }
    }
    // Collapse FIRST, as the rule says: the stamp narrows the reacter's card to what
    // plays on our stacks, and until the worlds collapse the card may play in none
    // of them (2018541: the r4 on red 2). Every hole card, by the user's ruling: the
    // clue is evidence of which world we are in, and the worlds it leaves are the
    // ones we keep. Undone if the stamp still cannot be made.
    const Game before_collapse = game;
    collapse_to_worlds(game, *ascr_all, found->kept, /*shared=*/true);
    if (!reactor0::stamp_react_play_button(game, action, react_order)) {
      game = before_collapse;
      return AscrOutcome::NONE;
    }
    const Thought& t0 = game.common.thoughts[react_order];
    if (t0.old_inferred) game.reset_thought_to(react_order, *t0.old_inferred);
    game.narrow_thought(react_order, found->reading);

    // The pairing, kept for call invariant rule 0 (see the walk below).
    {
      const int paired = target.order;
      game.with_meta(react_order, [paired](ConvData& m) { m.react_target_order = paired; });
    }
    const IdentitySet react_before = prev.common.thoughts[react_order].possibilities();
    if (!game.waiting.empty()) {
      game.waiting.front().react_order = react_order;
      game.waiting.front().receiver_target_order = target.order;
      game.waiting.front().react_before = react_before;
    }
    if (game.pending_reactions[receiver]) {
      game.pending_reactions[receiver]->react_order = react_order;
      game.pending_reactions[receiver]->receiver_target_order = target.order;
      game.pending_reactions[receiver]->react_before = react_before;
    }
    hanabi::logging::log_branch("tiiah.ascr",
                                {{"site", "walk"}, {"tier", found->tier + 1},
                                 {"target", target.order}, {"react_order", react_order},
                                 {"worlds", static_cast<int>(found->kept.size())}});
    return AscrOutcome::READ;
  };
  for (const ReceiverTarget& target :
       receiver_targets(game, receiver, receiver_acts_first, &pair_view)) {
    int target_slot = 0;
    for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
      if (state.hands[receiver][i] == target.order) {
        target_slot = static_cast<int>(i) + 1;
      }
    }
    const int react_slot =
        hanabi::reactor::calc_slot(anchor, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    const int react_order = state.hands[reacter][react_slot - 1];

    // An inverted target comes back from the walk only when the receiver has
    // nothing else, and then the clue is a DOUBLE CHUCK: both players press
    // Discard, which is the button that stacks an inverted card (§1d).
    const bool double_chuck =
        state.variant->suits[target.id.suit_index].suit_type.inverted;

    // What the reacter has to be holding. A direct target wants any card that
    // plays right now; a one-away target is a FINESSE, and wants the one card
    // that bridges to it. Under a double chuck they are discarding instead, and
    // what is asked of them is that the discard be affordable.
    std::optional<Identity> connector;
    if (target.away == 1) {
      connector = hanabi::reactor::variants::connector_of(state, target.id);
      if (!connector) {
        const AscrOutcome r = ascr_pairing(target, react_order);
        if (r == AscrOutcome::READ) return ClueInterp::REACTIVE;
        if (r == AscrOutcome::REJECT) return std::nullopt;
        continue;
      }
    }
    const IdentitySet react_live =
        hanabi::reactor::effective_possible_for(game, react_order);
    auto reacter_side_ok = [&](Identity i) {
      if (connector) return i == *connector;
      return double_chuck ? safe_to_chuck(state, after, i) : after.is_playable(i);
    };
    if (!react_live.exists(reacter_side_ok)) {
      // ASCR before walking on: does some world make this pairing work?
      const AscrOutcome r = ascr_pairing(target, react_order);
      if (r == AscrOutcome::READ) return ClueInterp::REACTIVE;
      if (r == AscrOutcome::REJECT) return std::nullopt;
      continue;  // shared: walk on, and so does the reacter
    }
    // A KNOWN BUCKET VIOLATION (v22.4.0, the user's ruling): a direct target the
    // reacter can answer only with an out-of-bucket card the receiver would misread
    // is no pairing. Unlike the giver's legality test below this rests on public
    // information -- the reacter's empathy and the target's possibilities -- so every
    // walking seat walks past it alike. A world may still make it legal (ASCR).
    // Not at pace <= 1, where a pairing may break the relation (v18.4.0).
    // "Known" means GLOBALLY known: the violation must hold on the common view as
    // well as on the pair's frame. The two seats of a pair can hold different rows,
    // and a verdict that rests on one row alone steers the giver and the reacter to
    // different pairings (self-play 6 Suits, seeds 54 and 212).
    auto known_violation = [&](const State& faced) {
      return !reacter_can_legally_answer(
          faced, action.clue.kind, target.id,
          react_live.filter([&faced](Identity i) { return faced.is_playable(i); }),
          game.common.thoughts[target.order].possibilities());
    };
    if (!connector && !double_chuck && state.pace() > 1 && known_violation(after) &&
        known_violation(reacter_faces(game, receiver, receiver_acts_first,
                                      &state.common_play_stacks))) {
      const AscrOutcome r = ascr_pairing(target, react_order);
      if (r == AscrOutcome::READ) return ClueInterp::REACTIVE;
      if (r == AscrOutcome::REJECT) return std::nullopt;
      continue;
    }

    if (auto actual = state.deck[react_order].id()) {
      // giver-only: reject, never retarget. The reacter cannot see their own
      // card, so they would act on this pairing however wrong it is (§1g) —
      // including chucking a card the team still needs.
      if (!reacter_side_ok(*actual)) return std::nullopt;
      // §1d's LEGALITY test, and the GIVER's alone (v16.15.0). It is about what
      // the clue may SAY rather than which slots it names: a finesse says it by
      // itself; otherwise the bucket relation carries the identities, and failing
      // that the pairing is only legal when both players can already name their
      // own card. A double chuck is exempt — the reacter is not playing, so no
      // identity has to reach them, and an inverted suit has no bucket anyway.
      //
      // It may NOT steer the walk, and until v16.15.0 it did: a failure was a
      // `continue`, which retargeted to the next pairing. But the test reads
      // `state.deck[react_order].id()`, which is `nullopt` in the reacter's own
      // game — so the reacter never evaluated it and never retargeted, and the
      // two seats walked to different pairings. Giver-only information cannot
      // choose among slots every seat has to agree on (§1g), which is what the
      // `reacter_side_ok` line above has always said.
      //
      // Replay 2010246 T2 is the cost. will-bot67's rank 2 had two pairings: a
      // direct `r1` whose reacter slot held a `y1` (both bucket 0, so illegal),
      // and behind it a `y2` finesse. will-bot67 skipped to the finesse;
      // will-bot69, unable to see its own card, took the `r1` pairing and played
      // the wrong slot.
      //
      // So the giver simply may not give it, and every reader walks on shared
      // information alone. The bucket remains what tells a reader what their card
      // IS -- that half is below, and unchanged.
      //
      // Except in the ENDGAME, pace <= 1 (v18.4.0): there a pairing may break the
      // bucket relation whatever the players can name, and each reads it by §1d's
      // core rule -- the bucket (or the finesse, when provable) if it leaves
      // anything, else any playable. Human diagnostic 2013726 T27
      // (v18_human_vs_bot_diagnostics/2013726.md): 4 to green gets black's r4 and
      // green's `{g1,n3}`, and was illegal here, so blue gave a Red instead.
      const bool endgame = state.pace() <= 1;
      if (action.giver == state.our_player_index && !connector && !double_chuck &&
          !endgame &&
          !bucket_relation_holds(*state.variant, action.clue.kind, *actual,
                                 target.id) &&
          !both_know_their_own(game, react_order, target.order)) {
        // A globally known violation (below, the walk) is one readers understand,
        // but the giver does not give one (v22.4.0): taking the exception here cost
        // 0.3 points and 9 strikeouts per 300 games of 6 Suits in self-play.
        return std::nullopt;
      }
      // A FINESSE IS GIVEABLE ONLY WHEN THE RECEIVER CAN PROVE IT (v22.0.0, the
      // user's ruling): its target, as the clue leaves it, cannot be any card of
      // the bucket half (`finesse_from_the_card`). Otherwise the receiver reads the
      // bucket half alone (`keep_convention_half`) and would misread it. No endgame
      // exemption, since the receiver reads it the same way there. The giver's
      // alone, like the test above: a reject, never a retarget.
      if (action.giver == state.our_player_index && connector && !double_chuck) {
        const Thought& tt = game.common.thoughts[target.order];
        const IdentitySet could =
            tt.inferred.non_empty() ? tt.inferred.intersect(tt.possible) : tt.possible;
        if (!finesse_from_the_card(game, action.clue.kind, could,
                                   IdentitySet::single(*connector), react_order)) {
          return std::nullopt;
        }
      }
    }

    // The DISCHARGE read from its rule (§1k, v20.1.0): the pairing is a finesse on
    // the frame we share with the giver, so it names our card exactly -- the
    // connector -- and that card is already down on our own stacks, played into the
    // hole by the GIVER, who is still superposed for it. Then our card IS the
    // connector, and we throw it. This used to be reached only when the play stamp
    // FAILED, i.e. when our empathy happened to allow no other playable; with wide
    // empathy the stamp succeeded and the reading below re-read the card on our own
    // stacks, where the target plays outright and the bucket names something else.
    // Replay 2018316 T8: yagami_black's 5 to blue was a reverse-reactive finesse on
    // green's o2 through the i1 black itself had played unknowingly at T2 (pink 0
    // on black's frame, 1 on green's). Green read the bucket's `{b3}` and played the
    // i1 into a strike; it is the i1, and green throws it.
    bool discharged = false;
    if (connector && !double_chuck && reacter == state.our_player_index &&
        state.is_basic_trash(*connector) &&
        discharge_hole_card(game, wc, *connector)) {
      game.narrow_thought(react_order, IdentitySet::single(*connector));
      if (reactor0::stamp_react_discard_button(game, action, react_order)) {
        discharged = true;
        hanabi::logging::log_branch("tiiah.discharge_stamped", {{"order", react_order}});
      }
    }
    auto stamped =
        discharged ? std::optional<ClueInterp>(ClueInterp::REACTIVE)
        : double_chuck
            ? reactor0::stamp_react_discard_button(game, action, react_order)
            : reactor0::stamp_react_play_button(game, action, react_order);
    // The playable-dupe DISCHARGE when the pairing is not a finesse (§1k,
    // v16.25.0): our empathy leaves a single playable X on the frame we share with
    // the giver, but X is already down on our own stacks -- the giver threw it into
    // the hole without knowing -- so the play button cannot be stamped. We are
    // called to throw it instead; the receiver still plays. The finesse case is
    // read from the rule above, before any stamp.
    if (!stamped && !double_chuck && reacter == state.our_player_index) {
      const IdentitySet on_frame =
          react_live.filter([&after](Identity i) { return after.is_playable(i); });
      if (on_frame.length() == 1 && state.is_basic_trash(on_frame.head()) &&
          discharge_hole_card(game, wc, on_frame.head())) {
        game.narrow_thought(react_order, on_frame);
        stamped = reactor0::stamp_react_discard_button(game, action, react_order);
        if (stamped) {
          hanabi::logging::log_branch("tiiah.discharge_stamped", {{"order", react_order}});
        }
      }
    }
    if (!stamped) continue;

    // What the pairing tells the reacter about their own card. A finesse names
    // it outright; the bucket relation narrows it to the playable cards of the
    // right bucket; when neither applies the reading is the fallback §1d
    // describes — a superposition over every playable identity their empathy
    // still allows, which is what the stamp above already left.
    //
    // A double chuck names nothing and needs to name nothing: the call itself
    // says which button to press, and an inverted target has no bucket, so the
    // `bucket_of` test below fails for it without a case of its own.
    // WHICH slots are paired is settled above, on what the giver and the reacter
    // share. WHAT the pairing says is the REACTER's to read, on their own stacks
    // (§1.3) — they watched every play but their own, so their view is at least
    // as advanced as anything the pair shares, and the same pairing can mean two
    // different things across that gap.
    //
    // Replay 2008489 T37 is exactly that gap. will-bot69 clued with yellow on 2
    // — it had played the y3 itself and never knew — so the pair's view made the
    // receiver's `y4` ONE AWAY and the clue a finesse naming the `y3` as the
    // connector. will-bot67 had watched that y3 go down: on its own stacks the
    // y4 is playable outright, so there is no finesse and the bucket relation
    // carries the identities instead. Reading the giver's stale finesse, it
    // wrote `{y3}` on a card that was a `g3` and played it into a strike.
    const State own = reacter == state.our_player_index
                          ? reacter_faces(game, receiver, receiver_acts_first,
                                          &state.play_stacks)
                          : after;
    const int own_away = own.playable_away(target.id);
    std::optional<Identity> own_connector;
    if (own_away == 1) {
      own_connector = hanabi::reactor::variants::connector_of(state, target.id);
    }
    if (discharged) {
      // Named above: the giver's own unnamed connector, and we throw it. This
      // outranks the own-stacks reading below.
    } else if (own_connector) {
      game.narrow_thought(react_order, IdentitySet::single(*own_connector));
    } else if (own_away == 0 && !double_chuck) {
      if (auto want = bucket_of(*state.variant, target.id.suit_index)) {
        // The reacter's card is a playable one — judged AFTER the queued plays,
        // so a card that only comes live once the receiver plays what they know
        // counts — sitting in the bucket the relation names. Worked example:
        // red on 2 with the receiver holding a called r3, a rank clue, and a
        // green target gives the reacter `{r4, y1}` — the playables of bucket 0.
        const int from = action.clue.kind == ClueKind::RANK ? (*want + 2) % 3
                                                            : (*want + 1) % 3;
        // ...in every world the reacter's own hole plays leave open, which is
        // `bucket_over_worlds` above -- the same reading the RECEIVER reconstructs
        // at reaction time.
        // With the pair's BAND when the reacter is a partner: the row may already
        // carry the floor those same worlds produced (v16.24.0).
        const State world_base =
            reacter == state.our_player_index
                ? (state.play_evidence.empty() ? own : own.with_band(state.play_evidence))
                : own.with_band(state.evidence_known_to_both(action.giver, reacter));
        const auto br = bucket_over_worlds(game, world_base, reacter, from);
        const IdentitySet& allowed = br.allowed;
        const auto& worlds = br.worlds;
        const auto& support = br.support;
        // Judged against what the card could be BEFORE the stamp (v20.10.0). The
        // stamp has already narrowed it to the playables of one state -- often the
        // OTHER buckets' playables -- so testing the stamped set dropped the bucket
        // reading whenever it lay only in some world, and left the reacter reading
        // a card from the wrong bucket. Replay 2018857 T17: will-bot67's o12 was
        // stamped `{g2,b2}` (bucket 1, the target's own); its bucket-0 reading is the
        // `{r2}` of the worlds where its own o11 or o20 was the r1.
        const Thought& t_before = game.common.thoughts[react_order];
        const IdentitySet before_stamp =
            t_before.old_inferred ? t_before.old_inferred->intersect(t_before.possible)
                                  : prev.common.thoughts[react_order].possibilities();
        const IdentitySet reading = before_stamp.intersect(allowed);
        if (reading.non_empty()) {
          // Undo the stamp helper's narrowing before applying ours, the same way
          // `reactor0::narrow_to_stamped_button` does: `stamp_react_play_button`
          // narrowed to the playables of ONE state, which is the single-world
          // reading this is here to widen, and `narrow_thought` alone could
          // never get past it. Rule 1 constrains the net effect of an
          // interpretation, not the writes inside it.
          if (t_before.old_inferred) game.reset_thought_to(react_order, *t_before.old_inferred);
          game.narrow_thought(react_order, allowed);
          // ASCR (§1e): the reacter is called to play a card that plays only in
          // some worlds of its own hole cards, so the worlds collapse to those.
          // One tier, the bucket reading: the stamp's other-bucket playables are
          // what stands when it is empty, so the tiers are never mixed.
          std::vector<const OpenWorld*> world_ptrs;
          for (const OpenWorld& w : worlds) world_ptrs.push_back(&w);
          const auto found = ascr_find(
              world_ptrs, {reading},
              [](const OpenWorld& w, Identity i) { return w.state.is_playable(i); },
              /*require_evidence=*/false);
          if (found && found->kept.size() < worlds.size()) {
            collapse_to_worlds(game, worlds, found->kept, /*shared=*/true);
            hanabi::logging::log_branch(
                "tiiah.ascr", {{"site", "reacter_bucket"}, {"tier", found->tier + 1},
                               {"react_order", react_order},
                               {"worlds", static_cast<int>(found->kept.size())}});
          }
          record_conditional(game, react_order, worlds, support);
        }
      }
    }
    // The pairing, kept on the reacter's card (v20.7.0): call invariant rule 0
    // (`relegate_spent_reactions`) clears `urgent` once this target has left the
    // receiver's hand, so a deferred reaction whose target was played stops
    // outranking every clue (reactor0 DECISION_MAKING.md, Precedence step 2,
    // v9.3.0). reactor0's walk records it in `record_react_target`; this walk never
    // did, so every TIIAH reaction stayed urgent. Human diagnostic 2018759 T34
    // (v18_human_vs_bot_diagnostics/2018759.md): will-bot67 played its spent m3
    // reaction instead of saving will-bot69's playable b2 chop.
    {
      const int paired = target.order;
      game.with_meta(react_order, [paired](ConvData& m) { m.react_target_order = paired; });
    }
    const IdentitySet react_before = prev.common.thoughts[react_order].possibilities();
    if (!game.waiting.empty()) {
      game.waiting.front().react_order = react_order;
      game.waiting.front().receiver_target_order = target.order;
      game.waiting.front().react_before = react_before;
    }
    if (game.pending_reactions[receiver]) {
      game.pending_reactions[receiver]->react_order = react_order;
      game.pending_reactions[receiver]->receiver_target_order = target.order;
      game.pending_reactions[receiver]->react_before = react_before;
    }
    return ClueInterp::REACTIVE;
  }

  // ASCR BEYOND THE FRAME (v22.6.0, the user's ruling; replay 2023126 T13). The walk
  // offers only cards playable or one away on the frame, a MINIMUM over the hole's
  // worlds; by ASCR a card is a target in every world where it is at most one away.
  // With no target left on the frame, each such card, leftmost first, goes through
  // ASCR: direct worlds want a bucket playable (legality included), one-away worlds
  // the connector. 2023126: green could not name its own hole cards o6 `{r1,o2}` and
  // o8 `{r2,y2}`, so on the frame red was 0 and blue's r3 two away; wherever o6 was
  // the r1 it is at most one away, and black's Red asked for green's o18 as the r2
  // (o8 the y2) or a g2/b2 (the r2 down). The b4 is no target: no world has b2-b3.
  // A copy: an ASCR attempt that cannot stamp restores `game`, hands and all. Not on
  // the reverse arm, whose dispatch the seats may not share: a seat that finds no
  // pairing there reads the clue as stable, as the others may (self-play seed 242).
  const std::vector<int> receiver_hand =
      receiver_acts_first ? std::vector<int>{} : state.hands[receiver];
  for (int o : receiver_hand) {
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
    const auto id = state.deck[o].id();
    if (!id || state.variant->suits[id->suit_index].suit_type.inverted) continue;
    const int away = after.playable_away(*id);
    if (away == 0 || away == 1) continue;  // the frame's own, walked above
    int target_slot = 0;
    for (size_t i = 0; i < receiver_hand.size(); ++i) {
      if (receiver_hand[i] == o) target_slot = static_cast<int>(i) + 1;
    }
    const int react_slot = hanabi::reactor::calc_slot(anchor, target_slot, hand_size);
    if (react_slot < 1 || react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    const AscrOutcome r = ascr_pairing(ReceiverTarget{o, *id, away},
                                       state.hands[reacter][react_slot - 1],
                                       /*world_one_away=*/true);
    if (r != AscrOutcome::NONE) {
      hanabi::logging::log_branch("tiiah.ascr_beyond_frame",
                                  {{"target", o}, {"read", r == AscrOutcome::READ}});
    }
    if (r == AscrOutcome::READ) return ClueInterp::REACTIVE;
    if (r == AscrOutcome::REJECT) return std::nullopt;
  }

  return std::nullopt;
}

namespace {
// Defined with §1d's receiver reading, below.
bool proven_finesse(const Game& prev, const Game& game, const ReactorWC& wc,
                    Identity seen, int react_order);
}  // namespace

// WHAT THE REACTER PLAYED, read by the RECEIVER once the reaction has resolved
// (1d, v16.19.0).
//
// `interpret_reactive` narrows the reacter's card at clue time, and it writes into
// `common` -- so `note_hidden_action` later finds a singleton, advances the shared
// stacks and never stamps a superposition at all. The receiver never gets there: it
// returns before the target walk, because it cannot see its own hand to find the
// target. 1d has always said so ("in their game the reacter's card was never
// narrowed at all"), and until now nothing wrote it back, so at the receiver's seat
// every reactive blind play left a superposition as wide as the pre-clue empathy,
// for the rest of the game.
//
// Replay 2010512 is what that costs. yagami answered two rank-1 reactives by playing
// its `p1` and then its `p2`; bucket 2 is `{p}` alone, so each reading was a single
// identity and will-bot69, the giver, resolved both. will-bot67, the receiver, kept
// them as 24- and 20-candidate sets, so its shared stacks read purple 0 instead of 2
// and its row for yagami stayed `[0,0,0,1,0]`. At T13 the human's reactive yellow
// named a `y2` that is only playable once yellow is on 1: the walk found no pairing,
// read the clue as a MISTAKE, and answered a reaction it no longer believed in with
// a stable clue -- while will-bot69 went on waiting for the reaction.
//
// The receiver can do this, and only here, because it WATCHED the card. The bucket
// the reacter reasoned in is `bucket_of` the identity it saw: the reacter derived
// that bucket from the receiver's target, and the relation is what put the card in
// it, so the two agree without the receiver ever knowing its own target.
void narrow_reacter_play(const Game& prev, Game& game, const ReactorWC& wc,
                         int react_order) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return;
  if (react_order < 0 || react_order >= static_cast<int>(game.meta.size())) return;
  // Nothing to do at the giver's or the reacter's seat: the clue-time reading left
  // a singleton there, so `note_hidden_action` never built a map entry.
  if (!game.meta[react_order].superposed()) return;
  if (wc.reacter < 0 || wc.reacter >= s.num_players) return;
  if (wc.reacter == s.our_player_index) return;  // our own card; we cannot see it
  // The REACTER's card only (v20.16.0). On the reverse arm the receiver moves first,
  // and its standing play is not the reaction: read as one, it took the bucket
  // reading meant for the reacter. Self-play seed 270 T41: Bob's called o34
  // `{b1,b2}` (the b1), played as the reverse reactive's standing play, settled for
  // the team as the b2.
  if (s.holder_of(react_order) != wc.reacter) return;
  if (react_order >= static_cast<int>(prev.state.deck.size())) return;
  auto seen = prev.state.deck[react_order].id();
  if (!seen) return;
  auto from = bucket_of(*s.variant, seen->suit_index);
  if (!from) return;  // an inverted suit is in no bucket, and a double chuck says
                      // nothing about the card anyway (1d)

  // A FINESSE THE RECEIVER CAN PROVE (v17.1.0). The reacter read a finesse
  // pairing as the connector alone, so when the called card cannot be any card
  // of the bucket half in any world, the pairing was a finesse and the reacter
  // knew exactly the card we watched it play. E.g. Cathy's Red names Bob's r2
  // and Alice answers with the r1; Bob, whose red card cannot be a purple, would
  // otherwise read her card as `{r1,y2}`, so his shared view and his row for
  // Alice would stay on red 0 while the other two seats moved to red 1.
  //
  if (proven_finesse(prev, game, wc, *seen, react_order)) {
    narrow_superposition(game, react_order, IdentitySet::single(*seen));
    return;
  }

  // The frame the pairing was judged in, asked exactly as clue time asks it. From
  // this seat we are outside the giver-and-reacter pair, so it falls back to the
  // shared view -- a floor rather than the row those two hold, which can only make
  // the reading wider and the deduction weaker.
  // With its BAND, since worlds are replayed on it (v16.24.0).
  //
  // WITHOUT the card being read (v17.1.0). It is in the hole by now, so the shared
  // view floored across worlds would count it as already down. E.g. a g3
  // answering a Red would make the frame read green 3 rather than 2, the bucket
  // reading come out `{g4}`, and the card settle for the team as the g4 -- a row
  // for the giver going to green 4 with green really on 3.
  const State base =
      s.with_stacks(reacter_frame(game, wc.giver, wc.reacter, react_order))
          .with_band(s.evidence_known_to_both(wc.giver, wc.reacter));
  // Over the SHARED sets of the reacter's hole cards (v17.2.0): the giver and the
  // reacter predict this reading (`reaction_team_reading`), and must reach it
  // from the same sets.
  const auto br = bucket_over_worlds(game, base, wc.reacter, *from, react_order,
                                     /*shared=*/true);
  if (!br.allowed.non_empty()) return;
  if (!narrow_superposition(game, react_order, br.allowed)) return;
  // A finesse the receiver cannot prove may not be given (v22.0.0), so a pairing
  // that reaches here, past `proven_finesse`, is a bucket one and the bucket set is
  // what the reacter knows.
  if (game.meta[react_order].superposed()) {
    record_conditional(game, react_order, br.worlds, br.support);
  }
}

namespace {

// §1d's reading of the receiver's card: the union of the BUCKET half and the
// FINESSE half, per world, with the worlds that support each identity. Shared by
// the reader (`narrow_receiver_call`) and the giver's prediction of it
// (`annotate_candidate`), so the two cannot disagree about what a call says.
// Not yet intersected with what the card could be; each caller does that with
// its own view of the card.
struct ReceiverReading {
  IdentitySet allowed = IdentitySet::empty();
  std::vector<std::pair<Identity, std::uint64_t>> support;
  // The two halves apart, for telling a finesse from a direct pairing.
  IdentitySet bucket = IdentitySet::empty();
  IdentitySet finesse = IdentitySet::empty();
};

ReceiverReading receiver_reading(const Variant& variant,
                                 const std::vector<OpenWorld>& worlds,
                                 ClueKind kind, const IdentitySet& react_live) {
  ReceiverReading out;
  IdentitySet& allowed = out.allowed;
  auto& support = out.support;
  auto offer = [&](Identity i, std::size_t w) {
    allowed = allowed.add(i);
    auto it = std::find_if(support.begin(), support.end(),
                           [i](const auto& pr) { return pr.first == i; });
    if (it == support.end()) support.emplace_back(i, 1ULL << w);
    else it->second |= (1ULL << w);
  };

  // The bucket half. Judged per world, so the playability filter is ours rather
  // than inherited from the stamp's single-world set -- which matters, because
  // `narrow_receiver_call`'s undo drops that set.
  std::optional<int> from;
  bool one_bucket = true;
  for (Identity i : react_live) {
    auto b = bucket_of(variant, i.suit_index);
    if (!b) { one_bucket = false; break; }
    if (!from) from = *b;
    else if (*from != *b) { one_bucket = false; break; }
  }
  for (std::size_t w = 0; w < worlds.size(); ++w) {
    if (one_bucket && from) {
      const int want = kind == ClueKind::RANK ? (*from + 1) % 3 : (*from + 2) % 3;
      const IdentitySet here = IdentitySet::create(
          [&](Identity i) {
            auto b = bucket_of(variant, i.suit_index);
            return b && *b == want && worlds[w].state.is_playable(i);
          },
          static_cast<int>(variant.suits.size()) * 5);
      for (Identity i : here) {
        offer(i, w);
        out.bucket = out.bucket.add(i);
      }
    }
    // The finesse half: the card that follows what the reacter played. Reversed
    // suits run 5 -> 1, so the successor is `prev()` there -- `Identity::next()`
    // raw would name a card that does not exist.
    for (Identity i : react_live) {
      const auto& st = variant.suits[i.suit_index].suit_type;
      const auto nxt = st.reversed ? i.prev() : i.next();
      if (nxt) {
        offer(*nxt, w);
        out.finesse = out.finesse.add(*nxt);
      }
    }
  }
  return out;
}

// THE BUCKET FIRST (v22.0.0, the user's ruling): keep only the half the convention
// names -- the finesse half when the finesse is provable (`finesse_from_the_card`),
// else the bucket half alone. Until v22.0.0 the receiver read the union of the two.
// E.g. 5 to Cathy answered by Bob's g1 reads her target `{p1,t1}`, not
// `{p1,t1,g2}`; the giver may not give a finesse the receiver cannot prove (the walk).
void keep_convention_half(ReceiverReading& rr, bool proven) {
  const IdentitySet keep = proven ? rr.finesse : rr.bucket;
  rr.allowed = rr.allowed.intersect(keep);
  rr.support.erase(std::remove_if(rr.support.begin(), rr.support.end(),
                                  [&keep](const auto& pr) { return !keep.contains(pr.first); }),
                   rr.support.end());
}

// The receiver's card this reaction has just called: a CALLED_TO_PLAY that was
// not one before it. -1 when there is none.
int new_play_call(const Game& prev, const Game& game, int receiver) {
  for (int o : game.state.hands[receiver]) {
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    if (o < static_cast<int>(prev.meta.size()) &&
        prev.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      continue;  // an older call, pinned by its own clue
    }
    return o;
  }
  return -1;
}

// The worlds the receiver's reading ranges over (1e): at the receiver's own seat
// its belief over its own hole cards; elsewhere the frame the giver and the
// receiver share, over both of theirs. `narrow_receiver_call` explains why.
std::vector<OpenWorld> receiver_worlds(const Game& game, const ReactorWC& wc) {
  const State& s = game.state;
  const bool at_receiver = wc.receiver == s.our_player_index;
  // The reverse reactive reads on the giver's and the reacter's frame (v22.8.0,
  // `interpret_reactive` above), at every seat alike.
  const bool reverse = wc.receiver == s.next_player_index(wc.giver);
  const State base =
      reverse ? s.with_stacks(reacter_frame(game, wc.giver, wc.reacter))
                    .with_band(s.evidence_known_to_both(wc.giver, wc.reacter))
      : at_receiver ? s.private_base()
                    : s.with_stacks(s.stacks_known_to_both(wc.giver, wc.receiver))
                          .with_band(s.evidence_known_to_both(wc.giver, wc.receiver));
  const std::vector<int> holders =
      at_receiver ? std::vector<int>{wc.receiver}
                  : std::vector<int>{wc.receiver, wc.giver};
  return open_worlds(game, base, holders);
}

// Was this reaction a FINESSE, provably, from what every seat can see? The
// receiver's called card is the card after the one the reacter played (the
// finesse half) or a playable of the reacter's bucket one step along (the
// bucket half), and the walk takes whichever the card IS. When the card could be
// the former and cannot be any card of the latter in any world, it was the
// finesse. Judged on the card's clue-given `possible`, which every seat holds
// alike, and only for a plain play reaction (a double chuck is no pairing).
bool proven_finesse(const Game& prev, const Game& game, const ReactorWC& wc,
                    Identity seen, int react_order) {
  const State& s = game.state;
  if (wc.receiver < 0 || wc.receiver >= s.num_players) return false;
  const int target = new_play_call(prev, game, wc.receiver);
  if (target < 0 || target >= static_cast<int>(prev.common.thoughts.size())) {
    return false;
  }
  const IdentitySet& before = prev.common.thoughts[target].inferred;
  const IdentitySet& possible = game.common.thoughts[target].possible;
  const IdentitySet could = before.non_empty() ? before.intersect(possible) : possible;
  return finesse_from_the_card(game, wc.clue.kind, could, IdentitySet::single(seen),
                               react_order);
}

// The test itself, on a frame every seat computes alike (v17.2.0): the SHARED view,
// over every seat's hole cards as the team reads them. The giver and the reacter
// ask it too, to predict what the receiver will be able to name, so it cannot rest
// on anybody's own belief.
//
// The reacter's card itself is left out of the worlds (`react_order`): its identity
// is the one being asked about, and at the receiver's seat it is already in the
// hole while at the other two it is not yet.
bool finesse_from_the_card(const Game& game, ClueKind kind, const IdentitySet& could,
                           const IdentitySet& react_live, int react_order) {
  const State& s = game.state;
  std::vector<int> everyone;
  for (int p = 0; p < s.num_players; ++p) everyone.push_back(p);
  const State base = s.common_evidence.empty() ? s.shared_view()
                                               : s.shared_view().with_band(s.common_evidence);
  const auto worlds =
      open_worlds(game, base, everyone, 64, react_order, /*shared=*/true);
  const ReceiverReading rr = receiver_reading(*s.variant, worlds, kind, react_live);
  return could.intersect(rr.finesse).non_empty() &&
         could.intersect(rr.bucket).is_empty();
}

// THE RECEIVER WORLD FALLBACK (v20.5.0, the user's ruling). The reaction is in,
// but the shared stamp found nothing the receiver's target could play on the frame
// the giver and the receiver share -- a MINIMUM over the hole's worlds -- and fell
// to its bluff or mistake reading. Before that stands, the receiver's slot is read
// in the worlds, at every seat alike:
//
//   1. The bucket rule (§1d's bucket half, or its finesse half when provable): if some world
//      lets a card of that reading play, the target is it.
//   2. Otherwise any identity of the target that is ONE AWAY on the frame, bucket
//      or not, and plays in some world.
//
// Either way the worlds that make the reading are the ones kept, and the hole cards
// collapse to them: the clue is evidence of which world we are in. Only when both
// fail is the clue the bluff or mistake the stamp read.
//
// Replay 2018517 T19-T21. will-bot67 had thrown o21 `{r2,y1}` into the hole
// unnamed. yagami's 3 was a reactive: will-bot69's m3 into will-bot67's slot 1,
// the r3 (m plays into m4 or the red/yellow bucket). On the frame red was 1, so no
// 3 played, the stamp read a bluff, and will-bot67 discarded. In the world where
// o21 was the r2, the r3 plays: o21 collapses to `{r2}` and the r3 is called.
void receiver_world_fallback(const Game& prev, Game& game, const ReactorWC& wc,
                             int react_order) {
  const State& s = game.state;
  const auto slots = hanabi::reactor::calc_target_slot(prev, game, react_order, wc);
  if (!slots) return;
  const int slot = slots->second;
  if (slot < 1 || slot > static_cast<int>(wc.receiver_hand.size())) return;
  const int target = wc.receiver_hand[slot - 1];
  const auto& hand = s.hands[wc.receiver];
  if (std::find(hand.begin(), hand.end(), target) == hand.end()) return;
  if (target >= static_cast<int>(prev.common.thoughts.size())) return;

  // Only a seat that WATCHED the reacter's card reads the fallback: the reading is
  // the bucket and the finesse of that one card. The reacter itself cannot name what
  // it played, and a fallback read off its inference would collapse the worlds on a
  // guess (replay 2015109 T9: will-bot67's own `{r1,r2,y1,y2}` reaction kept half
  // the worlds). It keeps the stamp's reading.
  //
  // Unless every identity the reacter's card could be lies in ONE bucket (v22.9.0):
  // the bucket half of the reading is then the same whichever card it was, so there
  // is no guess to collapse on, and the seat reads the bucket half alone (the
  // finesse half would name a different card per identity). Replay 2023897 T5:
  // blue's own `{g1,b1}` reaction into black's o10 left blue reading no call, so at
  // T8 blue saw no standing play on black and gave a reverse reactive it took for
  // a stable clue.
  const auto played = prev.state.deck[react_order].id();
  IdentitySet react_live = IdentitySet::empty();
  bool bucket_only = false;
  if (played) {
    react_live = IdentitySet::single(*played);
  } else {
    react_live = prev.common.thoughts[react_order].possibilities();
    std::optional<int> one;
    bool single = react_live.non_empty();
    for (Identity i : react_live) {
      const auto b = bucket_of(*s.variant, i.suit_index);
      if (!b || (one && *one != *b)) {
        single = false;
        break;
      }
      one = b;
    }
    if (!single) return;
    bucket_only = true;
  }

  // The worlds the receiver's reading ranges over, exactly as `narrow_receiver_call`
  // reads them.
  const bool at_receiver = wc.receiver == s.our_player_index;
  // The reverse reactive reads on the giver's and the reacter's frame (v22.8.0,
  // `interpret_reactive` above), at every seat alike.
  const bool reverse = wc.receiver == s.next_player_index(wc.giver);
  const State base =
      reverse ? s.with_stacks(reacter_frame(game, wc.giver, wc.reacter))
                    .with_band(s.evidence_known_to_both(wc.giver, wc.reacter))
      : at_receiver ? s.private_base()
                    : s.with_stacks(s.stacks_known_to_both(wc.giver, wc.receiver))
                          .with_band(s.evidence_known_to_both(wc.giver, wc.receiver));
  const std::vector<int> holders =
      at_receiver ? std::vector<int>{wc.receiver}
                  : std::vector<int>{wc.receiver, wc.giver};
  const auto all = open_worlds(game, base, holders);
  std::vector<OpenWorld> worlds;
  for (const OpenWorld* w : strike_free(all)) worlds.push_back(*w);
  if (worlds.size() < 2) return;  // one world is the frame itself

  const IdentitySet& before = prev.common.thoughts[target].inferred;
  const IdentitySet& possible = game.common.thoughts[target].possible;
  const IdentitySet could = before.non_empty() ? before.intersect(possible) : possible;
  if (could.is_empty()) return;

  // ASCR (§1e): the bucket rule first -- §1d's bucket half, or its finesse half when provable --
  // then any one-away reading, bucket or not; whichever plays in some world. A
  // reading every world allows is no evidence about the hole, and the stamp failed
  // for some other reason -- its clue-time frame, say -- which this is not here to
  // second-guess (replay 2011885 T10).
  ReceiverReading rr = receiver_reading(*s.variant, worlds, wc.clue.kind, react_live);
  {  // the bucket first (v22.0.0, `keep_convention_half`)
    keep_convention_half(
        rr, !bucket_only &&
                finesse_from_the_card(game, wc.clue.kind, could, react_live, react_order));
  }
  std::vector<IdentitySet> tiers{could.intersect(rr.allowed)};
  if (!bucket_only) {
    tiers.push_back(could.filter([&base](Identity i) { return base.playable_away(i) == 1; }));
  }
  std::vector<const OpenWorld*> world_ptrs;
  for (const OpenWorld& w : worlds) world_ptrs.push_back(&w);
  const auto found = ascr_find(
      world_ptrs, tiers,
      [](const OpenWorld& w, Identity i) { return w.state.is_playable(i); },
      /*require_evidence=*/true);
  if (!found) return;  // the bluff or mistake stands

  if (before.non_empty()) game.reset_thought_to(target, before);
  game.narrow_thought(target, found->reading);
  const int turn = s.turn_count;
  const int giver = wc.giver;
  game.with_meta(target, [turn, giver](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.by = giver;
    m.focused = true;
    m = m.reason(turn).signal(turn);
  });
  collapse_to_worlds(game, worlds, found->kept, /*shared=*/true);
  // The conditional half, over the worlds kept (v20.8.0), as `narrow_receiver_call`
  // records it: each identity of the reading holds in its own worlds, so a later
  // fact can settle which (replay 2018766: the r2 where the receiver's hole card was
  // the r1, the y2 where it was the y1).
  {
    std::vector<OpenWorld> kept_worlds;
    for (const OpenWorld* w : found->kept) kept_worlds.push_back(*w);
    std::vector<std::pair<Identity, std::uint64_t>> support;
    for (Identity i : found->reading) {
      std::uint64_t mask = 0;
      for (std::size_t w = 0; w < kept_worlds.size() && w < 64; ++w) {
        if (kept_worlds[w].state.is_playable(i)) mask |= 1ULL << w;
      }
      if (mask != 0) support.emplace_back(i, mask);
    }
    record_conditional(game, target, kept_worlds, support);
  }
  hanabi::logging::log_branch("tiiah.ascr",
                              {{"site", "receiver"}, {"tier", found->tier + 1},
                               {"target", target},
                               {"worlds", static_cast<int>(found->kept.size())}});
}

}  // namespace

// The RECEIVER's half of 1d's relation, applied when their call is made.
//
// The reacter's side is narrowed at clue time, by `interpret_reactive` above.
// The receiver's call is not made until the reacter acts -- that is the whole
// shape of a reactive -- and it is made by reactor0's shared
// `stamp_receiver_call`, which knows nothing about buckets. So until v16.9.0
// the receiver kept the generic reading, every playable the stacks and their
// own negative information still allowed (replay 2008217 T2: {r1,y1,g1,p1}
// where the relation says {r1,y1}).
//
// What the receiver may write is the union of the two readings 1d names:
//
//   * THE BUCKET. The reacter's bucket, one step along -- up for a rank clue,
//     down for a colour one. Read off the reacter's own INFERENCE rather than
//     their card: the set is what every seat holds alike, and it is the only
//     form the reacter themselves can use, since they cannot name what they
//     played. A set spanning two buckets says nothing and is skipped.
//   * THE FINESSE. The card the reacter's own continues into, `id.next()`.
//     Taken from the identity when this seat can see it and from every
//     candidate in the reacter's inference when it cannot -- the POV rule
//     `resolve_hidden_action` already follows for a hidden play.
//
// Both are then intersected with what the card could already be, which is what
// drops the finesse branch in that replay: the reacter played a b1, so the
// continuation is b2, and a card the Blue clue did not touch cannot be blue.
// Had they played the g1, `g2` survives and the receiver writes {r1,y1,g2}.
void narrow_receiver_call(const Game& prev, Game& game, const ReactorWC& wc,
                          int react_order) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return;
  if (wc.receiver < 0 || wc.receiver >= static_cast<int>(s.hands.size())) return;
  if (react_order < 0 ||
      react_order >= static_cast<int>(prev.common.thoughts.size())) {
    return;
  }

  // The call this reaction just made. A double chuck calls the receiver to
  // DISCARD and an inverted suit is in no bucket, so only a play is read here.
  int target = -1;
  for (int o : s.hands[wc.receiver]) {
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    if (o < static_cast<int>(prev.meta.size()) &&
        prev.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      continue;  // an older call, pinned by its own clue
    }
    target = o;
    break;
  }
  if (target < 0) {
    // No call was stamped: the shared stamp read the target on the frame the giver
    // and the receiver share, found nothing playable there, and fell to its bluff
    // or mistake reading. Before that stands, the reading is tried in the worlds.
    receiver_world_fallback(prev, game, wc, react_order);
    return;
  }

  // WHAT THE REACTER'S CARD COULD BE, from this seat. Its identity when this
  // seat watched it, and the inference the clue left on it when this seat is
  // the reacter and cannot name its own card — the POV rule
  // `resolve_hidden_action` already follows for a hidden play. Both halves
  // below read this one set.
  //
  // The receiver cannot use the inference alone: at clue time they return
  // before the walk runs (they cannot see their own hand to find the target),
  // so in THEIR game the reacter's card was never narrowed at all. What they
  // have instead is better — they watched the card.
  IdentitySet react_live = IdentitySet::empty();
  if (auto played = prev.state.deck[react_order].id()) {
    react_live = IdentitySet::single(*played);
  } else {
    react_live = prev.common.thoughts[react_order].possibilities();
  }
  if (!react_live.non_empty()) return;

  // ...in every world the RECEIVER's own hole cards leave open (1e, v16.17.0).
  //
  // The reacter's half has ranged over its worlds since v16.13.0; this one did
  // not, so it read one stack vector and a candidate that is playable only in
  // some other world was never offered. Replay 2010329: will-bot69 had thrown an
  // `{r2,g1,b1}` into the hole, and its called card was an `r3` -- playable only
  // in the world where that card was the `r2`. It read `{g2}`.
  // The RECEIVER's worlds, replayed on what the receiver knows (v16.25.0). At the
  // receiver's own seat that is our belief. At any other seat it is the frame the
  // giver and the receiver share, NOT our belief: we watched the receiver's hole
  // cards land, so replaying them on our own stacks strikes the world in which
  // they are what we saw. Replay 2011475 T18: yagami's o4 `{g1,b1}` was the g1;
  // on will-bot67's belief (green 1) the g1 world struck, and the call on her o21
  // read `{r4,b1}` instead of `{g1,b1,r4}`.
  const bool at_receiver = wc.receiver == s.our_player_index;
  // The reverse reactive reads on the giver's and the reacter's frame (v22.8.0,
  // `interpret_reactive` above), at every seat alike.
  const bool reverse = wc.receiver == s.next_player_index(wc.giver);
  const State base =
      reverse ? s.with_stacks(reacter_frame(game, wc.giver, wc.reacter))
                    .with_band(s.evidence_known_to_both(wc.giver, wc.reacter))
      : at_receiver ? s.private_base()
                    : s.with_stacks(s.stacks_known_to_both(wc.giver, wc.receiver))
                          .with_band(s.evidence_known_to_both(wc.giver, wc.receiver));
  const std::vector<int> holders =
      at_receiver ? std::vector<int>{wc.receiver}
                  : std::vector<int>{wc.receiver, wc.giver};
  const auto worlds = open_worlds(game, base, holders);

  ReceiverReading rr = receiver_reading(*s.variant, worlds, wc.clue.kind, react_live);
  // The baseline comes from `prev` rather than from `old_inferred`: unlike
  // `target_play`, `stamp_receiver_call` writes through `narrow_thought` and so
  // leaves no `old_inferred` to roll back to. `prev` is the game before the
  // reaction was processed, which is exactly the pre-stamp inference.
  const IdentitySet& before = prev.common.thoughts[target].inferred;
  {  // the bucket first (v22.0.0, `keep_convention_half`)
    const IdentitySet& possible = game.common.thoughts[target].possible;
    const IdentitySet could =
        before.non_empty() ? before.intersect(possible) : possible;
    keep_convention_half(
        rr, finesse_from_the_card(game, wc.clue.kind, could, react_live, react_order));
  }
  const IdentitySet& allowed = rr.allowed;
  const auto& support = rr.support;

  if (!allowed.non_empty()) return;
  // Never empty the card: an inference that explains nothing is worse than the
  // generic one the stamp already left (1i). Judged against what the card could be
  // BEFORE the stamp (v16.26.0), not against the stamp's own set: that set is the
  // single-frame, bucket-blind reading this function exists to replace, so it is
  // routinely disjoint from the bucket reading. Replay 2011830 T16: will-bot69's
  // o15 was stamped `{r2,y3,g3}` on the pair frame, its own base named the bucket-1
  // `g4`, and the old guard kept the stamp -- which then read green as 3 in a world
  // and refused the g5 it was called to play at T23.
  {
    const IdentitySet& possible = game.common.thoughts[target].possible;
    const IdentitySet could =
        before.non_empty() ? before.intersect(possible) : possible;
    if (could.intersect(allowed).is_empty()) return;
  }
  // Undo the stamp helper's narrowing before applying ours, in the same spirit as
  // `reactor0::narrow_to_stamped_button` and the reacter's half above:
  // `stamp_receiver_call` narrowed to the playables of ONE frame, which is the
  // single-world reading this is here to widen, and `narrow_thought` alone could
  // never get past it. Rule 1 constrains the net effect of an interpretation, not
  // the writes inside it.
  if (before.non_empty()) game.reset_thought_to(target, before);
  game.narrow_thought(target, allowed);
  record_conditional(game, target, worlds, support);
}

// THE GIVER'S PREDICTION of the receiver's reading, for the tiebreak that
// prefers the reactive leaving the receiver the fewest candidates (§2,
// v16.28.0). The same §1d reading `narrow_receiver_call` will make, read as the
// giver can: the reacter's card by sight, the frame the giver shares with the
// receiver advanced by that card, the pair's worlds, and the target's
// possibilities once this clue has landed.
//
// Replay 2011885 T32: Red and Rank 1 to will-bot69 both named yagami's n2 and
// will-bot69's n3. Rank 1's bucket half added the r5, so will-bot69 would have
// read {r5,n3}; the Red did not touch the card, which ruled the r5 out, and it
// read {n3}. Tied everywhere else, the default tiebreak took the Rank 1.
void annotate_candidate(const Game& game, const Game& hypo,
                        reactor0::ClueCandidate& c) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return;
  if (c.reading.shape != reactor0::ClueShape::REACTIVE_PLAY) return;
  const int react_order = c.reading.reacter_side.order;
  const int target = c.reading.receiver_side.order;
  const int receiver = c.reading.receiver_side.holder;
  const int giver = c.action.giver;
  if (react_order < 0 || target < 0 || receiver < 0) return;
  if (react_order >= static_cast<int>(hypo.common.thoughts.size()) ||
      target >= static_cast<int>(hypo.common.thoughts.size())) {
    return;
  }
  const auto seen = s.deck[react_order].id();
  const IdentitySet react_live =
      seen ? IdentitySet::single(*seen)
           : hypo.common.thoughts[react_order].possibilities();
  if (!react_live.non_empty()) return;
  const State& hs = hypo.state;
  State base = hs.with_stacks(hs.stacks_known_to_both(giver, receiver))
                   .with_band(hs.evidence_known_to_both(giver, receiver));
  if (seen && base.is_playable(*seen)) base = base.with_play(*seen);
  const auto worlds = open_worlds(hypo, base, std::vector<int>{receiver, giver});
  ReceiverReading rr = receiver_reading(*s.variant, worlds, c.action.clue.kind,
                                        react_live);
  {  // the bucket first (v22.0.0, `keep_convention_half`)
    // What the receiver's card could be once the clue lands: its reading before the
    // clue, cut by the clue's touch (the stamp has not been read by the receiver).
    const IdentitySet& possible = hypo.common.thoughts[target].possible;
    const IdentitySet pre = target < static_cast<int>(game.common.thoughts.size())
                                ? game.common.thoughts[target].inferred
                                : IdentitySet::empty();
    const IdentitySet could =
        pre.intersect(possible).non_empty() ? pre.intersect(possible) : possible;
    keep_convention_half(rr, finesse_from_the_card(hypo, c.action.clue.kind, could,
                                                   react_live, react_order));
  }
  c.receiver_reading_size =
      rr.allowed.intersect(hypo.common.thoughts[target].possible).length();
}

std::optional<std::pair<IdentitySet, int>> reaction_team_reading(const Game& game,
                                                                 int player, int order,
                                                                 Identity id) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return std::nullopt;
  // The connection this play answers: the live one, or a deferred one.
  const ReactorWC* wc = nullptr;
  auto answers = [&](const ReactorWC& w) {
    return w.reacter == player && w.react_order == order && w.react_before.non_empty();
  };
  if (!game.waiting.empty() && answers(game.waiting.front())) wc = &game.waiting.front();
  for (const auto& pr : game.pending_reactions) {
    if (!wc && pr && answers(*pr)) wc = &*pr;
  }
  if (!wc) return std::nullopt;
  if (wc->receiver == s.our_player_index) return std::nullopt;  // it reads it itself
  const auto from = bucket_of(*s.variant, id.suit_index);
  if (!from) return std::nullopt;

  // The finesse the receiver can prove, from its called card's public reading.
  IdentitySet team = IdentitySet::empty();
  const int target = wc->receiver_target_order;
  bool finesse = false;
  if (target >= 0 && target < static_cast<int>(game.common.thoughts.size())) {
    const Thought& t = game.common.thoughts[target];
    const IdentitySet could =
        t.inferred.non_empty() ? t.inferred.intersect(t.possible) : t.possible;
    finesse = finesse_from_the_card(game, wc->clue.kind, could, IdentitySet::single(id),
                                    order);
  }
  if (finesse) {
    team = IdentitySet::single(id);
  } else {
    // Otherwise the bucket, on the frame the receiver reads it in: the shared view
    // floored over every seat's hole cards, without this one (`reacter_frame`'s
    // outside-seat branch).
    std::vector<int> everyone;
    for (int p = 0; p < s.num_players; ++p) everyone.push_back(p);
    const std::vector<int> frame = floor_over_worlds(
        game, s.common_play_stacks, everyone, s.common_evidence, /*shared=*/true, order);
    const State base = s.with_stacks(frame).with_band(s.common_evidence);
    team = bucket_over_worlds(game, base, wc->reacter, *from, order, /*shared=*/true)
               .allowed;
  }
  team = team.intersect(wc->react_before);
  if (!team.contains(id)) return std::nullopt;  // not a reading we can predict
  return std::make_pair(team, wc->receiver);
}

}  // namespace hanabi::tiiah
