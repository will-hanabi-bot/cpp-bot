#include "hanabi/conventions/tiiah/interpret_clue.h"

#include <algorithm>
#include <optional>

#include "hanabi/basics/fix.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
#include "hanabi/conventions/tiiah/interpret_reactive.h"
#include "hanabi/conventions/tiiah/superposition.h"
#include "hanabi/conventions/reactor0/interpret_reactive.h"
#include "hanabi/conventions/variants/hole.h"
#include "hanabi/conventions/variants/predicates.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::tiiah {

namespace {


// Read a stable clue on a SHARED frame for as long as this object lives
// (CONVENTION.md §1.3). The giver chose the clue from what the two of them know,
// so every seat has to decode it from the same stacks: the pair's view, the
// minimum across the worlds of their hole cards. Where those worlds DISAGREE the
// ladder's one reading is widened afterwards to the union over them
// (`read_stable_over_worlds`, §1e, v16.24.0).
//
// A swap rather than a fork of reactor0's ladders: they take the state through
// `Game`, and forking them to take a second one would be the copy §1b exists to
// avoid. Unwinding is safe because a clue writes stamps and thoughts, never
// stacks — the three fields below are all that `shared_view` moves.
class SharedStacks {
 public:
  static bool needed(const State& s, const std::vector<int>& stacks) {
    return !stacks.empty() && stacks != s.play_stacks;
  }
  SharedStacks(State& s, const std::vector<int>& stacks)
      : s_(s),
        play_stacks_(s.play_stacks),
        playable_(s.playable_set),
        trash_(s.trash_set) {
    const State swapped = s.with_stacks(stacks);
    s_.play_stacks = swapped.play_stacks;
    s_.playable_set = swapped.playable_set;
    s_.trash_set = swapped.trash_set;
  }
  ~SharedStacks() {
    s_.play_stacks = std::move(play_stacks_);
    s_.playable_set = playable_;
    s_.trash_set = trash_;
  }
  SharedStacks(const SharedStacks&) = delete;
  SharedStacks& operator=(const SharedStacks&) = delete;

