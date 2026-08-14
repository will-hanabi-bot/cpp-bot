#include "hanabi/conventions/reactor0/interpret_clue.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/fix.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor/interpret_clue.h"
#include "hanabi/conventions/reactor/interpret_reactive.h"
#include "hanabi/conventions/reactor0/interpret_reactive.h"
#include "hanabi/conventions/variants/brownish.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/conventions/variants/pinkish.h"
#include "hanabi/conventions/variants/predicates.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor0 {

namespace variants = hanabi::reactor::variants;

namespace {

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

std::vector<int> newly_touched_of(const Game& prev, const ClueAction& action) {
  std::vector<int> out;
  for (int o : action.list_) {
    if (!prev.state.deck[o].clued) out.push_back(o);
  }
  return out;
}

// prev/post obvious playables + giver-visible connectables for `target`,
// mirroring reactor's try_stable fill-in machinery
// (src/conventions/reactor/interpret_clue.cpp:527-560).
std::vector<int> unique_concat(std::vector<int> a, const std::vector<int>& b) {
  std::unordered_set<int> seen(a.begin(), a.end());
  for (int x : b) {
    if (!seen.count(x)) {
      a.push_back(x);
      seen.insert(x);
    }
  }
  return a;
}

// A previously-clued card this clue fills in as a NEW obvious playable /
// connectable ("play reveal"). For rank clues newly-touched cards count
// too, matching reactor's find_reveal.
std::optional<int> find_play_reveal(const Game& prev, Game& game,
                                    const ClueAction& action) {
  const auto& clue = action.clue;
  int giver = action.giver;
  int target = action.target;
  int next_player_index = game.state.next_player_index(giver);

  auto prev_playables = unique_concat(
      prev.common.obvious_playables(prev, target),
      connectable_simple(prev, prev.players[giver], next_player_index, target));

  std::vector<Interp> saved_history = game.move_history;
  game.move_history.push_back(ClueInterp::PLAY);
  auto playables = unique_concat(
      game.common.obvious_playables(game, target),
      connectable_simple(game, game.players[giver], next_player_index, target));
  game.move_history = std::move(saved_history);

  for (int o : playables) {
    if (contains(action.list_, o) && !contains(prev_playables, o) &&
        (clue.kind == ClueKind::RANK || prev.state.deck[o].clued)) {
      return o;
    }
  }
  return std::nullopt;
}

// --- orange (inverted-suit) readings --------------------------------------
//
// Vocabulary, from reactor's GLOSSARY "pitch / chuck": **pitch** = press the
// Play button, **chuck** = press the Discard button. For an inverted suit the
// game swaps their outcomes, so a chuck advances the orange stack and a pitch
// sends the card to the discard pile (regaining a clue). CTP/CTD name the
// BUTTON, so "get an orange card onto its stack" means stamping CTD.
//
// All three predicates read `common` + `possible`, which is POV-invariant:
// giver, receiver and every observer compute the same answer, so the
// convention may branch on them without desyncing (§1g).

bool could_be_inverted(const Game& game, int order) {
  const State& state = game.state;
  return game.common.thoughts[order].possible.exists(
      [&](Identity i) { return variants::is_inverted_id(state, i); });
}

// Known — not merely possible — to be an orange identity that is currently
// playable, i.e. chucking it definitely advances the stack.
bool known_playable_inverted(const Game& game, int order) {
  const State& state = game.state;
  const IdentitySet& poss = game.common.thoughts[order].possible;
  return poss.non_empty() && poss.forall([&](Identity i) {
    return variants::is_inverted_id(state, i) && state.is_playable(i);
  });
}

// Could a chuck still land this card on a stack, from the holder's POV?
bool could_reach_stacks(const Game& game, int order) {
  const State& state = game.state;
  return game.common.thoughts[order].possible.exists([&](Identity i) {
    return variants::is_inverted_id(state, i) && state.is_playable(i);
  });
}

// The touched cards that could be orange, LEFT to RIGHT. `state.hands` is
// stored leftmost (newest) first, which is the same walk
// `leftmost_could_be_playable` does.
std::vector<int> orange_touched(const Game& game, const ClueAction& action) {
  std::vector<int> out;
  for (int o : game.state.hands[action.target]) {
    if (contains(action.list_, o) && could_be_inverted(game, o)) out.push_back(o);
  }
  return out;
}

// Stamp a CHUCK. Mirrors reactor's idiom (`target_play` +
// `called_focus_status`, reactor/interpret_clue.cpp:132 and
// variants/inverted.cpp:53-61): narrow `inferred` to the identities that
// would actually advance a stack, then mark CALLED_TO_DISCARD.
//
// `reactor::target_discard` is deliberately NOT reused: it filters `inferred`
// to the NON-critical ids — the "this is safe to throw away" reading — which
// is the opposite of what a chuck wants, and empties the set outright in Dark
// Orange, where every card is critical (src/basics/variant.cpp:183).
std::optional<ClueInterp> stamp_orange_chuck(Game& game, const ClueAction& action,
                                             int order) {
  const State& state = game.state;
  IdentitySet keep =
      game.common.thoughts[order].possibilities().filter([&](Identity i) {
        return variants::is_inverted_id(state, i) && state.is_playable(i);
      });
  if (keep.is_empty()) return std::nullopt;
  game.with_thought(order, [&](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    out.inferred = keep;
    out.info_lock = std::optional<IdentitySet>{keep};
    return out;
  });
  const int turn = state.turn_count;
  const int giver = action.giver;
  game.with_meta(order, [turn, giver](ConvData& m) {
    m.focused = true;
    m.status = CardStatus::CALLED_TO_DISCARD;
    m.by = giver;
    m = m.reason(turn).signal(turn);
  });
  return ClueInterp::PLAY;
}

// Stamp a PITCH — press Play, sending the orange to the discard pile and
// regaining a clue. `reactor::target_play` is NOT reused: it narrows
// `inferred` to the playable set, but a pitched orange is being thrown away
// and need not be playable at all. Only the "it is orange" part of the
// promise is recorded.
std::optional<ClueInterp> stamp_orange_pitch(Game& game, const ClueAction& action,
                                             int order) {
  const State& state = game.state;
  IdentitySet keep = game.common.thoughts[order].possibilities().filter(
      [&](Identity i) { return variants::is_inverted_id(state, i); });
  if (keep.is_empty()) return std::nullopt;
  game.with_thought(order, [&](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    out.inferred = keep;
    return out;
  });
  const int turn = state.turn_count;
  const int giver = action.giver;
  game.with_meta(order, [turn, giver](ConvData& m) {
    m.focused = true;
    m.status = CardStatus::CALLED_TO_PLAY;
    m.by = giver;
    m = m.reason(turn).signal(turn);
  });
  return ClueInterp::DISCARD;
}

}  // namespace

