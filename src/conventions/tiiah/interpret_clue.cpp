#include "hanabi/conventions/tiiah/interpret_clue.h"

#include <optional>

#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/conventions/tiiah/interpret_reactive.h"
#include "hanabi/conventions/variants/hole.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::tiiah {

namespace {


// Read a stable clue on the SHARED stacks for as long as this object lives
// (CONVENTION.md §1.3). The giver chose the clue from what everyone knows, so
// every seat has to decode it from the same stacks — and §1e's "assume none of
// the superposed cards were played" is exactly what the shared view says,
// because a superposed play never advanced it.
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
void repin_own_call(const Game& prev, Game& game) {
  const State& s = game.state;
  if (s.pairwise_play_stacks.empty()) return;
  const int me = s.our_player_index;
  if (me < 0 || me >= static_cast<int>(s.hands.size())) return;
  for (int o : s.hands[me]) {
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    if (o < static_cast<int>(prev.meta.size()) &&
        prev.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      continue;  // a standing call, already settled by the clue that made it
    }
    const IdentitySet kept =
        game.common.thoughts[o].inferred.intersect(s.playable_set);
    if (kept.is_empty() || kept == game.common.thoughts[o].inferred) continue;
    game.with_thought(o, [&kept](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      out.inferred = kept;
      return out;
    });
  }
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
    if (!game.common.thoughts[o].possibilities().contains(*pin)) return;
    game.narrow_thought(o, IdentitySet::single(*pin));
    return;
  }
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
  const bool reversed =
      hanabi::reactor::variants::reverse_reactive_position(prev, action.giver);
  if (reversed) {
    if (action.target == bob) {
      return interpret_reactive(prev, game, action, /*reacter=*/cathy,
                                /*receiver=*/bob);
    }
    // ...and a clue to Cathy is stable, which is the whole point of the flip.
  } else if (cathy != action.giver && action.target != bob) {
    return interpret_reactive(prev, game, action, /*reacter=*/bob,
                              /*receiver=*/cathy);
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
  const bool swap = SharedStacks::needed(state, view);
  std::optional<Game> prev_shared;
  if (swap) {
    prev_shared = prev;
    prev_shared->state = prev.state.with_stacks(view);
  }
  const Game& p = swap ? *prev_shared : prev;

  std::optional<ClueInterp> interp;
  {
    // Scoped so the swap is RELEASED before the re-pin below, which has to see
    // our own belief rather than the pair's view.
    std::optional<SharedStacks> scope;
    if (swap) scope.emplace(game.state, view);
    if (action.clue.kind != ClueKind::COLOUR) {
      interp = reactor0::stable_rank(p, game, action, stall_ctx);
    } else {
      interp = reactor0::stable_colour(p, game, action, stall_ctx);
      // §1f, applied to whatever the ladder called.
      pin_rainbowy_colour(p, game, action);
    }
  }
  repin_own_call(prev, game);
  return interp;
}

}  // namespace hanabi::tiiah