 private:
  State& s_;
  std::vector<int> play_stacks_;
  IdentitySet playable_;
  IdentitySet trash_;
};

// The stacks a clue between `giver` and `holder` is read against, from OUR seat
// (CONVENTION.md §1.3).
//
// A clue only has to mean one thing to the two seats it is between, so the view
// is what THOSE two share — not `common_play_stacks`, which is what all three
// share and which one seat's ignorance holds back for everybody. Since a hidden
// play is known to every seat but its player, "what we and seat X share" is
// exactly row X, and the relation is symmetric: whether we are the giver or the
// holder, the row we want is the OTHER one's.
//
// A third party cannot compute it at all — the plays missing from the shared
// view are its own, and it cannot name them. It gets the shared view as a floor,
// and §1e's back-solve recovers the rest from the card the pair called.
// `State::stacks_known_to_both` is the implementation; this names the roles the
// convention gives the two seats.
std::vector<int> reading_stacks(const State& s, int giver, int holder) {
  return s.stacks_known_to_both(giver, holder);
}

// §1.3, the holder's half: OUR OWN called card is read against OUR OWN belief.
//
// The ladder above ran on the pairwise view, because the SLOT has to rest on
// what the giver and the receiver both know. What the card then IS, though, is
// ours to say — we watched every play but our own, so our stacks are at least
// as high as anything the pair shares, and the call means the next card on the
// stacks WE can see. Replay 2008489 T33: yagami calls will-bot69's next blue.
// The pair both know blue is on 3, and so does will-bot69, so the call is the
// b4 it is holding; read against the shared view it was a b3 that had already
// gone in, so the call read as a stall and the card went unplayed.
//
// A narrowing, never a widening: the pairwise reading is kept when our own view
// leaves the card nothing to be, since that means our belief is the thing that
// is wrong.
//
// NOR does it drop what the GIVER could have meant (v17.2.0, the user's ruling).
// Our belief can be ahead of the giver's by exactly the giver's own hole cards,
// which we watched and it cannot name; a card dead on our stacks only because of
// one of those is one the giver may well have called -- a duplicate it threw in
// the hole without knowing. The holder keeps it, so the reading is true and a
// partner who can see the card can fix the call (§1h). E.g. Bob's `{g1,b1}` was
// the g1, his Rank 1 named Cathy's other g1, and Cathy reads `{r1,y1,b1,p1}`
// rather than a dead call.
void repin_own_call(const Game& prev, Game& game, int giver) {
  const State& s = game.state;
  if (s.pairwise_play_stacks.empty()) return;
  const int me = s.our_player_index;
  if (me < 0 || me >= static_cast<int>(s.hands.size())) return;
  // Playable in some strike-free world of the giver's hole cards, on the frame the
  // giver and we share.
  IdentitySet giver_live = IdentitySet::empty();
  // What the giver MEANT: playable on the frame itself, which is the giver's own
  // belief -- it cannot know which world of its hole cards it is in.
  IdentitySet giver_meant = IdentitySet::empty();
  if (giver >= 0 && giver < s.num_players && giver != me) {
    const State base = s.with_stacks(s.stacks_known_to_both(giver, me))
                           .with_band(s.evidence_known_to_both(giver, me));
    giver_meant = base.playable_set;
    const auto worlds = open_worlds(game, base, giver);
    for (const OpenWorld* w : strike_free(worlds)) {
      giver_live = giver_live.union_with(w->state.playable_set);
    }
  }
  for (int o : s.hands[me]) {
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    if (o < static_cast<int>(prev.meta.size()) &&
        prev.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      continue;  // a standing call, already settled by the clue that made it
    }
    // A NAMED DUPE (v18.18.0). When the giver's frame leaves our call exactly one
    // identity, and we WATCHED the giver throw that very identity into the hole
    // without naming it, the call can only be the dupe: the giver meant the card
    // its own frame allows, and cannot know its hole card already played it. So
    // the card is that identity, known trash to us, and the call is withdrawn --
    // we throw it rather than hold it. A card the giver did not call so exactly
    // keeps both readings, as below. Human diagnostic 2014561 T56: "yagami_black
    // will toss it if yagami_blue already played the other copy" -- which is what
    // lets the giver call a card it may have played, when the call is named
    // (reactor0 `calls_a_card_we_may_have_played`).
    // ...except when the reading names more than that identity (v21.0.0): it was
    // widened to the giver's frame in a world of our hole cards
    // (`read_stable_over_worlds`), where the giver deduced its hole card from our
    // play. A world exists in which the card is not trash, so we keep the call.
    // Replay 2020406 T28: o26 `{p1,p3}`.
    const bool widened_past_giver =
        game.common.thoughts[o].inferred.difference(giver_meant).non_empty();
    if (giver >= 0 && giver != me && !widened_past_giver) {
      const IdentitySet meant = game.common.thoughts[o].inferred.intersect(giver_meant);
      if (meant.length() == 1) {
        const Identity x = *meant.begin();
        bool watched = false;
        for (int h = 0; h < static_cast<int>(game.meta.size()) && !watched; ++h) {
          if (!game.meta[h].superposed() && game.meta[h].shared_left.is_empty()) continue;
          if (s.holder_of(h) != giver) continue;
          const auto id = s.deck[h].id();
          watched = id && *id == x;
        }
        if (watched && s.is_basic_trash(x)) {
          const int turn = s.turn_count;
          game.with_thought(o, [x](const Thought& t) {
            Thought out = t;
            out.old_inferred = t.inferred;
            out.inferred = IdentitySet::single(x);
            return out;
          });
          game.with_meta(o, [turn](ConvData& m) { m = m.cleared().reason(turn); });
          continue;
        }
      }
    }
    // Playable in SOME world of our own hole cards (§1e, v16.24.0): our belief is
    // their minimum, and a call on a card that is only live in one of them -- the
    // b3 of 2011397 T10, in the world where our o9 was the b2 -- must survive.
    // One world is the old `playable_set` exactly.
    const IdentitySet kept = game.common.thoughts[o].inferred.intersect(
        playable_in_some_own_world(game).union_with(giver_live));
    if (kept.is_empty() || kept == game.common.thoughts[o].inferred) continue;
    game.with_thought(o, [&kept](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      out.inferred = kept;
      return out;
    });
  }
}

// THE REFUSAL (CONVENTION.md §1c, v16.14.0). Is this clue Bob telling Alice that
// the card she named is already played?
//
// Alice reads a reactive against her own stacks, and hers can be stale in exactly
// one way: she does not know what she threw in the hole. So she can name a card
// of Cathy's that the rest of the table can see has already gone down. Bob is the
// seat who must answer, and the convention gives him a way to answer "no": give
// Cathy a stable clue rather than react.
//
// Returns true when this clue is that, having applied what it teaches. The caller
// then skips the dispatch — a refusal must not be read as a fresh reactive — and
// lets the stable ladders read the clue's own content.
//
// Five conditions, and the fourth is the one that makes it a signal rather than
// an ordinary deferral. A deferral carries the reactive intent forward and is
// itself reactive; a refusal is stable. Without that test the two are the same
// event, since `Game::interpret_clue` already treats ANY clue by the reacter as
// clearing the waiting connection (`basics/decide.cpp:211-214`) — which is also
// why this reads `prev`, the connection having been cleared before we are called.
bool read_refusal(const Game& prev, Game& game, const ClueAction& action) {
  const State& state = game.state;
  if (prev.waiting.empty()) return false;
  const ReactorWC& wc = prev.waiting.front();
  if (wc.reacter != action.giver) return false;            // 1. Bob's clue
  if (action.target != wc.receiver) return false;          // 3. ...to Cathy
  if (wc.receiver_target_order < 0) return false;
  // 2. Ordinary reactives only, per the convention as ruled. Under the reverse
  // the receiver moves first and there is nothing to refuse yet.
  if (wc.receiver != state.next_player_index(state.next_player_index(wc.giver))) {
    return false;
  }
  // 5. Immediately: Bob's very next turn. A clue two turns later is a clue.
  if (state.turn_count != wc.turn + 1) return false;
  // 4. Stable, which is what distinguishes it from a deferral.
  if (reactor0::dispatch_is_reactive(game, action)) return false;

  // What Alice pointed at. She can see it, so she can name it -- and Bob has
  // just told her it is behind the stacks rather than on them.
  const auto gone = state.deck[wc.receiver_target_order].id();
  if (!gone) return false;

  collapse_refused_target(game, wc.giver, *gone);
  return true;
}

// The rainbowy suit this variant carries, if it has one: the suit a colour clue
// would otherwise leave the receiver superposed against (CONVENTION.md §1f).
// Rainbow and Omni are `rainbowish`, Muddy Rainbow and Cocoa Rainbow `muddy`,
// and Prism its own flag; 16 of the 44 TIIAH variants have exactly one.
std::optional<int> rainbowy_suit(const Variant& variant) {
  for (int s = 0; s < static_cast<int>(variant.suits.size()); ++s) {
    const SuitType& t = variant.suits[s].suit_type;
    if (t.rainbowish || t.muddy || t.prism) return s;
  }
  return std::nullopt;
}

// §1f. A colour clue's play call in a rainbowy variant means the next playable
// card of that colour's OWN suit — not a superposition of it and the rainbowy
// suit. Clue red on turn 1 and the receiver writes `r1`, full stop. When that is
// immediately impossible — the red suit finished, or dead above its stack — the
// call re-pins to the rainbowy suit's next playable instead.
//
// Run on the SHARED view, which is what carries §1f's second half: a giver
// superposed between a purple 1 and a teal 1 who clues purple is read as though
// the purple 1 were still unplayed, because a superposed play never advanced
// the shared stacks.
//
// A post-step over reactor0's ladder rather than a fork of it (§1b): the ladder
// decides WHICH card is called, and this decides what that card is.
void pin_rainbowy_colour(const Game& prev, Game& game,
                         const ClueAction& action) {
  const State& state = game.state;
  const Variant& variant = *state.variant;
  const int colour = action.clue.value;
  if (colour < 0 ||
      colour >= static_cast<int>(variant.colourable_suit_indices.size())) {
    return;
  }
  const int own = variant.colourable_suit_indices[colour];
  // "A non-orange colour clue": an inverted suit is not pinned, since a clue on
  // it names a chuck rather than a play and runs reactor0's orange ladder.
  if (variant.suits[own].suit_type.inverted) return;
  const auto rainbowy = rainbowy_suit(variant);
  if (!rainbowy) return;

  const State shared = state.shared_view();
  auto next_of = [&](int suit) -> std::optional<Identity> {
    const bool reversed = variant.suits[suit].suit_type.reversed;
    const int rank = shared.play_stacks[suit] + (reversed ? -1 : 1);
    if (rank < 1 || rank > 5) return std::nullopt;
    const Identity id(suit, rank);
    // Finished, or dead above the stack: "immediately impossible".
    if (shared.is_basic_trash(id)) return std::nullopt;
    return id;
  };
  std::optional<Identity> pin = next_of(own);
  if (!pin) pin = next_of(*rainbowy);
  if (!pin) return;

  // The card the ladder just called. Only a NEW call is pinned: an older one
  // was pinned by its own clue and §1i forbids widening it back.
  for (int o : state.hands[action.target]) {
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    if (o < static_cast<int>(prev.meta.size()) &&
        prev.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      continue;
    }
    // ...and when the ladder read the call on a frame AHEAD of the shared view (a
    // per-world rerun), its own-suit card is further up than the pin: keep the
    // own-suit identities it named and drop the rainbowy ones (v22.4.0). Narrowing
    // to the pin would empty the reading, which `narrow_thought` refuses, and the
    // rainbowy identity survived. Replay 2022760 T10: green's Green on black's o18
    // read `{g2,m1}` in the world where o7 was the g1; the m1 kept a world with
    // green on 0 alive for the team, though the call was green's.
    const IdentitySet inferred = game.common.thoughts[o].inferred;
    const IdentitySet own_ids =
        inferred.filter([own](Identity i) { return i.suit_index == own; });
    if (pin->suit_index == own && !inferred.contains(*pin) && own_ids.non_empty()) {
      game.narrow_thought(o, own_ids);
      return;
    }
    if (!game.common.thoughts[o].possibilities().contains(*pin)) return;
    game.narrow_thought(o, IdentitySet::single(*pin));
    return;
  }
}

// The special suit a rank clue's slot-1 call names (§1f, v20.20.0): the rainbowy
// suit, or else a WHITE-ISH one -- touched by no colour but still by rank, so
// White, Gray, Light Pink and Gray Pink. Null and Dark Null are white-ish too, but
// brownish: no rank touches them. No TIIAH variant carries two special suits.
std::optional<int> rank_pin_suit(const Variant& variant) {
  if (auto r = rainbowy_suit(variant)) return r;
  for (int s = 0; s < static_cast<int>(variant.suits.size()); ++s) {
    const SuitType& t = variant.suits[s].suit_type;
    if (t.whitish && !t.brownish) return s;
  }
  return std::nullopt;
}

// §1f for a RANK clue (v20.13.0, the user's ruling; white-ish suits since v20.20.0).
// A rank stable play clue that calls a NEW card in slot 1 makes it the special
// suit's playable (`rank_pin_suit`), unless that is directly impossible: the
// special suit's next card on the shared view is not of the clue's rank -- the
// clue only qualifies as a rank stable play clue when every card of that rank is
// playable or trash -- or the card cannot be it. Replay 2019249 T14: yagami's 1 to
// will-bot69 touched only its new slot-1 card, the m1, read as `{y1,g1,b1,m1}`.
void pin_special_rank(const Game& prev, Game& game, const ClueAction& action) {
  const State& state = game.state;
  const auto special = rank_pin_suit(*state.variant);
  if (!special) return;
  const auto& hand = state.hands[action.target];
  if (hand.empty()) return;
  const int slot1 = hand.front();
  if (game.meta[slot1].status != CardStatus::CALLED_TO_PLAY) return;
  if (slot1 < static_cast<int>(prev.meta.size()) &&
      prev.meta[slot1].status == CardStatus::CALLED_TO_PLAY) {
    return;  // an older call, pinned by its own clue (§1i)
  }
  if (prev.state.deck[slot1].clued) return;  // not a newly touched card
  const State shared = state.shared_view();
  const bool reversed = state.variant->suits[*special].suit_type.reversed;
  const int rank = shared.play_stacks[*special] + (reversed ? -1 : 1);
  if (rank != action.clue.value) return;
  const Identity pin(*special, rank);
  if (shared.is_basic_trash(pin)) return;
  if (!game.common.thoughts[slot1].possibilities().contains(pin)) return;
  game.narrow_thought(slot1, IdentitySet::single(pin));
}

// The stable 1 is read as the one identity it may call (v23.18.0, the user's
// rule; `stable_one_identity`): its newly called card, if the card can be it.
void pin_stable_one(const Game& prev, Game& game, const ClueAction& action) {
  const auto one = stable_one_identity(game.state);
  if (!one) return;
  for (int o : game.state.hands[action.target]) {
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    if (o < static_cast<int>(prev.meta.size()) &&
        prev.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      continue;  // an older call
    }
    if (game.common.thoughts[o].possibilities().contains(*one)) {
      game.narrow_thought(o, IdentitySet::single(*one));
    }
    return;  // the first newly called card only
  }
}

// Every identity that plays in some strike-free world of the SHARED view -- each
// seat's hole cards as the team reads them -- so every seat computes it alike. The
// flat fallback is `pitch_candidates_in_shared_worlds`'s (v20.18.0).
IdentitySet playable_in_shared_worlds(const Game& game) {
  const State& s = game.state;
  std::vector<int> everyone;
  for (int p = 0; p < s.num_players; ++p) everyone.push_back(p);
  const State base = s.common_evidence.empty()
                         ? s.shared_view()
                         : s.shared_view().with_band(s.common_evidence);
  const auto worlds = open_worlds(game, base, everyone, 64, -1, /*shared=*/true);
  IdentitySet out = IdentitySet::empty();
  for (const auto* w : strike_free(worlds)) out = out.union_with(w->state.playable_set);
  if (worlds.size() <= 1) {
    for (int p : everyone) {
      const auto own = open_worlds(game, base, std::vector<int>{p}, 64, -1, /*shared=*/true);
      for (const auto* w : strike_free(own)) out = out.union_with(w->state.playable_set);
    }
  }
  return out;
}

// THE SELF COLOUR BLUFF (v23.16.0, experimental; the user's convention). Alice,
// neither locked nor at 8 tokens, gives Bob a COLOUR clue that re-touches only
// cards already clued, none of which could play in any world, and that the ladder
// read as a plain stall -- no play reveal, no trash reveal. It calls Bob's leftmost
// card that could be the SPECIAL suit's next card to play. The special suit is the
// variant's last: its one non-plain suit, or the rightmost plain suit. Not under the
// rainbowish suits, whose colour clues touch the special suit itself. Self-play
// Dark Null seed 96 T13 (debug_dark_null_29_v23.15/darknull_9000096.json): Blue to
// sim-bob re-touches only his known b5, and calls his slot 1, the n1.
//
// Read only from what every seat shares: clue-touch empathy (`possible`) and the
// shared worlds. Built on a copy and committed only when the call stands.
bool self_colour_bluff(const Game& prev, Game& game, const ClueAction& action) {
  const State& state = game.state;
  const Variant& variant = *state.variant;
  if (!variant.throw_it_in_a_hole) return false;
  if (action.clue.kind != ClueKind::COLOUR) return false;
  if (action.target != state.next_player_index(action.giver)) return false;
  if (reactor::variants::includes_rainbowish(state)) return false;
  if (prev.state.clue_tokens == 8) return false;
  if (prev.common.obvious_locked(prev, action.giver)) return false;
  if (action.list_.empty()) return false;
  for (int o : action.list_) {
    if (o >= static_cast<int>(prev.state.deck.size()) || !prev.state.deck[o].clued) return false;
  }
  const IdentitySet live = playable_in_shared_worlds(game);
  for (int o : action.list_) {
    if (game.common.thoughts[o].possible.intersect(live).non_empty()) return false;
  }
  const int special = static_cast<int>(variant.suits.size()) - 1;
  if (special < 0) return false;
  const auto& suit_type = variant.suits[special].suit_type;
  if (suit_type.inverted) return false;
  const State shared = state.shared_view();
  const int rank = shared.play_stacks[special] + (suit_type.reversed ? -1 : 1);
  if (rank < 1 || rank > 5) return false;
  const Identity next(special, rank);
  if (shared.is_basic_trash(next)) return false;
  for (int o : state.hands[action.target]) {
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
    if (!game.common.thoughts[o].possible.contains(next)) continue;
    if (!game.common.thoughts[o].possibilities().contains(next)) continue;
    Game g = game;
    const int turn = g.state.turn_count;
    const int giver = action.giver;
    g.with_meta(o, [turn, giver](ConvData& m) {
      m.focused = true;
      m.status = CardStatus::CALLED_TO_PLAY;
      m.by = giver;
      m = m.reason(turn).signal(turn);
    });
    if (!g.narrow_thought(o, IdentitySet::single(next))) return false;
    if (g.meta[o].status != CardStatus::CALLED_TO_PLAY) return false;
    game = std::move(g);
    hanabi::logging::log_branch("tiiah.self_colour_bluff",
                                {{"order", o}, {"suit", next.suit_index}, {"rank", next.rank}});
    return true;
  }
  return false;
}

}  // namespace