// Exported (declared in interpret_clue.h) so the decision layer can ask
// "what would this stable colour clue name?" without simulating it —
// src/conventions/reactor0/state_eval.cpp uses it for the NOT-LOW rule that
// checks whether Bob already has a colour stable play clue for Cathy.
std::optional<int> leftmost_could_be_playable(
    const Game& game, const ClueAction& action,
    const std::vector<int>& candidates) {
  const State& state = game.state;
  auto conns = hanabi::reactor::delayed_plays(game, action.giver, action.target,
                                              /*stable=*/true);
  for (int o : state.hands[action.target]) {
    if (!contains(candidates, o)) continue;
    const Thought& t = game.common.thoughts[o];
    IdentitySet base = t.inferred.non_empty() ? t.inferred : t.possible;
    bool could_play = base.exists([&](Identity i) {
      if (state.playable_set.contains(i)) return true;
      for (const auto& [_, c] : conns) {
        if (c == i) return true;
      }
      return false;
    });
    if (could_play) return o;
  }
  return std::nullopt;
}

// --- stable colour --------------------------------------------------------

std::optional<ClueInterp> stable_colour(const Game& prev, Game& game,
                                        const ClueAction& action, bool stall) {
  hanabi::instr::ScopedTimer st("reactor0.stable_colour");
  hanabi::logging::LogScope ls(
      "reactor0.stable_colour",
      {{"giver", action.giver}, {"target", action.target}, {"stall", stall}});
  (void)stall;

  // Fixes outrank everything — a colour clue that resets a wrong earlier
  // inference must not be read as a fresh play promise.
  FixResult fix_result = check_fix(prev, game, action);
  if (std::holds_alternative<FixResultNormal>(fix_result)) {
    return ClueInterp::FIX;
  }

  const State& state = game.state;

  // 1. Play reveal: a previously-clued card became an obvious playable. If it
  //    is a known playable ORANGE the receiver must CHUCK it (press Discard),
  //    so the reveal is stamped CTD — a bare REVEAL stamps nothing and would
  //    leave the physical action to empathy alone.
  if (auto revealed = find_play_reveal(prev, game, action)) {
    if (known_playable_inverted(game, *revealed)) {
      stamp_orange_chuck(game, action, *revealed);
    }
    return ClueInterp::REVEAL;
  }

  // 1b. Orange play reveal. An orange colour clue that REVEALS a playable
  //     orange is a play reveal and tells the receiver to chuck that card,
  //     and this reading takes priority over the pitch/chuck ladder below.
  //     `find_play_reveal` alone does not cover it: for a colour clue it only
  //     considers cards that were ALREADY clued (`:78-83`), because a newly
  //     touched card becoming obviously playable is the ordinary direct-play
  //     reading — which for orange is exactly the case that has to change.
  if (variants::includes_inverted(state)) {
    for (int o : orange_touched(game, action)) {
      if (!known_playable_inverted(game, o)) continue;
      if (game.is_blind_playing(o)) continue;
      if (stamp_orange_chuck(game, action, o)) return ClueInterp::REVEAL;
      break;
    }
  }

  // 2. The orange ladder, reached only when no playable orange was revealed.
  //
  //    A colour clue naming the inverted suit is a call to get rid of, or to
  //    stack, one specific orange card:
  //      * non-dark orange at pace > 3 -> PITCH the leftmost touched orange
  //        the receiver does not know is critical. Pitching sends it to the
  //        discard pile and regains a clue, which is only worth doing while
  //        there is pace to spare and the card is expendable.
  //      * pace <= 3, or the suit is DARK -> CHUCK the leftmost touched
  //        orange instead, putting it on the stacks. Every Dark Orange card
  //        is a singleton (src/basics/variant.cpp:183), so pitching one is an
  //        unrecoverable loss and the chuck is the only sane reading.
  //    All-critical fallback: chuck the leftmost one that could still reach
  //    the stacks from the receiver's POV; if none could, the clue stalls.
  //    The chuck target is then vetted against what the GIVER can see (§1g):
  //    a chuck of a card we can see is not currently playable strikes, so we
  //    reject the clue rather than retarget.
  std::vector<int> oranges = orange_touched(game, action);
  if (!oranges.empty()) {
    const bool pitch_mode =
        !variants::includes_dark_inverted(state) && state.pace() > 3;
    if (pitch_mode) {
      for (int o : oranges) {
        if (holder_knows_critical(game, o)) continue;
        if (game.is_blind_playing(o)) continue;
        if (auto r = stamp_orange_pitch(game, action, o)) return r;
      }
    }
    for (int o : oranges) {
      if (!could_reach_stacks(game, o)) continue;
      if (game.is_blind_playing(o)) continue;
      // §1g: the two `continue`s above read common knowledge, so the receiver
      // walks on with us. THIS reads `state.deck[o]`, which the receiver
      // cannot see — giver-only knowledge may only REJECT. The receiver will
      // chuck this card whatever we do, and a chuck of a non-playable orange
      // is a misplay strike, so the clue must not be offered at all. Walking
      // on to the next orange would desync: the receiver still computes this
      // one. Mirrors `would_lose_inverted_reacter`'s second half
      // (src/conventions/variants/inverted.cpp:44-51) on the reactive side.
      if (auto id = state.deck[o].id(); id && !state.is_playable(*id)) {
        return std::nullopt;
      }
      if (auto r = stamp_orange_chuck(game, action, o)) return r;
    }
    return ClueInterp::STALL;
  }

  // 3. The leftmost touched card that could be playable is called to play.
  auto target = leftmost_could_be_playable(game, action, action.list_);
  if (target) {
    // Guards shared with reactor's ref_play
    // (src/conventions/reactor/interpret_clue.cpp:291-311): don't stack a
    // play promise on a blind-playing card, and a CTD'd card is only a valid
    // play target if it is visibly playable.
    //
    // There is no longer an inverted-target reject here. It used to read
    // `state.deck[*target].id()`, which is POV-ASYMMETRIC — nullopt for the
    // receiver's own card, so giver and receiver disagreed. The orange ladder
    // above now claims every touched card that could be orange, and a card's
    // `possible` always contains its true identity, so an actually-orange
    // target can no longer reach this branch.
    if (game.is_blind_playing(*target)) return std::nullopt;
    auto target_id = game.state.deck[*target].id();
    if (game.meta[*target].status == CardStatus::CALLED_TO_DISCARD &&
        !(target_id && game.state.is_playable(*target_id))) {
      return std::nullopt;
    }
    return hanabi::reactor::target_play(game, action, *target,
                                        /*urgent=*/false, /*stable=*/true);
  }

  // 4. The receiver knows none of the touched cards can be playable.
  return ClueInterp::STALL;
}

