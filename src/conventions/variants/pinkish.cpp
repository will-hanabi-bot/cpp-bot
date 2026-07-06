#include "hanabi/conventions/variants/pinkish.h"

#include <algorithm>

#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

namespace hanabi::reactor::variants {

namespace {

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

}  // namespace

bool violates_pink_promise(const Game& prev, const ClueAction& action) {
  if (action.clue.kind != ClueKind::RANK) return false;

  const State& state = prev.state;
  const Variant& v = *state.variant;

  bool pinkish = v.pink_s;
  for (const Suit& s : v.suits) {
    if (s.suit_type.pinkish) { pinkish = true; break; }
  }
  if (!pinkish) return false;

  // Chop = rightmost (oldest, last vector index) unclued card before the
  // clue lands.
  std::optional<int> chop;
  for (auto it = state.hands[action.target].rbegin();
       it != state.hands[action.target].rend(); ++it) {
    if (!state.deck[*it].clued) { chop = *it; break; }
  }
  if (!chop) return false;

  // Promise only applies when the chop itself is touched.
  if (!contains(action.list_, *chop)) return false;

  // Promised ranks (set, to handle pink_s substitutions where a clue can
  // legitimately call either the spoken rank or the special rank).
  std::vector<int> promised{action.clue.value};
  if (v.pink_s && v.special_rank) {
    int sr = *v.special_rank;
    if (sr == 5 && action.clue.value == 4) promised.push_back(5);
    else if (sr == 1 && action.clue.value == 2) promised.push_back(1);
  }

  auto chop_id = state.deck[*chop].id();
  if (!chop_id) return false;  // observer's own hand — can't verify
  return std::find(promised.begin(), promised.end(), chop_id->rank) ==
         promised.end();
}

bool apply_rank_promise(Game& game, int order, const BaseClue& clue) {
  const State& state = game.state;
  int rv = clue.value;
  game.with_thought(order, [rv](const Thought& t) {
    Thought out = t;
    out.inferred = t.inferred.filter([rv](Identity i) { return i.rank == rv; });
    return out;
  });
  game.with_meta(order, [](ConvData& m) { m.focused = true; });
  auto id = state.deck[order].id();
  return !(id && id->rank != clue.value);
}

int playable_rank_focus(const Game& prev, const State& state,
                        const ClueAction& action,
                        const std::vector<int>& newly_touched) {
  std::vector<int> touched_unclued;
  for (int o : state.hands[action.target]) {
    if (!prev.state.deck[o].clued && contains(action.list_, o)) {
      touched_unclued.push_back(o);
    }
  }
  return !touched_unclued.empty()
             ? *std::min_element(touched_unclued.begin(), touched_unclued.end())
             : *std::max_element(newly_touched.begin(), newly_touched.end());
}

}  // namespace hanabi::reactor::variants
