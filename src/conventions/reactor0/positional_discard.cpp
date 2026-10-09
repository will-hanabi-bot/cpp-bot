#include "hanabi/conventions/reactor0/positional_discard.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/convention.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/endgame/forced_endgame.h"
#include "hanabi/endgame/helper.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor0 {

namespace {

bool inverted(const State& s, Identity i) {
  return s.variant->suits[i.suit_index].suit_type.inverted;
}

// A card of `player`'s that the TEAM can see certainly advances a stack: every
// reading of it, as the common knowledge holds it, plays (or, on an inverted suit,
// chucks) on the shared stacks. The test Bob makes of Alice, so Alice makes it of
// herself too.
bool common_certain_play(const Game& game, int player) {
  const State& s = game.state;
  const State shared = s.shared_view();
  for (int o : s.hands[player]) {
    const IdentitySet live = game.common.thoughts[o].possibilities();
    if (live.is_empty()) continue;
    const bool plays =
        live.forall([&](Identity i) { return !inverted(s, i) && shared.is_playable(i); });
    const bool chucks = s.clue_tokens < 8 && live.forall([&](Identity i) {
      return inverted(s, i) && shared.is_playable(i);
    });
    if (plays || chucks) return true;
  }
  return false;
}

// Bob's cards we can see play now, as slot indices (0-based, slot 1 first).
std::vector<int> bob_playable_slots(const Game& game, int bob) {
  const State& s = game.state;
  std::vector<int> out;
  for (std::size_t k = 0; k < s.hands[bob].size(); ++k) {
    const auto id = s.deck[s.hands[bob][k]].id();
    if (!id || inverted(s, *id) || !s.is_playable(*id)) continue;
    out.push_back(static_cast<int>(k));
  }
  return out;
}

// A seat still to act after Bob in this final round holds the card after `id`.
bool unblocks_a_later_seat(const State& s, int bob, Identity id) {
  if (!s.endgame_turns) return false;
  const auto next = id.next();
  if (!next) return false;
  for (int i = 2; i < *s.endgame_turns; ++i) {
    const int seat = (s.current_player_index + i) % s.num_players;
    if (seat == bob) continue;
    for (int o : s.hands[seat]) {
      const auto seen = s.deck[o].id();
      if (seen && *seen == *next) return true;
    }
  }
  return false;
}

// A stall: a legal clue to a seat other than Bob where there is one, so that it
// cannot read as a play clue on his last turn.
std::optional<PerformAction> stall_clue(const Game& game, int bob) {
  const State& s = game.state;
  if (!s.can_clue()) return std::nullopt;
  std::optional<PerformAction> to_bob;
  for (int i = 1; i < s.num_players; ++i) {
    const int target = (s.our_player_index + s.num_players - i) % s.num_players;
    for (const Clue& clue : s.all_valid_clues(target)) {
      const PerformAction a = clue.kind == ClueKind::COLOUR
                                  ? PerformAction{PerformColour{clue.target, clue.value}}
                                  : PerformAction{PerformRank{clue.target, clue.value}};
      if (target != bob) return a;
      if (!to_bob) to_bob = a;
    }
  }
  return to_bob;
}

// Bob already knows this card is a play, by GLOBAL EMPATHY: every reading of it
// the team holds plays on the shared stacks. Then Alice need not name it (the
// user's exception, v23.2.0).
bool bob_knows_play(const Game& game, int order) {
  const State& s = game.state;
  const IdentitySet live = game.common.thoughts[order].possibilities();
  const State shared = s.shared_view();
  return live.non_empty() &&
         live.forall([&](Identity i) { return !inverted(s, i) && shared.is_playable(i); });
}

}  // namespace

bool positional_position(const Game& game, int alice) {
  const State& s = game.state;
  if (!is_reactor0_family(game.convention)) return false;
  if (s.cards_left != 0) return false;
  // Alice's own turn counts in `endgame_turns`, so Bob still acts when it is 2+.
  if (!s.endgame_turns || *s.endgame_turns < 2) return false;
  if (alice < 0 || alice >= s.num_players) return false;
  // A reaction Alice owes is read from the slot she actions, so that is what her
  // discard says -- it is not positional, and as Alice we answer the reaction.
  if (!game.waiting.empty() && game.waiting.front().reacter == alice) return false;
  for (const auto& pr : game.pending_reactions) {
    if (pr && pr->reacter == alice) return false;
  }
  return !common_certain_play(game, alice);
}

std::optional<PerformAction> positional_discard_signal(const Game& game) {
  const State& s = game.state;
  const int us = s.our_player_index;
  if (s.current_player_index != us || !positional_position(game, us)) return std::nullopt;
  if (!hanabi::endgame::certain_plays(game).empty()) return std::nullopt;
  if (s.clue_tokens >= 8) return std::nullopt;  // a discard is illegal here
  const int bob = s.next_player_index(us);
  std::optional<int> best;
  bool best_unblocks = false;
  for (int k : bob_playable_slots(game, bob)) {
    if (k >= static_cast<int>(s.hands[us].size())) continue;  // no slot of ours to name it
    if (bob_knows_play(game, s.hands[bob][k])) continue;  // nothing to tell him
    const bool unblocks =
        unblocks_a_later_seat(s, bob, *s.deck[s.hands[bob][k]].id());
    if (!best || (unblocks && !best_unblocks)) {
      best = k;
      best_unblocks = unblocks;
    }
  }
  if (!best) return std::nullopt;
  hanabi::logging::log_branch("reactor0.positional_discard_given",
                              {{"slot", *best + 1}, {"bob_order", s.hands[bob][*best]}});
  return PerformAction{PerformDiscard{s.hands[us][*best]}};
}

