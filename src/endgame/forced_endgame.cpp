#include "hanabi/endgame/forced_endgame.h"

#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

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
// current player (CP) holds at least two cards whose common-knowledge
// inference is a singleton critical identity, with at least one of those
// also currently playable.
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
std::optional<PerformAction> two_critical_play_action(const Game& game) {
  const State& s = game.state;
  int cp = s.current_player_index;

  int critical_count = 0;
  std::optional<int> playable_order;
  for (int o : s.hands[cp]) {
    const IdentitySet& inf = game.common.thoughts[o].inferred;
    if (inf.length() != 1) continue;
    Identity id = inf.head();
    if (!s.is_critical(id)) continue;
    ++critical_count;
    if (!playable_order && s.is_playable(id)) playable_order = o;
  }

  if (critical_count < 2 || !playable_order) return std::nullopt;
  return PerformAction{PerformPlay{*playable_order}};
}

// Fallback: enumerate every (target, kind, value) triple and return the
// first one that legally touches at least one card in the target's hand.
// Used only if `Game::find_all_clues` returns empty (very rare — would
// mean every valid clue is a mistake or perfectly redundant; we still
// must give *something* since the rule forbids play/discard).
std::optional<PerformAction> any_legal_clue(const Game& game) {
  const State& s = game.state;
  int cp = s.current_player_index;
  for (int target = 0; target < s.num_players; ++target) {
    if (target == cp) continue;
    for (int v = 0; v < static_cast<int>(s.variant->suits.size()); ++v) {
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
