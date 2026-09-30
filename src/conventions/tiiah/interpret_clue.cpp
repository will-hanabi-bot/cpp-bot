#include "hanabi/conventions/tiiah/interpret_clue.h"

#include <optional>

#include "hanabi/basics/fix.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/conventions/tiiah/interpret_reactive.h"
#include "hanabi/conventions/tiiah/superposition.h"
#include "hanabi/conventions/reactor0/interpret_reactive.h"
#include "hanabi/conventions/variants/hole.h"
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
    if (giver >= 0 && giver != me) {
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
// clearing the waiting connection (`basics/decide.cpp:210-213`) — which is also
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
  if (!refusal) {
    const bool reversed =
        hanabi::reactor::variants::reverse_reactive_position(prev, action.giver);
    if (reversed) {
      // A clue to Bob that FIXES his dead standing call is the fix below, not a
      // reverse reactive (v18.10.0): the call that puts the table in the reverse
      // position is the very one the fix is for. Replay 2013726 T5.
      if (action.target == bob && !hanabi::clue_would_fix_dead_call(prev, action)) {
        return interpret_reactive(prev, game, action, /*reacter=*/cathy,
                                  /*receiver=*/bob);
      }
      // ...and a clue to Cathy is stable, which is the whole point of the flip.
    } else if (cathy != action.giver && action.target != bob &&
               !hanabi::reactor::variants::inverted_stable(prev, action.giver,
                                                           action.target)) {
      // ROLE INVERSION (v18.2.0): a clue to Cathy is also stable when Bob holds a
      // STANDING play -- any called card, not only a touch-known one -- and
      // Cathy does not. That is the only thing it changes: a clue to Bob stays
      // stable in that position too. Replay 2013645 T11.
      return interpret_reactive(prev, game, action, /*reacter=*/bob,
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
    game.with_meta(*fixed, [turn](ConvData& m) {
      m = m.cleared().reason(turn);
      m.note_mark = NoteMark::RESET;
      m.note_mark_turn = turn;
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
      out = reactor0::stable_rank(p, g, action, stall_ctx);
    } else {
      out = reactor0::stable_colour(p, g, action, stall_ctx);
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
    const auto all = open_worlds(
        before_ladder,
        before_ladder.state.with_stacks(view).with_band(
            before_ladder.state.evidence_known_to_both(action.giver, action.target)),
        holders);
    const auto worlds = strike_free(all);
    if (worlds.size() > 1) {
      // Every world that makes the call, keeping the first one's reading.
      std::vector<const OpenWorld*> calling;
      for (const OpenWorld* w : worlds) {
        Game g = before_ladder;
        auto i2 = run_ladder(g, w->state.play_stacks);
        if (!called_something(g)) continue;
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
  // ...and the reading is the union over those worlds, whichever frame made the
  // call: the singleton `{b2}` told every seat o9 was the p2.
  read_stable_over_worlds(prev, game, action, view);
  repin_own_call(prev, game, action.giver);
  return interp;
}

}  // namespace hanabi::tiiah
