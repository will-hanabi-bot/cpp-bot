#include "hanabi/basics/fix.h"

#include <algorithm>
#include <unordered_set>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"

namespace hanabi {

FixResult check_fix(const Game& prev, const Game& game, const ClueAction& action) {
  const auto& list_ = action.list_;
  std::vector<int> clued_resets;
  std::vector<int> duplicate_reveals;
  bool trash_only = true;

  for (auto it = list_.rbegin(); it != list_.rend(); ++it) {
    int order = *it;
    auto thought_id = game.common.thoughts[order].id();
    auto prev_thought_id = prev.common.thoughts[order].id();
    const bool prev_clued = prev.state.deck[order].clued;

    bool duplicated = false;
    if (prev_clued && thought_id && !prev_thought_id) {
      for (int o : list_) {
        if (o == order) continue;
        if (!prev.state.deck[o].clued) continue;
        if (game.state.deck[order].matches(game.state.deck[o])) {
          duplicated = true;
          break;
        }
      }
    }

    if (prev.common.order_kt(game, order)) continue;

    // Arm 1: a blind play the clue proves is trash. A correction -- left
    // unactioned it strikes.
    const bool blind_play_correction =
        prev.meta[order].status == CardStatus::CALLED_TO_PLAY &&
        prev.is_blind_playing(order) &&
        game.common.thoughts[order].info_lock &&
        game.common.thoughts[order].info_lock->forall(
            [&](Identity i) { return game.state.is_basic_trash(i); });
    // Arm 2: an already-clued card is NOW known trash. Information, not a
    // correction -- nothing goes wrong if the receiver acts on it later.
    const bool trash_revealed =
        prev.state.deck[order].clued && !prev.common.thoughts[order].reset &&
        game.common.order_kt(game, order);

    if (blind_play_correction || trash_revealed) {
      clued_resets.insert(clued_resets.begin(), order);
      if (blind_play_correction) trash_only = false;
    } else if (duplicated) {
      duplicate_reveals.insert(duplicate_reveals.begin(), order);
    }
  }

  if (!clued_resets.empty() || !duplicate_reveals.empty()) {
    return FixResultNormal{std::move(clued_resets), std::move(duplicate_reveals),
                           trash_only && duplicate_reveals.empty()};
  }
  return FixResultNone{};
}

namespace {

// What a hole card played by someone OTHER than `giver` may have been: the team's
// set for it (`shared_left` for a card settled privately, else `superposition`).
// The giver watched those cards go in, so a clue from it can be saying one of them
// was this -- the half of a fix a third seat reads off its own blind play (the
// reviewer's rule for fix precedence; 2013726 T5, black's `{r1,g1}`).
IdentitySet hole_candidates_not_by(const Game& game, int giver) {
  IdentitySet out = IdentitySet::empty();
  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    const ConvData& m = game.meta[o];
    const IdentitySet& team = m.shared_left.non_empty() ? m.shared_left : m.superposition;
    if (team.is_empty() || game.state.holder_of(o) == giver) continue;
    out = out.union_with(team);
  }
  return out;
}

}  // namespace

std::optional<int> dead_call_fix(const Game& before, const Game& after, int giver,
                                 int clued) {
  const State& s = after.state;
  if (!s.variant->throw_it_in_a_hole) return std::nullopt;
  if (clued < 0 || clued >= static_cast<int>(s.hands.size())) return std::nullopt;
  // The deadness is asked of the view the giver and the holder share -- the shared
  // view, for a third seat, which cannot compute theirs -- or of a hole card someone
  // other than the giver played, which the third seat CAN read off its own blind
  // play (v18.10.0): the clue then says that card was this identity.
  const State shared = s.with_stacks(s.stacks_known_to_both(giver, clued));
  const IdentitySet in_the_hole = hole_candidates_not_by(before, giver);
  for (int order : s.hands[clued]) {
    if (order >= static_cast<int>(before.meta.size())) continue;
    if (order >= static_cast<int>(before.common.thoughts.size())) continue;
    if (before.meta[order].status != CardStatus::CALLED_TO_PLAY) continue;
    // The holder could not tell. A call already down to one identity needs no fix:
    // either it is fine or the ordinary invariants have dropped it already.
    if (before.common.thoughts[order].possibilities().length() < 2) continue;
    const IdentitySet now = after.common.thoughts[order].possibilities();
    if (now.length() != 1) continue;
    if (!shared.is_basic_trash(now.head()) && !in_the_hole.contains(now.head())) continue;
    return order;
  }
  return std::nullopt;
}

