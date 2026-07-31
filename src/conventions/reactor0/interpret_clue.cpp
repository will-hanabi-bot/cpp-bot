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

// The leftmost (slot-ascending) order in `candidates` whose common empathy
// intersects the playable set or a delayed-play successor. POV-safe: only
// common thoughts are consulted.
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

}  // namespace

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

  // 1. Play reveal: a previously-clued card became an obvious playable.
  if (find_play_reveal(prev, game, action)) return ClueInterp::REVEAL;

  // 2. The leftmost touched card that could be playable is called to play.
  auto target = leftmost_could_be_playable(game, action, action.list_);
  if (target) {
    // Guards shared with reactor's ref_play
    // (src/conventions/reactor/interpret_clue.cpp:291-311): don't stack a
    // play promise on a blind-playing card; a CTD'd card is only a valid
    // play target if it is visibly playable; and a CTP on an inverted
    // (orange) card would pitch it into the discard pile.
    if (game.is_blind_playing(*target)) return std::nullopt;
    auto target_id = game.state.deck[*target].id();
    if (game.meta[*target].status == CardStatus::CALLED_TO_DISCARD &&
        !(target_id && game.state.is_playable(*target_id))) {
      return std::nullopt;
    }
    if (target_id && variants::is_inverted_id(game.state, *target_id)) {
      return std::nullopt;
    }
    return hanabi::reactor::target_play(game, action, *target,
                                        /*urgent=*/false, /*stable=*/true);
  }

  // 3. The receiver knows none of the touched cards can be playable.
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

  // Classify the rank over every identity the clue can actually touch
  // (variant-aware — a rank-5 clue under Pink-Fives touches pink cards of
  // other ranks too).
  bool all_trash = true;
  bool playable_rank = true;
  for (Identity id : state.variant->touch_possibilities(clue.kind, clue.value)) {
    bool basic = state.is_basic_trash(id);
    if (!basic) all_trash = false;
    if (!basic && !state.is_playable(id)) playable_rank = false;
  }

  // 1. Direct play clue: all remaining useful identities of the rank are
  //    playable (assuming good touch).
  if (playable_rank && !all_trash) {
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
          [&](Identity i) { return state.is_playable(i); });
      if (new_inferred.is_empty()) return std::nullopt;
      game.with_thought(*focus, [&](const Thought& t) {
        Thought out = t;
        out.inferred = new_inferred;
        out.info_lock = std::optional<IdentitySet>{new_inferred};
        return out;
      });
      CardStatus focus_status = variants::called_focus_status(state, new_inferred);
      game.with_meta(*focus, [focus_status](ConvData& m) {
        m.focused = true;
        m.status = focus_status;
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
