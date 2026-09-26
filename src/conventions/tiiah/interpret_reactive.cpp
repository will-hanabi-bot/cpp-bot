#include "hanabi/conventions/tiiah/interpret_reactive.h"

#include <optional>
#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "hanabi/conventions/reactor/interpret_reactive.h"
#include "hanabi/conventions/reactor0/colour_value.h"
#include "hanabi/conventions/tiiah/buckets.h"
#include "hanabi/conventions/tiiah/superposition.h"
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
// `base` is the view it is all judged against — the stacks the giver and the
// REACTER share (§1.3), since the reacter is the seat that must act and the
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

// Keep the conditional half of a reading, so a later fact can withdraw it.
//
// Only the candidates that some world declines to support are worth recording:
// one every world agrees on is unconditional, and one the enumeration collapsed
// to a single world is not conditional on anything either.
void record_conditional(Game& game, int order,
                        const std::vector<OpenWorld>& worlds,
                        const std::vector<std::pair<Identity, std::uint64_t>>& support) {
  if (worlds.size() <= 1) return;
  const std::uint64_t all = (worlds.size() >= 64)
                                ? ~0ULL
                                : ((1ULL << worlds.size()) - 1);
  ConvData::ConditionalReading cond;
  for (const auto& [id, mask] : support) {
    if ((mask & all) == all) continue;  // every world agrees: no condition
    cond.support.emplace_back(id, mask);
  }
  if (cond.support.empty()) return;
  for (const OpenWorld& w : worlds) cond.worlds.push_back(w.assignment);
  game.with_meta(order, [&cond](ConvData& m) { m.conditional = cond; });
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

}  // namespace

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
  // The frame the giver chose the target in. A deferral resolves later, against
  // stacks that have moved, and under TIIAH they may also have moved differently
  // for different seats — so the SHARED view is what the reading binds to.
  wc.clue_play_stacks = state.common_play_stacks.empty()
                            ? state.play_stacks
                            : state.common_play_stacks;
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
  // Which seat moves first, and so which stacks everything below is judged
  // against. The receiver goes first only on the REVERSE reactive, where he
  // is the giver's Bob and the known play in his hand is what made the clue
  // reactive.
  const bool receiver_acts_first =
      receiver == state.next_player_index(action.giver);
  // WHICH SLOTS the clue names rests on what the giver and the REACTER share
  // (§1.3): the reacter is the seat that must act, and the giver is the one who
  // has to be able to predict them. The receiver needs no say -- once the
  // reacter has acted the sum rule leaves them no choice of slot, only a reading
  // of what their own card is.
  const std::vector<int> pair_view =
      state.stacks_known_to_both(action.giver, reacter);
  const State after =
      reacter_faces(game, receiver, receiver_acts_first, &pair_view);
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
      if (!connector) continue;
    }
    const IdentitySet react_live =
        hanabi::reactor::effective_possible_for(game, react_order);
    auto reacter_side_ok = [&](Identity i) {
      if (connector) return i == *connector;
      return double_chuck ? safe_to_chuck(state, after, i) : after.is_playable(i);
    };
    if (!react_live.exists(reacter_side_ok)) {
      continue;  // shared: walk on, and so does the reacter
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
      if (action.giver == state.our_player_index && !connector && !double_chuck &&
          !bucket_relation_holds(*state.variant, action.clue.kind, *actual,
                                 target.id) &&
          !both_know_their_own(game, react_order, target.order)) {
        return std::nullopt;
      }
    }

    const auto stamped =
        double_chuck
            ? reactor0::stamp_react_discard_button(game, action, react_order)
            : reactor0::stamp_react_play_button(game, action, react_order);
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
    if (own_connector) {
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
        // ...in every world the reacter's OWN hole plays leave open (§1e). A
        // seat that threw a card it could not name does not know its own
        // stacks, so "the playables of bucket 0" is a different set in each
        // world, and the honest reading is their union. Replay 2009367 T4:
        // will-bot69 threw an `{r1, y1}` at T2, so bucket 0 reads
        // `{r1, y1, r2, y2}` -- the `r2` only in the world where that card was
        // the `r1`. Exactly one world, and this is the old single-state read,
        // until somebody plays into the hole without knowing what they played.
        const auto worlds = open_worlds(game, own, reacter);
        IdentitySet allowed = IdentitySet::empty();
        std::vector<std::pair<Identity, std::uint64_t>> support;
        for (std::size_t w = 0; w < worlds.size(); ++w) {
          const IdentitySet here = IdentitySet::create([&](Identity i) {
            auto b = bucket_of(*state.variant, i.suit_index);
            return b && *b == from && worlds[w].state.is_playable(i);
          });
          allowed = allowed.union_with(here);
          for (Identity i : here) {
            auto it = std::find_if(support.begin(), support.end(),
                                   [i](const auto& p) { return p.first == i; });
            if (it == support.end()) {
              support.emplace_back(i, 1ULL << w);
            } else {
              it->second |= (1ULL << w);
            }
          }
        }
        if (game.common.thoughts[react_order]
                .possibilities()
                .intersect(allowed)
                .non_empty()) {
          // Undo the stamp helper's narrowing before applying ours, the same way
          // `reactor0::narrow_to_stamped_button` does: `stamp_react_play_button`
          // narrowed to the playables of ONE state, which is the single-world
          // reading this is here to widen, and `narrow_thought` alone could
          // never get past it. Rule 1 constrains the net effect of an
          // interpretation, not the writes inside it.
          const Thought& t0 = game.common.thoughts[react_order];
          if (t0.old_inferred) game.reset_thought_to(react_order, *t0.old_inferred);
          game.narrow_thought(react_order, allowed);
          record_conditional(game, react_order, worlds, support);
        }
      }
    }
    if (!game.waiting.empty()) {
      game.waiting.front().react_order = react_order;
      game.waiting.front().receiver_target_order = target.order;
    }
    if (game.pending_reactions[receiver]) {
      game.pending_reactions[receiver]->react_order = react_order;
      game.pending_reactions[receiver]->receiver_target_order = target.order;
    }
    return ClueInterp::REACTIVE;
  }
  return std::nullopt;
}

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
  if (target < 0) return;

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
  const auto worlds = open_worlds(game, s, wc.receiver);

  IdentitySet allowed = IdentitySet::empty();
  std::vector<std::pair<Identity, std::uint64_t>> support;
  auto offer = [&](Identity i, std::size_t w) {
    allowed = allowed.add(i);
    auto it = std::find_if(support.begin(), support.end(),
                           [i](const auto& pr) { return pr.first == i; });
    if (it == support.end()) support.emplace_back(i, 1ULL << w);
    else it->second |= (1ULL << w);
  };

  // The bucket half. Judged per world, so the playability filter is ours rather
  // than inherited from the stamp's single-world set -- which matters, because
  // the undo below drops that set.
  std::optional<int> from;
  bool one_bucket = true;
  for (Identity i : react_live) {
    auto b = bucket_of(*s.variant, i.suit_index);
    if (!b) { one_bucket = false; break; }
    if (!from) from = *b;
    else if (*from != *b) { one_bucket = false; break; }
  }
  for (std::size_t w = 0; w < worlds.size(); ++w) {
    if (one_bucket && from) {
      const int want = wc.clue.kind == ClueKind::RANK ? (*from + 1) % 3
                                                      : (*from + 2) % 3;
      const IdentitySet here = IdentitySet::create(
          [&](Identity i) {
            auto b = bucket_of(*s.variant, i.suit_index);
            return b && *b == want && worlds[w].state.is_playable(i);
          },
          static_cast<int>(s.variant->suits.size()) * 5);
      for (Identity i : here) offer(i, w);
    }
    // The finesse half: the card that follows what the reacter played. Reversed
    // suits run 5 -> 1, so the successor is `prev()` there -- `Identity::next()`
    // raw would name a card that does not exist.
    for (Identity i : react_live) {
      const auto& st = s.variant->suits[i.suit_index].suit_type;
      const auto nxt = st.reversed ? i.prev() : i.next();
      if (nxt) offer(*nxt, w);
    }
  }

  if (!allowed.non_empty()) return;
  // Never empty the card: an inference that explains nothing is worse than the
  // generic one the stamp already left (1i).
  if (game.common.thoughts[target].possibilities().intersect(allowed).is_empty()) {
    return;
  }
  // Undo the stamp helper's narrowing before applying ours, in the same spirit as
  // `reactor0::narrow_to_stamped_button` and the reacter's half above:
  // `stamp_receiver_call` narrowed to the playables of ONE frame, which is the
  // single-world reading this is here to widen, and `narrow_thought` alone could
  // never get past it. Rule 1 constrains the net effect of an interpretation, not
  // the writes inside it.
  //
  // The baseline comes from `prev` rather than from `old_inferred`: unlike
  // `target_play`, `stamp_receiver_call` writes through `narrow_thought` and so
  // leaves no `old_inferred` to roll back to. `prev` is the game before the
  // reaction was processed, which is exactly the pre-stamp inference.
  const IdentitySet& before = prev.common.thoughts[target].inferred;
  if (before.non_empty()) game.reset_thought_to(target, before);
  game.narrow_thought(target, allowed);
  record_conditional(game, target, worlds, support);
}

}  // namespace hanabi::tiiah