std::optional<ClueInterp> interpret_clue(const Game& prev, Game& game,
                                         const ClueAction& action) {
  hanabi::instr::ScopedTimer st("tiiah.interpret_clue");
  hanabi::logging::LogScope ls(
      "tiiah.interpret_clue",
      {{"giver", action.giver}, {"target", action.target}});
  const State& state = game.state;

  // The `emptyClues` table option lets a clue touching nothing be given, and
  // such a clue says nothing. No TIIAH variant is a BLIND family — every one of
  // the 44 carries `throwItInAHole` and no other behavioural flag — so there is
  // no blind arm to write here, unlike reactor0's dispatcher.
  if (state.options.empty_clues && action.list_.empty()) {
    return ClueInterp::USELESS;
  }

  const int bob = state.next_player_index(action.giver);
  const int cathy = state.next_player_index(bob);

  // THE DISPATCH (CONVENTION.md §1c). TIIAH has BOTH: reactor0's positional one
  // and the reverse, and the POSITION decides which seat's clue carries the
  // reaction.
  //
  //   position | clue to Bob                       | clue to Cathy
  //   ---------|----------------------------------|--------------------------
  //   holds    | REACTIVE, Cathy reacts, Bob gets | stable
  //   else     | stable                           | REACTIVE, Bob reacts
  //
  // The reverse arm works because Bob plays what he already knows, Cathy
  // answers, and Bob's target is waiting for him when he comes round again. The
  // ordinary arm is reactor0's, unchanged in shape — and it was missing until
  // v16.8.0, so every clue to Cathy fell through to the stable ladders and a
  // double play read as a lock (replay 2008177 T3).
  //
  // Asked of PREV, the position before the clue — as reactor0 asks
  // `clue_is_reactive`. Asking the post-clue game instead makes every play clue
  // to Bob answer "Bob has a known play", because the clue itself just gave him
  // one, and the dispatch would eat its own tail.
  // THE REFUSAL (CONVENTION.md §1c), ahead of the dispatch because it has to
  // pre-empt it: Bob's clue to Cathy is `giver=bob, target=cathy`, which the
  // ordinary arm below would read as a fresh reactive.
  //
  // Alice named a card of Cathy's that Bob can see is already played, so Bob
  // says so the only way the convention leaves him — by giving Cathy a stable
  // clue instead of reacting. The signal is the ENVELOPE, so the clue still gets
  // read as whatever stable clue it is; what the refusal adds is that Alice now
  // knows the card she pointed at has gone, and therefore what she threw in the
  // hole (§1e rule 5).
  // A refusal is stable by construction, and falls through to the ladders below
  // — the signal rides along with whatever the clue otherwise says.
  const bool refusal = read_refusal(prev, game, action);

  // THE DEFERRED READ (v20.12.0, the user's ruling). A clue to Bob is a reverse
  // reactive with US reacting if Bob holds a standing play. When Bob still owes US
  // a reaction, the card he will answer with depends on our own target, which we
  // cannot see -- so whether Bob holds a standing play, and what it is, is not ours
  // to know yet. We wait. Bob's next play or discard settles his reaction to us,
  // and `Game::handle_action` then rewinds to this clue with that card counted as
  // the standing call (`Game::deferred_reads`).
  //
  // Replay 2019249 T4: will-bot67's Yellow to yagami was a reverse reactive pairing
  // will-bot69's o15 with yagami's o9 -- the r2, leftmost playable once yagami's
  // owed T1 reaction, the r1, has played. will-bot69 was that reaction's receiver
  // and could not name the r1, so it read the Yellow as a stable MISTAKE.
  const int clue_turn = game.state.turn_count;
  const Game::DeferredRead* deferred = nullptr;
  for (const Game::DeferredRead& d : game.deferred_reads) {
    if (d.turn == clue_turn) deferred = &d;
  }
  std::optional<Game> forced_prev;
  if (deferred && deferred->standing >= 0) {
    const int standing = deferred->standing;
    auto stamp = [standing](Game& g) {
      g.with_meta(standing, [](ConvData& m) {
        m.status = CardStatus::CALLED_TO_PLAY;
        m.urgent = false;
      });
    };
    forced_prev = prev;
    stamp(*forced_prev);
    stamp(game);
  }
  const Game& pos = forced_prev ? *forced_prev : prev;
  // ...and when Bob settled it by DISCARDING, the clue was the fix of his reaction
  // card, not a reverse reactive (the user's ruling): a fixed reaction card is
  // thrown at once (`dead_call_fix` above stamps it). His reaction to us is void --
  // the card it named was dead -- and the identity he threw was already played, by
  // a hole card that admits it. Self-play seed 259 T4: Alice's Green fixed Bob's
  // o7, a dead b1; Cathy, who had played her o10 into the hole, learns it was the b1.
  if (deferred && deferred->standing == -1 && deferred->thrown >= 0) {
    const auto thrown_id = prev.state.deck[deferred->thrown].id();
    if (thrown_id && hanabi::tiiah::team_learns_a_hole_card_was(game, *thrown_id)) {
      hanabi::reactor0::retire_pending_reaction(game, bob);
      hanabi::logging::log_branch("tiiah.deferred_read_fix",
                                  {{"turn", clue_turn}, {"thrown", deferred->thrown}});
      return ClueInterp::FIX;
    }
  }
  if (!refusal && !deferred && action.target == bob && cathy != action.giver &&
      cathy == state.our_player_index) {
    const auto& owed = prev.pending_reactions;
    const int us = state.our_player_index;
    const bool bob_owes_us = us < static_cast<int>(owed.size()) && owed[us] &&
                             owed[us]->reacter == bob && owed[us]->receiver == us;
    if (bob_owes_us && !hanabi::reactor::variants::has_standing_play(prev, bob) &&
        !hanabi::reactor::variants::has_standing_play(prev, us)) {
      game.deferred_reads.push_back(Game::DeferredRead{clue_turn, bob, -2});
      hanabi::logging::log_branch("tiiah.deferred_read",
                                  {{"turn", clue_turn}, {"giver", action.giver},
                                   {"receiver", bob}});
      return ClueInterp::REACTIVE;
    }
  }
  if (!refusal) {
    const bool reversed =
        hanabi::reactor::variants::reverse_reactive_position(pos, action.giver);
    if (reversed) {
      // A clue to Bob that FIXES his dead standing call is the fix below, not a
      // reverse reactive (v18.10.0): the call that puts the table in the reverse
      // position is the very one the fix is for. Replay 2013726 T5.
      // ...nor once the deck is NEARLY OUT (v22.7.0, the user's ruling): fewer
      // cards left than seats, and a clue to Bob is a stable stall, read with the
      // stall context below. Replay 2023552 T53 (one card left): blue held a known
      // b5 and black nothing, so every clue to blue read as a reverse reactive with
      // no pairing -- a MISTAKE -- and green could not give the stall that showed
      // blue its g5. Not on the shared pace: human diagnostic 2013726 T30 had it
      // low with seven cards left, and its Brown is a reverse reactive.
      if (action.target == bob && !hanabi::clue_would_fix_dead_call(pos, action) &&
          !hanabi::reactor::variants::reverse_reactive_off_late(pos)) {
        return interpret_reactive(pos, game, action, /*reacter=*/cathy,
                                  /*receiver=*/bob);
      }
      // ...and a clue to Cathy is stable, which is the whole point of the flip.
    } else if (cathy != action.giver && action.target != bob &&
               !hanabi::reactor::variants::inverted_stable(pos, action.giver,
                                                           action.target)) {
      // ROLE INVERSION (v18.2.0): a clue to Cathy is also stable when Bob holds a
      // STANDING play -- any called card, not only a touch-known one -- and
      // Cathy does not. That is the only thing it changes: a clue to Bob stays
      // stable in that position too. Replay 2013645 T11.
      return interpret_reactive(pos, game, action, /*reacter=*/bob,
                                /*receiver=*/cathy);
    }
  }

  // THE FIX CLUE (CONVENTION.md §1h, v16.20.0), ahead of the stable ladders and
  // instead of them.
  //
  // A standing call on a card the giver can see is dead, and this clue has just
  // narrowed it to exactly that dead identity — from the positive touch or from the
  // negative, `Game::on_clue` does not care which. Nothing more needs doing to the
  // card: the narrowing has already happened, and `drop_dead_play_calls` withdraws
  // the call at every seat on the next `enforce_call_invariants`, because an
  // all-trash inference is all-trash in every seat's belief once the SHARED view
  // says so. What is left is the one decision only this can make — that the clue
  // means the fix and NOT what the ladders would have said.
  //
  // It supersedes rather than riding along, unlike §1c's refusal. That costs a
  // negative-touch fix whatever the ladder would have read into the cards it did
  // touch, which is the ruling and is written up in §1h.
  //
  // After the dispatch, so only a stable clue can be one — the same discriminator
  // the refusal uses.
  if (auto fixed = dead_call_fix(prev, game, action.giver, action.target)) {
    // The fix WITHDRAWS the call itself (v17.2.0). Rule 3 of the call invariants
    // can no longer be left to do it: a call live in some shared world is not
    // dead there (§1c, v16.29.0), and the dead identity of a duplicate is live in
    // the world where the giver's hole card was something else.
    const int turn = game.state.turn_count;
    // ...but a REACTION call that is fixed becomes a discard, to be pressed at once
    // (v20.12.0, the user's ruling). The reacter answers with the fixed card's
    // Discard, which tells the seat waiting on that reaction -- its receiver, who
    // cannot name the card -- that this clue was the fix and not a reverse
    // reactive (`Game::deferred_reads`). Self-play seed 259: Bob owed Cathy a
    // reaction on o7 and deferred; Cathy then played a b1, Alice's Green fixed o7
    // as the dead b1, and Bob, his call withdrawn, played an unrelated r1 that
    // Cathy took for his standing call.
    const bool reaction_call = game.meta[*fixed].react_target_order >= 0;
    game.with_meta(*fixed, [turn, reaction_call](ConvData& m) {
      m = m.cleared().reason(turn);
      m.note_mark = NoteMark::RESET;
      m.note_mark_turn = turn;
      if (reaction_call) {
        m.status = CardStatus::CALLED_TO_DISCARD;
        m.urgent = true;
        m.signal_turn = turn;
        m.fixed_reaction = true;
      }
    });
    game.disarm_reaction_elim(*fixed);
    repin_own_call(prev, game, action.giver);
    return ClueInterp::FIX;
  }

  // STABLE (CONVENTION.md §1b) — reactor0's ladders, unchanged. They are
  // exported precisely so a sibling convention can share them rather than fork
  // a copy that drifts.
  // `shared_in_endgame`, not `in_endgame`: the stacks are a belief here, and a
  // clue that read as a stall at one seat and a play call at another is the
  // whole failure this variant invites (CONVENTION.md 1.1).
  const bool stall_ctx = prev.common.obvious_locked(prev, action.giver) ||
                         game.shared_in_endgame() ||
                         prev.state.clue_tokens == 8;
  // Both ladders read the stacks the giver and the receiver share (§1.3) — the
  // receiver is the one who must act, so the slot has to rest on what the two of
  // them can both compute. Nothing is copied while that view already agrees with
  // our belief, which is every game until somebody plays into the hole without
  // knowing what they played.
  const std::vector<int> view = reading_stacks(state, action.giver, action.target);

  // One run of the ladder on `frame`. Scoped so the swap is RELEASED before the
  // re-pin below, which has to see our own belief rather than the pair's view.
  auto run_ladder = [&](Game& g, const std::vector<int>& frame) {
    const bool swap = SharedStacks::needed(g.state, frame);
    std::optional<Game> prev_shared;
    if (swap) {
      prev_shared = prev;
      prev_shared->state = prev.state.with_stacks(frame);
    }
    const Game& p = swap ? *prev_shared : prev;
    std::optional<SharedStacks> scope;
    if (swap) scope.emplace(g.state, frame);
    std::optional<ClueInterp> out;
    if (action.clue.kind != ClueKind::COLOUR) {
      // A pinkish re-touch (pink tempo / trash / identity) rests on the stacks
      // EVERY seat knows (v19.0.0).
      out = reactor0::stable_rank(p, g, action, stall_ctx,
                                  &state.common_play_stacks);
      // §1f for a rank clue: a new slot-1 call is the special suit's playable.
      // ...except a 1, which names the stable 1's identity (v23.18.0).
      if (action.clue.value == 1) {
        pin_stable_one(p, g, action);
      } else {
        pin_special_rank(p, g, action);
      }
    } else {
      // A colour play reveal outranks the leftmost newly touched card when it is
      // one on the stacks the giver and the receiver share (v22.5.0, the user's
      // ruling, reversing v18.20.0's every-seat frame): the receiver is the one
      // who must see the reveal. Replay 2022792 T30: yagami_black's Green
      // re-touched yagami_blue's clued o2, the g3, with green on 2 for the pair;
      // yagami_green had green on 1, so on the common stacks blue read the newly
      // touched o26 instead.
      out = reactor0::stable_colour(p, g, action, stall_ctx, &frame);
      // §1f, applied to whatever the ladder called.
      pin_rainbowy_colour(p, g, action);
    }
    return out;
  };
  auto called_something = [&](const Game& g) {
    for (int o : g.state.hands[action.target]) {
      if (g.meta[o].status == CardStatus::CALLED_TO_PLAY &&
          prev.meta[o].status != CardStatus::CALLED_TO_PLAY) {
        return true;
      }
    }
    return false;
  };

  const Game before_ladder = game;
  std::optional<ClueInterp> interp = run_ladder(game, view);
  auto newly_called = [&](const Game& g) {
    std::vector<int> out;
    for (int o : g.state.hands[action.target]) {
      if (g.meta[o].status == CardStatus::CALLED_TO_PLAY &&
          prev.meta[o].status != CardStatus::CALLED_TO_PLAY) {
        out.push_back(o);
      }
    }
    return out;
  };
  // Whether a newly touched card could, on what the clue publicly says of it, be a
  // direct play on the pair's view. A seat that cannot see it reads that call
  // first, so a reveal found only in a world must not answer a seat that refused it
  // by sight (below, and v22.1.0's block).
  auto newly_touched_could_play_on_view = [&]() {
    const State on_view = before_ladder.state.with_stacks(view);
    for (int o : action.list_) {
      if (prev.state.deck[o].clued) continue;
      for (Identity i : before_ladder.common.thoughts[o].possible) {
        if (on_view.is_playable(i)) return true;
      }
    }
    return false;
  };

  // §1e, v16.24.0: the ladder read the call in ONE frame -- the minimum across the
  // worlds of the pair's hole cards -- and in each of those worlds it names a
  // different card. A seat that can SEE the card judges the call against it, so on
  // the minimum frame it may refuse a call that is perfectly sound in the world the
  // pair is actually in. Then the call is read in each world, and the first that
  // makes it is the call.
  //
  // Replay 2011397 T10: yagami's Blue named will-bot69's o8, the b3, with its o9
  // `{b2,p2}` in the hole. On the minimum frame (blue on 1) will-bot67, who could
  // see the b3, refused the call and read a MISTAKE; will-bot69, who could not,
  // read `{b2}`. In the world where o9 was the b2, the call is the b3.
  //
  // An OUTSIDE seat also tries the worlds of its own hole cards (v17.1.0): the pair
  // watched those go in, so a world of them is one the pair may well be in, and
  // the call can rest on it. E.g. Cathy's Blue names Alice's b3 on the b2 Bob
  // blind-played a turn earlier; Bob cannot name his own b2, so without this he
  // finds no call on any frame of theirs and reads a MISTAKE.
  if (!called_something(game)) {
    std::vector<int> holders{action.giver, action.target};
    const int me = before_ladder.state.our_player_index;
    if (me != action.giver && me != action.target) holders.push_back(me);
    const State frame_base = before_ladder.state.with_stacks(view).with_band(
        before_ladder.state.evidence_known_to_both(action.giver, action.target));
    auto all = open_worlds(before_ladder, frame_base, holders);
    // Too many hole cards between the three seats and the enumeration reads flat --
    // one world, which can never make the call. An outside seat then still tries
    // the worlds of its OWN hole cards, the half this re-run exists for (v20.18.0).
    // Replay 2019555 T12: o10, o12 and o15 of the pair with will-bot69's o7 `{r1,y1}`
    // came to 144 worlds; read flat, will-bot67's Red to yagami's r2 found no call,
    // though in the world where o7 was the r1 it is the r2.
    if (all.size() <= 1 && holders.size() > 2) {
      all = open_worlds(before_ladder, frame_base, me);
    }
    const auto worlds = strike_free(all);
    if (worlds.size() > 1) {
      // Every world that makes the call, keeping the first one's reading.
      std::vector<const OpenWorld*> calling;
      // A colour REVEAL found only in a world stands only when no newly touched
      // card could be a direct play on the pair's view (v22.5.0). Otherwise the
      // seat that cannot see that card reads the direct call on its first run and
      // never reaches the worlds, while a seat that sees it refused it and finds
      // the reveal: giver and receiver split (self-play 6 Suits seed 40 T40).
      const bool direct_first = action.clue.kind == ClueKind::COLOUR &&
                                newly_touched_could_play_on_view();
      for (const OpenWorld* w : worlds) {
        Game g = before_ladder;
        auto i2 = run_ladder(g, w->state.play_stacks);
        if (!called_something(g)) continue;
        if (direct_first) {
          bool reveal = true;
          for (int o : newly_called(g)) {
            if (!prev.state.deck[o].clued) reveal = false;
          }
          if (reveal) continue;
        }
        if (calling.empty()) {
          game = std::move(g);
          interp = i2;
        }
        calling.push_back(w);
      }
      // A call that rests on our own hole cards tells us what they were, as the
      // back-solve does (§1e rule 4): only the worlds that make it are ours. What
      // we learn is private -- the pair already knew.
      //
      // The TARGET learns it too (v17.5.0): the giver watched our hole cards go in,
      // so a call that makes sense in only some of their worlds says which. Only
      // when every card it calls is one we can already name, though. The giver
      // judges the call against the card it SEES, and we against our empathy, so
      // with an unnamed card the worlds that make the call at our seat can be
      // ones the giver never meant -- a trash reveal can read as a play call on
      // `{g2,g3,g4,g5}` and settle our hole card on a g1 it was not. With a named
      // card it is sound: a Blue on our known b2 is a call only where our `{b1,p1}`
      // hole card was the b1, and without this we would sit on the b2 and throw
      // other cards in its place.
      auto calls_named_cards = [&]() {
        for (int o : game.state.hands[action.target]) {
          if (game.meta[o].status != CardStatus::CALLED_TO_PLAY ||
              prev.meta[o].status == CardStatus::CALLED_TO_PLAY) {
            continue;
          }
          if (game.common.thoughts[o].possible.length() != 1) return false;
        }
        return true;
      };
      // Except a COLOUR clue whose call re-touches a card that was already clued
      // (v18.12.0, the reviewer's rule): playable in the worlds that make the call,
      // trash in the others, so it may be asking for the dupe to be thrown. The
      // collapse waits for the holder -- a play is evidence by itself, and a
      // discard is rule 7's shared form. Human diagnostic 2014076 T14.
      auto retouches_a_clued_card = [&]() {
        if (action.clue.kind != ClueKind::COLOUR) return false;
        for (int o : game.state.hands[action.target]) {
          if (game.meta[o].status == CardStatus::CALLED_TO_PLAY &&
              prev.meta[o].status != CardStatus::CALLED_TO_PLAY &&
              prev.state.deck[o].clued) {
            return true;
          }
        }
        return false;
      };
      if (!calling.empty() && calling.size() < worlds.size() &&
          retouches_a_clued_card()) {
        // Deferred: no narrowing yet.
      } else if (!calling.empty() &&
          (holders.size() > 2 || (me == action.target && calls_named_cards()))) {
        bool narrowed = false;
        for (const auto& [ord, unused] : calling.front()->assignment) {
          (void)unused;
          if (before_ladder.state.holder_of(ord) != me) continue;
          IdentitySet allowed = IdentitySet::empty();
          for (const OpenWorld* w : calling) {
            for (const auto& [o2, id2] : w->assignment) {
              if (o2 == ord) allowed = allowed.add(id2);
            }
          }
          if (narrow_own_privately(game, ord, allowed)) narrowed = true;
        }
        if (narrowed) game.elim();
      }
    }
  }
  // A COLOUR REVEAL IN SOME WORLDS (v22.1.0, the user's ruling). On the pair's stacks
  // the reveal (v22.5.0) may fail only because a hole card is unnamed. If a world
  // exists in which the colour clue promises a playable, that is assumed: the shared
  // worlds of every seat's hole cards are tried, each raising the pair's view to the
  // world's stacks, and the hole collapses, for every seat, onto those that make the
  // call (ASCR, §1e).
  //
  // Except when the called card is TRASH in a world that does not make the call: the
  // clue may then be asking for the dupe to be thrown, and the collapse waits for the
  // holder (v18.12.0; human diagnostic 2014076 T14).
  //
  // Replay 2021427 T13: black's Green re-touched yagami_green's known 2, o6 (the g2),
  // and newly touched o7 (the g5). will-bot69's o13 `{g1,b1}` was the g1, so green
  // was on 0 globally and every seat read a stall. In the world where o13 was the g1
  // the Green is a reveal of the g2: o13 collapses to the g1, and o6 is called.
  //
  // A reveal already called on the pair's view (v22.5.0) is collapsed the same way
  // when the common stacks do not make it: every seat reads the reveal, so every
  // seat learns the hole card under it. The pair's frame may rest on the worlds of
  // our own hole cards (§1e), whose narrowing is otherwise private.
  bool reveal_ahead_of_common = false;
  if (action.clue.kind == ClueKind::COLOUR && action.giver != action.target &&
      called_something(game)) {
    const std::vector<int> called = newly_called(game);
    bool reveal = true;
    for (int o : called) {
      if (!prev.state.deck[o].clued) reveal = false;
    }
    if (reveal) {
      Game g = before_ladder;
      run_ladder(g, before_ladder.state.common_play_stacks);
      reveal_ahead_of_common = newly_called(g) != called;
    }
  }
  if ((!called_something(game) || reveal_ahead_of_common) &&
      action.clue.kind == ClueKind::COLOUR && action.giver != action.target) {
    const State& bs = before_ladder.state;
    std::vector<int> everyone;
    for (int p = 0; p < bs.num_players; ++p) everyone.push_back(p);
    const State shared_base = bs.common_evidence.empty()
                                  ? bs.shared_view()
                                  : bs.shared_view().with_band(bs.common_evidence);
    const auto all = open_worlds(before_ladder, shared_base, everyone, 64, -1,
                                 /*shared=*/true);
    const auto worlds = strike_free(all);
    // Only a REVEAL, and only when the clue calls nothing for a reason every seat
    // shares: no newly touched card could, on what the clue publicly says of it, be
    // a direct play in any world. A seat that can SEE a newly touched card refuses a
    // direct call on it that the receiver, who cannot, still reads; letting the
    // world reveal answer that refusal splits the giver from the receiver
    // (self-play Prism seed 200 T10: the giver read the newly touched o9 as `{i4}`,
    // the receiver as a direct `{b1}`, and struck).
    bool newly_touched_could_play = false;
    for (int o : action.list_) {
      if (prev.state.deck[o].clued) continue;
      for (Identity i : before_ladder.common.thoughts[o].possible) {
        if (bs.with_stacks(view).is_playable(i)) newly_touched_could_play = true;
        for (const OpenWorld* w : worlds) {
          if (w->state.is_playable(i)) newly_touched_could_play = true;
        }
      }
    }
    if (worlds.size() > 1 && !newly_touched_could_play) {
      std::vector<const OpenWorld*> calling;
      std::optional<Game> first;
      std::optional<ClueInterp> first_interp;
      std::vector<int> first_called;
      bool split = false;  // worlds calling different cards: no one reading to assume
      for (const OpenWorld* w : worlds) {
        // The call frame is the pair's view, raised to the world where it is behind.
        std::vector<int> frame = view;
        for (std::size_t k = 0; k < frame.size() && k < w->state.play_stacks.size();
             ++k) {
          const int v = w->state.play_stacks[k];
          const bool reversed = bs.variant->suits[k].suit_type.reversed;
          if (reversed ? v < frame[k] : v > frame[k]) frame[k] = v;
        }
        Game g = before_ladder;
        auto i2 = run_ladder(g, frame);
        if (!called_something(g)) continue;
        // Worlds that call DIFFERENT cards read the clue different ways, and there is
        // no one reading to assume: nothing collapses (replay 2014561 T18: where blue
        // was on 2 the Blue revealed o18's b3, where it was on 3 -- the truth -- it
        // called the newly touched b4).
        const std::vector<int> called_here = newly_called(g);
        if (!first) {
          first = std::move(g);
          first_interp = i2;
          first_called = called_here;
        }
        if (called_here == first_called) calling.push_back(w);
        else split = true;
      }
      bool trash_elsewhere = false;
      if (first && calling.size() < worlds.size()) {
        for (int o : first_called) {
          const IdentitySet reading = first->common.thoughts[o].possibilities();
          // ...and a world is assumed only if this seat cannot rule it out: a call
          // that is trash on our own view promises nothing we can believe. Replay
          // 2014561 T18: the team's set for black's o21 had lost its truth, the b3,
          // so no shared world had blue on 3; every seat that watched o21 land knew
          // the re-touched b3 was already down.
          bool trash_here = reading.non_empty();
          for (Identity i : reading) {
            if (!bs.is_basic_trash(i)) trash_here = false;
          }
          if (trash_here) trash_elsewhere = true;
          for (const OpenWorld* w : worlds) {
            if (std::find(calling.begin(), calling.end(), w) != calling.end()) continue;
            bool all_trash = reading.non_empty();
            for (Identity i : reading) {
              if (!w->state.is_basic_trash(i)) all_trash = false;
            }
            if (all_trash) trash_elsewhere = true;
          }
        }
      }
      // ...and the call must be on a card that was already clued: a reveal.
      bool reveal = first.has_value();
      for (int o : first_called) {
        if (!prev.state.deck[o].clued) reveal = false;
      }
      if (reveal && !split && calling.size() < worlds.size() && !trash_elsewhere) {
        game = std::move(*first);
        interp = first_interp;
        collapse_to_worlds(game, all, calling, /*shared=*/true);
        // Several hole cards may each be the card below the call (2021427: o5 and
        // o13 could both be the g1), and card by card the collapse cannot say so.
        // The call names a playable, so the card below it is down: the team learns
        // that one of them was it -- settled when one, the joint fact when several.
        for (int o : game.state.hands[action.target]) {
          if (game.meta[o].status != CardStatus::CALLED_TO_PLAY ||
              prev.meta[o].status == CardStatus::CALLED_TO_PLAY) {
            continue;
          }
          const IdentitySet reading = game.common.thoughts[o].possibilities();
          if (reading.length() != 1) continue;
          const Identity id = *reading.begin();
          const bool reversed = bs.variant->suits[id.suit_index].suit_type.reversed;
          if (const auto below = reversed ? id.next() : id.prev()) {
            team_learns_a_hole_card_was(game, *below);
          }
        }
        hanabi::logging::log_branch("tiiah.ascr",
                                    {{"site", "colour_reveal"},
                                     {"worlds", static_cast<int>(calling.size())}});
      }
    }
  }
  // A colour re-touch of Bob's that is still a plain stall is the self colour
  // bluff (v23.16.0, experimental): his special-suit card is called.
  if (interp && *interp == ClueInterp::STALL && !called_something(game) &&
      self_colour_bluff(prev, game, action)) {
    interp = ClueInterp::PLAY;
  }
  // ...and the reading is the union over those worlds, whichever frame made the
  // call: the singleton `{b2}` told every seat o9 was the p2.
  read_stable_over_worlds(prev, game, action, view);
  repin_own_call(prev, game, action.giver);
  return interp;
}