std::optional<int> clue_would_fix_dead_call(const Game& before,
                                            const ClueAction& action) {
  const State& s = before.state;
  if (!s.variant->throw_it_in_a_hole) return std::nullopt;
  const int clued = action.target;
  if (clued < 0 || clued >= static_cast<int>(s.hands.size())) return std::nullopt;
  // DEAD TO THE TEAM, from what every seat holds alike (the reviewer's rule): trash
  // on the shared view, or a candidate of a hole card someone other than the giver
  // played -- the giver saw that card, so the clue can be telling us it was this.
  // Not the giver-and-holder stacks: the third seat cannot compute those when the
  // evidence is its own blind play (2013726 T5, black's `{r1,g1}`).
  const State shared = s.shared_view();
  const IdentitySet in_the_hole = hole_candidates_not_by(before, action.giver);
  for (int order : s.hands[clued]) {
    if (before.meta[order].status != CardStatus::CALLED_TO_PLAY) continue;
    const IdentitySet live = before.common.thoughts[order].possibilities();
    if (live.length() < 2) continue;
    // What the touches leave: the identities the clue touches if it touched the
    // card, the ones it misses if it did not.
    const bool touched = std::find(action.list_.begin(), action.list_.end(), order) !=
                         action.list_.end();
    const IdentitySet now = live.filter([&](Identity i) {
      return s.variant->id_touched(i, action.clue.kind, action.clue.value) == touched;
    });
    if (now.length() != 1) continue;
    if (!shared.is_basic_trash(now.head()) && !in_the_hole.contains(now.head())) continue;
    return order;
  }
  return std::nullopt;
}

std::optional<IdentitySet> distribution_clue(const Game& prev, const Game& game,
                                                const ClueAction& action, int focus) {
  const State& state = game.state;
  const Thought& thought = game.common.thoughts[focus];

  bool all_prev_clued = true;
  for (int o : action.list_) {
    if (!prev.state.deck[o].clued) {
      all_prev_clued = false;
      break;
    }
  }
  if (all_prev_clued) return std::nullopt;

  if (!game.in_endgame() &&
      state.rem_score() > static_cast<int>(state.variant->suits.size())) {
    return std::nullopt;
  }
  auto focus_id = state.deck[focus].id();
  if (focus_id && state.is_basic_trash(*focus_id)) return std::nullopt;

  IdentitySet poss;
  if (action.clue.kind == ClueKind::COLOUR) {
    poss = thought.possible;
  } else {
    int r = action.clue.value;
    poss = thought.possible.filter([&](Identity i) { return i.rank == r; });
  }

  IdentitySet useful;
  for (Identity id : poss) {
    if (state.is_basic_trash(id)) continue;
    bool duplicated = false;
    for (int i = 0; i < state.num_players; ++i) {
      if (i == action.target) continue;
      for (int o : state.hands[i]) {
        if (game.is_touched(o) && game.order_matches(o, id, /*infer=*/true)) {
          duplicated = true;
          break;
        }
      }
      if (duplicated) break;
    }
    if (duplicated) {
      useful = useful.add(id);
    } else {
      return std::nullopt;
    }
  }
  return useful.non_empty() ? std::optional<IdentitySet>{useful} : std::nullopt;
}

bool rainbow_mismatch(const Game& game, const ClueAction& action, Identity id,
                       int prompt) {
  const State& state = game.state;
  const int target = action.target;
  const auto& list_ = action.list_;
  const BaseClue clue = action.clue;

  if (clue.kind != ClueKind::COLOUR) return false;
  if (!state.variant->suits[id.suit_index].suit_type.rainbowish) return false;
  if (game.known_as(prompt, "Rainbow") || game.known_as(prompt, "Omni")) return false;
  for (const auto& c : state.deck[prompt].clues) {
    if (c.kind == clue.kind && c.value == clue.value) return false;
  }

  bool all_rainbow = true;
  if (target == state.our_player_index) {
    for (int o : list_) {
      bool ok = game.me().thoughts[o].possible.forall([&](Identity c2) {
        return state.variant->suits[c2.suit_index].suit_type.rainbowish;
      });
      if (!ok) {
        all_rainbow = false;
        break;
      }
    }
  } else {
    for (int o : list_) {
      if (!state.variant->suits[state.deck[o].suit_index].suit_type.rainbowish) {
        all_rainbow = false;
        break;
      }
    }
  }
  if (!all_rainbow) return false;

  for (const auto& c : state.deck[prompt].clues) {
    auto touched = state.clue_touched(state.hands[target], c.kind, c.value);
    auto sorted_a = touched;
    auto sorted_b = list_;
    std::sort(sorted_a.begin(), sorted_a.end());
    std::sort(sorted_b.begin(), sorted_b.end());
    if (sorted_a == sorted_b) return true;
  }
  return false;
}

std::vector<int> connectable_simple(const Game& game, const Player& player,
                                       int start, int target,
                                       std::optional<Identity> id) {
  const State& state = game.state;

  if (id && state.is_playable(*id)) return {99};
  if (start == target) return player.obvious_playables(game, target);
  if (state.ended()) return {};

  int next_player_index = state.next_player_index(start);
  auto playables = player.obvious_playables(game, start);

  for (int order : playables) {
    auto play_id = player.thoughts[order].id(/*infer=*/true);
    if (!play_id) continue;
    Game new_game = game;
    if (new_game.state.current_player_index != start) {
      new_game = new_game.simulate_action(TurnAction{state.turn_count, start});
    }
    new_game = new_game.simulate_action(
        PlayAction{start, order, play_id->suit_index, play_id->rank});
    auto result = connectable_simple(new_game, player, next_player_index, target, id);
    if (!result.empty()) return result;
  }

  return connectable_simple(game, player, next_player_index, target, id);
}

}  // namespace hanabi
