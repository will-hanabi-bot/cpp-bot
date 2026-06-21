#include "hanabi/endgame/forced_endgame.h"

#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::endgame {

namespace {

// Rule 1 — "5-lockout".
//
// Precondition: `cards_left == 1`, `play_stacks[suit] < 4`, and the suit's
// 5 still exists in some hand.
//
// Cycle offsets (relative to current player): `offset(p) = (p - cp + n) %
// n`. CP itself has offset 0 (their current turn is "now"); after CP plays
// the deck empties and the remaining cycle is offsets `1..n-1`.
//
// Rule fires for suit `S` iff every 4-holder has offset >= the 5-holder's
// offset. Same-hand counts (>=), CP-is-5-holder counts (5-offset = 0,
// no 4-holder can have a smaller non-negative offset), and the "5-holder
// strictly precedes all 4-holders" case all fall under that single
// predicate. When CP themselves holds a 4 (offset 0), the rule does NOT
// fire — playing the 4 advances the stack and the 5 plays normally on
// its endgame turn.
bool five_lockout_fires(const Game& game, int suit) {
  const State& s = game.state;
  if (s.play_stacks[suit] >= 4) return false;

  Identity five{suit, 5};
  Identity four{suit, 4};

  std::optional<int> five_holder;
  std::vector<int> four_holders;
  for (int p = 0; p < s.num_players; ++p) {
    for (int o : s.hands[p]) {
      auto id = s.deck[o].id();
      if (!id) continue;
      if (*id == five) five_holder = p;
      if (*id == four) four_holders.push_back(p);
    }
  }
  if (!five_holder) return false;  // 5 already discarded — rule N/A.

  int cp = s.current_player_index;
  int n = s.num_players;
  auto offset = [&](int p) { return (p - cp + n) % n; };

  int five_offset = offset(*five_holder);
  for (int fh : four_holders) {
    if (offset(fh) < five_offset) return false;
  }
  return true;
}

// Rule 2 — "two-critical play".
//
// Precondition: `cards_left == 1`, `clue_tokens < num_players`, and the
// current player (CP) holds at least two singleton-critical cards in
// hand, with at least one of those also currently playable.
//
// Why play is forced. With `cards_left == 1`, CP has exactly two play
// turns remaining if they play now (this turn + the final-round turn that
// comes back around after the deck empties). Cluing or discarding burns
// one of those two play opportunities. The `clue_tokens < num_players`
// guard rules out the "team cycles clues to keep the deck at 1" stall —
// at least one of the next `n` turns must be a play or discard, which
// empties the deck before CP recovers the play they skipped. With two
// strictly critical cards in hand (each only one copy left in the game),
// every skipped play turn costs one of them permanently.
//
// Singleton check uses `common.thoughts[o].inferred ∩ players[cp]
// .thoughts[o].inferred`. Common alone can include basic-trash that
// good-touch hasn't stripped (`game.good_touch` stays false outside
// specific code paths). Per-player alone can be wider than common in
// test setups that seed common.thoughts directly without syncing
// per-player views (`fully_known` in test_forced_endgame.cpp). The
// intersection is the tight set both agree on. Replay 1899527 T47 —
// common inferred for will-bot69's slot 4 = {y1, y2, y4} (the rank-3-
// stack basic-trash y1/y2 aren't elim'd from common); per-player narrows
// to {y4} via good-touch / visibility. Intersection = {y4} singleton.
//
// Play-target tiebreaker. With multiple playable criticals, prefer the
// one whose play unblocks another critical play. The successor identity
// (rank+1, or rank-1 for reversed suits) is "unblocked" if another
// player holds it. Replay 1899527 T47 — slot 1 = r5 (critical playable,
// nothing in line after), slot 4 = y4 (critical playable, will-bot67
// holds y5 next-up). Playing r5 first leaves will-bot67 unable to play
// y5 in their endgame turn (still needs y4 to land first), losing y5
// permanently. Playing y4 first means y5 becomes playable, will-bot67
// plays y5 on their final-round turn, CP plays r5 on their final-round
// turn = full score.
std::optional<PerformAction> two_critical_play_action(const Game& game) {
  const State& s = game.state;
  int cp = s.current_player_index;
  const Player& me = game.players[cp];

  std::vector<std::pair<int, Identity>> singleton_critical;
  for (int o : s.hands[cp]) {
    IdentitySet tight = game.common.thoughts[o].inferred.intersect(
        me.thoughts[o].inferred);
    if (tight.length() != 1) continue;
    Identity id = tight.head();
    if (!s.is_critical(id)) continue;
    singleton_critical.push_back({o, id});
  }
  if (singleton_critical.size() < 2) return std::nullopt;

  std::vector<std::pair<int, Identity>> playable;
  for (const auto& [o, id] : singleton_critical) {
    if (s.is_playable(id)) playable.push_back({o, id});
  }
  if (playable.empty()) return std::nullopt;

  // Score each playable critical by whether its play unblocks a
  // successor held by another player. The successor identity for a
  // suit's reversed direction is rank-1 (Identity::prev); for normal
  // suits it's rank+1 (Identity::next). The unblock bonus only counts
  // when the successor itself is critical or useful (i.e., not already
  // basic-trash), since unblocking a trash card means nothing.
  auto successor = [&](Identity id) -> std::optional<Identity> {
    const auto& st = s.variant->suits[id.suit_index].suit_type;
    return st.reversed ? id.prev() : id.next();
  };
  auto unblock_score = [&](Identity id) -> int {
    auto succ = successor(id);
    if (!succ) return 0;
    if (s.is_basic_trash(*succ)) return 0;
    for (int p = 0; p < s.num_players; ++p) {
      if (p == cp) continue;
      for (int o : s.hands[p]) {
        auto deck_id = s.deck[o].id();
        if (deck_id && *deck_id == *succ) return 1;
      }
    }
    return 0;
  };

  auto best = playable.front();
  int best_score = unblock_score(best.second);
  for (size_t i = 1; i < playable.size(); ++i) {
    int score = unblock_score(playable[i].second);
    if (score > best_score) {
      best = playable[i];
      best_score = score;
    }
  }
  return PerformAction{PerformPlay{best.first}};
}

// Fallback: enumerate every (target, kind, value) triple and return the
// first one that legally touches at least one card in the target's hand.
// Used only if `Game::find_all_clues` returns empty (very rare — would
// mean every valid clue is a mistake or perfectly redundant; we still
// must give *something* since the rule forbids play/discard).
//
// Colour values iterate `colourable_suit_indices.size()`, not `suits
// .size()`. In Ambiguous variants multiple suits share a single clue
// colour (e.g. Tomato + Mahogany both clue with Red), so the count of
// valid colour clue values is the count of distinct clue colours.
// Mirrors the loop bound in `State::all_colour_clues` / `all_valid
// _clues` (src/basics/state.cpp). Surfaced as a server-side rejection
// for "Ambiguous (6 Suits)" (6 suits, 3 colours): the bot tried to
// send colour value 3 and the server warned
// "You cannot give a color clue with a value of \"3\".".
std::optional<PerformAction> any_legal_clue(const Game& game) {
  const State& s = game.state;
  int cp = s.current_player_index;
  for (int target = 0; target < s.num_players; ++target) {
    if (target == cp) continue;
    const int num_colours =
        static_cast<int>(s.variant->colourable_suit_indices.size());
    for (int v = 0; v < num_colours; ++v) {
      auto touched =
          s.clue_touched(s.hands[target], ClueKind::COLOUR, v);
      if (!touched.empty()) return PerformAction{PerformColour{target, v}};
    }
    for (int v = 1; v <= 5; ++v) {
      auto touched = s.clue_touched(s.hands[target], ClueKind::RANK, v);
      if (!touched.empty()) return PerformAction{PerformRank{target, v}};
    }
  }
  return std::nullopt;
}

}  // namespace

std::optional<PerformAction> forced_endgame_action(const Game& game) {
  hanabi::instr::ScopedTimer st("endgame.forced_endgame_action");
  hanabi::logging::LogScope ls("endgame.forced_endgame_action");
  const State& s = game.state;
  if (s.cards_left != 1) return std::nullopt;

  // Rule 2 takes precedence over Rule 1: when both fire (e.g., CP holds the
  // suit's 4 and 5, both critical, 4 playable), playing the playable
  // critical card is the concrete winning move; the 5-lockout's "clue to
  // delay" would skip a play turn and lose a critical.
  if (s.clue_tokens < s.num_players) {
    if (auto a = two_critical_play_action(game)) return a;
  }

  if (s.clue_tokens == 0) return std::nullopt;

  bool any_lockout = false;
  for (int suit = 0; suit < static_cast<int>(s.variant->suits.size()); ++suit) {
    if (five_lockout_fires(game, suit)) {
      any_lockout = true;
      break;
    }
  }
  if (!any_lockout) return std::nullopt;

  auto clues = game.find_all_clues(s.current_player_index);
  if (!clues.empty()) return clues.front();
  return any_legal_clue(game);
}

}  // namespace hanabi::endgame