std::optional<Identity> stable_one_identity(const State& state) {
  const auto& suits = state.variant->suits;
  const int n = static_cast<int>(suits.size());
  if (n == 0) return std::nullopt;
  const std::vector<int>& stacks = state.common_play_stacks.size() == suits.size()
                                       ? state.common_play_stacks
                                       : state.play_stacks;
  auto named = [](const std::string& name, std::initializer_list<const char*> set) {
    return std::any_of(set.begin(), set.end(), [&name](const char* s) { return name == s; });
  };
  const std::string& last = suits[n - 1].name;
  const bool has_special =
      !named(last, {"Red", "Yellow", "Green", "Blue", "Purple", "Teal"});
  const bool special_untouched =
      has_special && named(last, {"Brown", "Dark Brown", "Muddy Rainbow", "Cocoa Rainbow",
                                  "Null", "Dark Null"});
  auto ones_play = [&](int s) {
    const auto& t = suits[s].suit_type;
    return !t.reversed && !t.inverted && stacks[s] == 0;
  };
  if (has_special && !special_untouched && ones_play(n - 1)) return Identity(n - 1, 1);
  for (int s = n - 1; s >= 0; --s) {
    if (special_untouched && s == n - 1) continue;
    if (ones_play(s)) return Identity(s, 1);
  }
  return std::nullopt;
}

}  // namespace hanabi::tiiah