// --- stable rank ----------------------------------------------------------

std::optional<ClueInterp> stable_rank(const Game& prev, Game& game,
                                      const ClueAction& action, bool stall) {
  hanabi::instr::ScopedTimer st("reactor0.stable_rank");
  hanabi::logging::LogScope ls(
      "reactor0.stable_rank",
      {{"giver", action.giver}, {"target", action.target}, {"stall", stall}});
  const State& state = game.state;
  const auto& clue = action.clue;
  auto newly_touched = newly_touched_of(prev, action);

  // Pink-promise gate, as in reactor: an illegal promise means MISTAKE
  // before any branch can stamp a partial interpretation.
  if (!newly_touched.empty() && variants::violates_pink_promise(prev, action)) {
    return std::nullopt;
  }

  // Classify the rank over what the cards this clue ACTUALLY TOUCHED can be,
  // not over the whole variant's touch set. **This is where reactor0 diverges
  // from reactor** (which still scans `variant->touch_possibilities`, see
  // reactor/interpret_clue.cpp:447-458 and its §1c).
  //
  // Why: in an omni variant a rank clue touches the omni suit at EVERY rank
  // (`Variant::id_touched` returns true for any pinkish suit on any rank
  // clue, src/basics/variant.cpp:230). So the variant-wide set for a rank-N
  // clue contains the omni suit at ranks 1-5, and a single useful-but-
  // unplayable omni rank made `playable_rank` false — which meant priority 1
  // essentially never fired in those variants and every rank clue degraded to
  // the referential discard at the bottom of this ladder. Replays 1942517 #1
  // (rank 1 at all-zero stacks read as a ref discard) and 1942525 T53 (a
  // playable Sky 4 never called) are the two worked examples.
  //
  // Two steps, and BOTH are needed:
  //   1. the pink promise. A rank clue in a pinkish variant promises the
  //      rank, so the omni suit's other ranks are off the table. Without this
  //      1942517 is not fixed: at turn 1 nothing has eliminated Dark Omni 2-5
  //      from the touched card's `possible`.
  //   2. per-card visibility, via `reactor::effective_possible_for`. That
  //      narrows the card's `possible` by the copies visible in every
  //      non-holder hand, which is POV-invariant by construction (it is
  //      defined from the HOLDER's viewpoint, so giver, receiver and every
  //      observer compute the same set) — the same helper the reactive vets
  //      use. Plain `common.thoughts` is NOT enough: without this 1942525 is
  //      not fixed, because Dark Omni 4 is a rank-4 identity and only its
  //      visibility in a third hand rules it out. Note the whole set can be
  //      empty for a card whose copies are all accounted for.
  IdentitySet touchable = IdentitySet::empty();
  for (int o : action.list_) {
    touchable |= hanabi::reactor::effective_possible_for(game, o);
  }
  // The pink promise, gated on the FLAG test rather than the name-based
  // `includes_pinkish`. `violates_pink_promise` uses the flags for a reason
  // its header spells out — do not unify the two.
  //
  // A true pink/omni SUIT is touched at every rank, so a rank-N clue promises
  // rank N. A special-rank variant is different: with `pink_s` (which is
  // exactly `specialRankAllClueRanks`, src/basics/variant.cpp:318) the special
  // rank is touched by EVERY clue rank, so a rank-N clue promises rank N **or
  // the special rank**. Filtering to N alone dropped the special rank and, at
  // replay 1942709 in "Pink-Ones & Orange", turned a lock into a play call.
  bool pinkish_flag = state.variant->pink_s;
  for (const Suit& suit : state.variant->suits) {
    if (suit.suit_type.pinkish) pinkish_flag = true;
  }
  if (pinkish_flag) {
    const int rv = clue.value;
    const std::optional<int> special =
        state.variant->pink_s ? state.variant->special_rank : std::nullopt;
    touchable = touchable.filter([rv, special](Identity i) {
      return i.rank == rv || (special && i.rank == *special);
    });
  }
  bool all_trash = true;
  bool playable_rank = true;
  bool useful_inverted = false;
  bool useful_plain = false;
  for (Identity id : touchable) {
    bool basic = state.is_basic_trash(id);
    if (basic) continue;
    all_trash = false;
    if (variants::is_inverted_id(state, id)) useful_inverted = true;
    else useful_plain = true;
    if (!state.is_playable(id)) playable_rank = false;
  }
  // A rank direct play clue means PITCH (press Play) by default, and pitching
  // an inverted card sends it to the discard pile instead of its stack. So a
  // useful orange identity can only be read as playable when EVERY useful
  // identity of the rank is orange — then the button is unambiguous and the
  // focus is called to CHUCK (press Discard) instead. That is the case where
  // every other suit's copy of the rank is already on the stacks, so the clue
  // names the orange one and nothing else (replay 1957905 #31).
  //
  // A MIXED useful set stays undecidable — the receiver could not tell which
  // button to press — and declines exactly as before (bug_report_3.txt 3.1,
  // `RankDirectPlayDeclinesOrangeAndLocksInstead`).
  const bool orange_only = useful_inverted && !useful_plain;
  if (useful_inverted && useful_plain) playable_rank = false;
  // An empty set teaches nothing — fall through rather than claim every
  // identity is playable vacuously.
  if (!touchable.non_empty()) {
    all_trash = false;
    playable_rank = false;
  }

  // The orange-only reading sits BELOW the play reveal of priority 2. When the
  // clue pins a previously-clued orange to a playable one, the reveal already
  // says everything — empathy carries the chuck, since `decide.cpp:885-894`
  // routes an empathy-pinned playable orange through PerformDiscard. Claiming
  // it at priority 1 would also trip the `unnecessary_focus` test below, which
  // counts the focus's OWN pinned identity as "visible elsewhere"
  // (`Thought::matches` is `id() == other`, src/basics/card.cpp:48-52) and
  // would turn the reveal into a STALL. Only the ORANGE-ONLY branch defers;
  // an ordinary direct play clue still outranks the reveal as before.
  const bool defer_to_reveal = orange_only && playable_rank && !all_trash &&
                               find_play_reveal(prev, game, action).has_value();

  // 1. Direct play clue: all remaining useful identities of the rank are
  //    playable (assuming good touch).
  if (playable_rank && !all_trash && !defer_to_reveal) {
    std::optional<int> focus;
    if (!newly_touched.empty()) {
      if (variants::includes_pinkish(state)) {
        focus = variants::playable_rank_focus(prev, state, action, newly_touched);
      } else {
        // Leftmost newly touched = highest order (slot 1 is newest).
        focus = *std::max_element(newly_touched.begin(), newly_touched.end());
      }
    } else {
      // No newly touched cards: the leftmost TOUCHED card that could be
      // playable is promised playable.
      focus = leftmost_could_be_playable(game, action, action.list_);
    }
    if (focus) {
      // An unnecessary focus (every possibility trash or already visible
      // elsewhere) teaches nothing — the clue is a stall, not a promise.
      bool unnecessary_focus = game.common.thoughts[*focus].possible.forall(
          [&](Identity i) {
            if (state.is_basic_trash(i)) return true;
            for (const auto& hand : state.hands) {
              for (int o : hand) {
                if (game.common.thoughts[o].matches(i)) return true;
              }
            }
            return false;
          });
      if (unnecessary_focus) return ClueInterp::STALL;

      IdentitySet new_inferred = game.common.thoughts[*focus].inferred.filter(
          [&](Identity i) {
            // Under the orange-only reading the call is a chuck, so narrow to
            // the identities a chuck actually advances — same reasoning as
            // `stamp_orange_chuck` above, and it keeps TODO #12's unpinned
            // -playable-orange hazard away from this focus.
            if (orange_only && !variants::is_inverted_id(state, i)) return false;
            return state.is_playable(i);
          });
      if (new_inferred.is_empty()) return std::nullopt;
      game.with_thought(*focus, [&](const Thought& t) {
        Thought out = t;
        out.inferred = new_inferred;
        out.info_lock = std::optional<IdentitySet>{new_inferred};
        return out;
      });
      // A rank direct play clue means PITCH, so the focus is stamped
      // CALLED_TO_PLAY — EXCEPT under the orange-only reading, where every
      // useful identity of the rank is inverted and the call is a chuck.
      // `variants::called_focus_status` is the shared helper for that
      // (it returns CTD for any inverted member of the set); reactor keeps it
      // at its own call site unconditionally
      // (reactor/interpret_clue.cpp:504), reactor0 gates it on `orange_only`
      // so a MIXED set can never reach it — a mixed set has already set
      // `playable_rank = false` and cannot get here.
      const CardStatus status =
          orange_only ? variants::called_focus_status(state, new_inferred)
                      : CardStatus::CALLED_TO_PLAY;
      game.with_meta(*focus, [status](ConvData& m) {
        m.focused = true;
        m.status = status;
      });
      return ClueInterp::PLAY;
    }
    // Playable rank but nothing to promise: fall through to the reveal /
    // stall tail below.
  }

  // 2. Play reveal: the clue fills a previously-clued card in as an obvious
  //    playable, without the whole rank being playable. Terminal, and ranked
  //    above every other reveal and above the referential readings — the
  //    receiver has a play to make, which outranks being told what to
  //    discard.
  if (find_play_reveal(prev, game, action)) return ClueInterp::REVEAL;

  // 3. Trash reveal: every touchable identity is trash. Terminal — never a
  //    referential reading.
  if (all_trash) {
    if (newly_touched.empty()) return ClueInterp::STALL;
    int focus = *std::max_element(newly_touched.begin(), newly_touched.end());
    IdentitySet ts = state.trash_set;
    game.with_thought(focus, [&](const Thought& t) {
      Thought out = t;
      out.inferred = t.inferred.intersect(ts);
      return out;
    });
    game.with_meta(focus, [](ConvData& m) { m.trash = true; });
    return ClueInterp::REVEAL;
  }

  // 4. Reveals a previously-clued card as trash or a same-hand dupe.
  FixResult fix_result = check_fix(prev, game, action);
  if (std::holds_alternative<FixResultNormal>(fix_result)) {
    return ClueInterp::FIX;
  }
  {
    auto prev_kt = prev.common.thinks_trash(prev, action.target);
    auto game_kt = game.common.thinks_trash(game, action.target);
    for (int o : game_kt) {
      if (prev.state.deck[o].clued && !contains(prev_kt, o)) {
        return ClueInterp::REVEAL;
      }
    }
  }
  if (!newly_touched.empty()) {
    int max_nt = *std::max_element(newly_touched.begin(), newly_touched.end());
    if (game.common.order_kt(game, max_nt) &&
        variants::brownish_trash_reveal(prev, game, action, newly_touched)) {
      return ClueInterp::REVEAL;
    }
  }

  // 5./6. Lock slot → LOCK, else referential discard — reactor's
  // ref_discard implements both, including the pink promise.
  if (!newly_touched.empty()) {
    return hanabi::reactor::ref_discard(prev, game, action, stall);
  }

  // 7. Touches no new cards, conveys nothing else.
  return ClueInterp::STALL;
}

