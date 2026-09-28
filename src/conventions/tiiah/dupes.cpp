#include "hanabi/conventions/tiiah/dupes.h"

#include <algorithm>
#include <variant>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"

namespace hanabi::tiiah {

namespace {

std::optional<Identity> only_one(const IdentitySet& set) {
  if (set.length() != 1) return std::nullopt;
  return set.head();
}

// What we can name our own card as: our belief, and failing that the team's.
std::optional<Identity> our_name(const Game& game, int order) {
  if (auto id = only_one(game.me().thoughts[order].possibilities())) return id;
  return only_one(game.common.thoughts[order].possibilities());
}

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

// A called card of seat `p` that really is `id` (we can see it) while the team's
// reading of it is not exactly `id`: the card the passback protects.
std::optional<int> unnamed_called_copy(const Game& game, int p, Identity id) {
  const State& s = game.state;
  for (int o : s.hands[p]) {
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    const auto seen = s.deck[o].id();
    if (!seen || *seen != id) continue;
    const IdentitySet reading = game.common.thoughts[o].possibilities();
    if (!reading.contains(id) || reading.length() <= 1) continue;
    // ...and whose holder would still PLAY it once our copy lands: the strike the
    // passback exists to prevent. Judged on the stacks we share with the holder,
    // with our X on them. Replay 2010296 T9: the other copy read {y2, p1}, and
    // with yellow on 2 the y2 is trash, so once our p1 landed its holder would
    // throw it away -- nothing to prevent, and our p1 is simply played.
    State after = s.with_stacks(s.stacks_known_to_both(s.our_player_index, p));
    if (after.is_playable(id)) after = after.with_play(id);
    const bool would_play = reading.difference(id).exists(
        [&after](Identity i) { return after.is_playable(i); });
    if (would_play) return o;
  }
  return std::nullopt;
}

}  // namespace

std::optional<int> dupe_passback(const Game& game) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return std::nullopt;
  if (s.clue_tokens >= 8) return std::nullopt;  // a discard is illegal
  const int me = s.our_player_index;
  for (int o : s.hands[me]) {
    // Only a card we are about to PLAY: one called to play, or one we can name as
    // playable.
    const auto x = our_name(game, o);
    if (!x) continue;
    const bool called = game.meta[o].status == CardStatus::CALLED_TO_PLAY;
    if (!called && !s.is_playable(*x)) continue;
    for (int p = 0; p < s.num_players; ++p) {
      if (p == me) continue;
      if (unnamed_called_copy(game, p, *x)) return o;
    }
  }
  return std::nullopt;
}

bool read_passback(Game& game, const Action& raw) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return false;
  const auto* dc = std::get_if<DiscardAction>(&raw);
  if (!dc || dc->failed) return false;
  const int discarder = dc->player_index_v;
  const int order = dc->order;
  if (order < 0 || order >= static_cast<int>(game.common.thoughts.size())) return false;
  // What the discard shows: a discard reaches the pile, so every seat can name it.
  std::optional<Identity> x;
  if (dc->suit_index != -1 && dc->rank != -1) {
    x = Identity{dc->suit_index, dc->rank};
  } else {
    x = s.deck[order].id();
  }
  if (!x) return false;
  // The discarder must have been able to NAME it: a passback is a deliberate
  // throw of a known copy, not a chop.
  const auto known = game.common.thoughts[order].id(/*infer=*/true, /*symmetric=*/true);
  if (!known || *known != *x) return false;

  bool applied = false;
  for (int p = 0; p < s.num_players; ++p) {
    if (p == discarder) continue;
    for (int o : s.hands[p]) {
      if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      const IdentitySet reading = game.common.thoughts[o].possibilities();
      if (!reading.contains(*x) || reading.length() <= 1) continue;
      game.narrow_thought(o, IdentitySet::single(*x));
      applied = true;
    }
  }
  return applied;
}

std::optional<int> discharge_hole_card(const Game& game, const ReactorWC& wc,
                                       Identity id) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return std::nullopt;
  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    if (s.holder_of(o) != wc.giver) continue;
    if (contains(s.hands[wc.giver], o)) continue;  // still in hand, not in the hole
    const ConvData& m = game.meta[o];
    const IdentitySet& shared = m.shared_left.non_empty() ? m.shared_left : m.superposition;
    if (shared.length() > 1 && shared.contains(id)) return o;
  }
  return std::nullopt;
}

std::optional<int> find_discharge(const Game& game, const Action& raw) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return std::nullopt;
  const auto* dc = std::get_if<DiscardAction>(&raw);
  if (!dc || dc->failed || game.waiting.empty()) return std::nullopt;
  const ReactorWC& wc = game.waiting.front();
  if (dc->player_index_v != wc.reacter) return std::nullopt;
  std::optional<Identity> x;
  if (dc->suit_index != -1 && dc->rank != -1) {
    x = Identity{dc->suit_index, dc->rank};
  } else if (dc->order >= 0 && dc->order < static_cast<int>(s.deck.size())) {
    x = s.deck[dc->order].id();
  }
  if (!x) return std::nullopt;
  return discharge_hole_card(game, wc, *x);
}

bool discharge_instead(const Game& game, int order) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return false;
  if (s.clue_tokens >= 8) return false;  // a discard is illegal
  if (game.waiting.empty()) return false;
  const ReactorWC& wc = game.waiting.front();
  if (wc.reacter != s.our_player_index) return false;
  const auto x = our_name(game, order);
  if (!x) return false;
  if (!s.is_basic_trash(*x)) return false;  // not already down on our stacks
  return discharge_hole_card(game, wc, *x).has_value();
}

}  // namespace hanabi::tiiah