std::optional<PerformAction> positional_gamble(const Game& game) {
  const State& s = game.state;
  const int us = s.our_player_index;
  if (s.current_player_index != us || !positional_position(game, us)) return std::nullopt;
  if (!hanabi::endgame::certain_plays(game).empty()) return std::nullopt;
  const int bob = s.next_player_index(us);
  if (!bob_playable_slots(game, bob).empty()) return std::nullopt;  // Bob has a play
  // The play is ours. A gamble that presses Discard (an inverted chuck) would read
  // as positional, so only the Play button is bet on.
  auto is_play = [](const std::optional<PerformAction>& a) {
    return a && std::holds_alternative<PerformPlay>(*a);
  };
  auto a = hanabi::endgame::required_play_action(game, /*narrow=*/false);
  if (!is_play(a)) {
    a = hanabi::endgame::gamble_on(
        game, s.playable_set.filter([&](Identity i) { return !inverted(s, i); }));
  }
  if (!is_play(a)) return std::nullopt;
  hanabi::logging::log_branch("reactor0.positional_gamble",
                              {{"order", std::get<PerformPlay>(*a).target}});
  return a;
}

PerformAction positional_guard(const Game& game, const PerformAction& chosen) {
  const State& s = game.state;
  const auto* d = std::get_if<PerformDiscard>(&chosen);
  if (!d) return chosen;
  const int us = s.our_player_index;
  if (s.current_player_index != us || !positional_position(game, us)) return chosen;
  if (auto sig = positional_discard_signal(game); sig && *sig == chosen) return chosen;
  const int bob = s.next_player_index(us);
  // A play Bob already knows (the exception): Alice was free not to name it, but a
  // discard is read all the same, so the one she makes names it. Self-play seed
  // 117 T61 (Black): at 0 clues our slot-5 discard sent Bob's last turn to his p1
  // instead of the r5 he knew.
  for (int k : bob_playable_slots(game, bob)) {
    if (k >= static_cast<int>(s.hands[us].size())) continue;
    const PerformAction named{PerformDiscard{s.hands[us][k]}};
    if (named == chosen) return chosen;
    hanabi::logging::log_branch("reactor0.positional_guard",
                                {{"replaced", d->target}, {"by", s.hands[us][k]}});
    return named;
  }
  if (auto clue = stall_clue(game, bob)) {
    hanabi::logging::log_branch("reactor0.positional_guard", {{"replaced", d->target},
                                                             {"by", "stall_clue"}});
    return *clue;
  }
  // A slot Bob does not hold names nothing.
  const auto& ours = s.hands[us];
  for (std::size_t k = s.hands[bob].size(); k < ours.size(); ++k) {
    hanabi::logging::log_branch("reactor0.positional_guard",
                                {{"replaced", d->target}, {"by", ours[k]}});
    return PerformAction{PerformDiscard{ours[k]}};
  }
  return chosen;
}

std::optional<PerformAction> positional_play(const Game& game) {
  const State& s = game.state;
  if (s.cards_left != 0) return std::nullopt;
  for (int o : s.our_hand()) {
    if (game.meta[o].positional_play) return PerformAction{PerformPlay{o}};
  }
  return std::nullopt;
}

void read_positional_discard(const Game& prev, Game& game, const DiscardAction& action) {
  if (action.failed) return;  // a misplay pressed Play
  const State& ps = prev.state;
  const int alice = action.player_index_v;
  if (!positional_position(prev, alice)) return;
  // A discard that throws a card called to discard says what that call says (one
  // that answers a reaction is ruled out with the position).
  if (action.order < static_cast<int>(prev.meta.size()) &&
      prev.meta[action.order].status == CardStatus::CALLED_TO_DISCARD) {
    return;
  }
  const auto& hand = ps.hands[alice];
  const auto it = std::find(hand.begin(), hand.end(), action.order);
  if (it == hand.end()) return;
  const std::size_t slot = static_cast<std::size_t>(it - hand.begin());
  const int bob = ps.next_player_index(alice);
  if (slot >= ps.hands[bob].size()) return;  // names nothing
  const int card = ps.hands[bob][slot];
  game.with_meta(card, [](ConvData& m) {
    m.positional_play = true;
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
  });
  // What it can be: a playable, where its reading allows one.
  const IdentitySet live = game.common.thoughts[card].possibilities();
  const State shared = game.state.shared_view();
  const IdentitySet playable =
      live.filter([&](Identity i) { return !inverted(ps, i) && shared.is_playable(i); });
  if (playable.non_empty()) game.narrow_thought(card, playable);
  hanabi::logging::log_branch("reactor0.positional_discard_read",
                              {{"slot", static_cast<int>(slot) + 1}, {"order", card}});
}

}  // namespace hanabi::reactor0