// --- top-level dispatcher -------------------------------------------------

std::optional<ClueInterp> interpret_clue(const Game& prev, Game& game,
                                         const ClueAction& action) {
  hanabi::instr::ScopedTimer st("reactor0.interpret_clue");
  hanabi::logging::LogScope ls(
      "reactor0.interpret_clue",
      {{"giver", action.giver}, {"target", action.target}});
  const State& state = game.state;

  // Forced interp from a rewind. Reactor0 itself never rewinds (no
  // response inversion), but honour the mechanism for tests and shared
  // tooling.
  if (game.next_interp && std::holds_alternative<ClueInterp>(*game.next_interp)) {
    ClueInterp forced = std::get<ClueInterp>(*game.next_interp);
    if (forced == ClueInterp::REACTIVE) {
      int reacter = state.next_player_index(action.giver);
      return interpret_reactive(prev, game, action, reacter);
    }
    // fall through to positional dispatch for any other forced value.
  }

  if (state.options.empty_clues && action.list_.empty()) {
    return ClueInterp::USELESS;
  }

  int bob = state.next_player_index(action.giver);
  bool stall_ctx = prev.common.obvious_locked(prev, action.giver) ||
                   game.in_endgame() || prev.state.clue_tokens == 8;

  if (action.target == bob) {
    // Always stable, even if Bob is loaded.
    return action.clue.kind == ClueKind::COLOUR
               ? stable_colour(prev, game, action, stall_ctx)
               : stable_rank(prev, game, action, stall_ctx);
  }
  // Always reactive, even in stall contexts.
  return interpret_reactive(prev, game, action, bob);
}

}  // namespace hanabi::reactor0
