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

  // Lock slot = rightmost (oldest, last vector index) unclued card before the
  // clue lands. Note this is NOT the chop, which is the *newest* unclued card.
  std::optional<int> lock_slot;
  for (auto it = state.hands[action.target].rbegin();
       it != state.hands[action.target].rend(); ++it) {
    if (!state.deck[*it].clued) { lock_slot = *it; break; }
  }
  if (!lock_slot) return false;

  // Promise only applies when the lock slot itself is touched.
  if (!contains(action.list_, *lock_slot)) return false;

  // Promised ranks (set, to handle pink_s substitutions where a clue can
  // legitimately call either the spoken rank or the special rank).
  std::vector<int> promised{action.clue.value};
  if (v.pink_s && v.special_rank) {
    int sr = *v.special_rank;
    if (sr == 5 && action.clue.value == 4) promised.push_back(5);
    else if (sr == 1 && action.clue.value == 2) promised.push_back(1);
  }

  auto lock_slot_id = state.deck[*lock_slot].id();
  if (!lock_slot_id) return false;  // observer's own hand — can't verify
  return std::find(promised.begin(), promised.end(), lock_slot_id->rank) ==
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
  // Leftmost = slot 1 = the NEWEST card = the HIGHEST order, so this is
  // `max_element`. It was `min_element` — the oldest, i.e. the rightmost —
  // which contradicted this function's own header contract ("the leftmost
  // newly-touched unclued card"), reactor0 CONVENTION.md §1c, and reactor's.
  // It only became visible once reactor0's rank classification was fixed and
  // priority 1 started firing in omni variants at all (bug_report_1.txt 1.2
  // and 1.3, which both expect the LEFT card to be called).
  //
  // Note `touched_unclued` is built with the same predicate as
  // `newly_touched`, so the two are always equal and the fallback below is
  // unreachable; it is kept only as a guard against an empty range.
  return !touched_unclued.empty()
             ? *std::max_element(touched_unclued.begin(), touched_unclued.end())
             : *std::max_element(newly_touched.begin(), newly_touched.end());
}

}  // namespace hanabi::reactor::variants
