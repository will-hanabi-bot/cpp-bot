// Port of python-bot/src/hanabi_bot/basics/state.py.
// Original Scala: scala-bot/src/scala_bot/basics/State.scala.
//
// State is the public game-state shared by every observer. Value type;
// `with_*` methods return a new State (copy-on-write). For the endgame
// solver hot path, State will be mutated in place via simulate_action_inplace
// (Phase 5) — that's a deliberate departure from the immutable shape.
#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/variant.h"

namespace hanabi {

struct Thought;

// Hand size by num_players (index = num_players). Indices 0,1 unused.
// Matches scala-bot/.../Game.scala line 14.
inline constexpr std::array<int, 7> kHandSize = {0, 0, 5, 5, 4, 4, 3};

struct State {
  // Pointed-to variant lives in the load_variants() cache; State doesn't own it.
  const Variant* variant = nullptr;
  TableOptions options;

  int num_players = 0;
  std::vector<std::string> names;
  int our_player_index = 0;

  int cards_left = 0;
  int cards_total = 0;

  std::vector<int> play_stacks;
  // discard_stacks[suit][rank-1] = orders of cards discarded for that identity,
  // newest first (matching Scala's `order +: list` cons).
  std::vector<std::array<std::vector<int>, 5>> discard_stacks;
  std::vector<int> max_ranks;
  // base_count[ord] = number of physical copies known unavailable (discarded,
  // misplayed, played) for that identity.
  std::vector<int> base_count;

  IdentitySet all_ids;
  IdentitySet playable_set;
  IdentitySet critical_set;
  IdentitySet trash_set;
  // card_count[ord] = total copies in the deck.
  std::vector<int> card_count;

  std::vector<std::vector<int>> hands;   // hands[player] = orders
  std::vector<Card> deck;
  std::vector<int> holders;              // holders[order] = player_index that drew it

  int turn_count = 0;
  int clue_tokens = 8;
  bool half_clue_token = false;
  int strikes = 0;
  std::optional<int> endgame_turns;
  int next_card_order = 0;

  std::vector<std::vector<Action>> action_list;
  int current_player_index = 0;

  // --- Factory ---
  static State create(std::vector<std::string> names, int our_player_index,
                       const Variant& variant, TableOptions options);

  // --- Mutators (return new State) ---
  State with_discard(Identity id, int order) const;
  State with_play(Identity id) const;
  State try_play(Identity id) const;
  State regain_clue() const;

  // --- Pure helpers ---
  bool ended() const;
  int score() const;
  int max_score() const;
  int rem_score() const { return max_score() - score(); }
  int pace() const { return score() + cards_left + num_players - max_score(); }

  int last_player_index(int player_index) const {
    return (player_index + num_players - 1) % num_players;
  }
  int next_player_index(int player_index) const {
    return (player_index + 1) % num_players;
  }

  // Reversed-direction semantics: a reversed suit's stack starts at 6
  // (sentinel meaning "nothing played, next playable = 5") and decreases
  // as cards play 5 → 4 → 3 → 2 → 1. `max_ranks[suit]` for reversed
  // stores the *lowest* still-achievable rank (rises as low-rank
  // criticals are discarded).
  bool is_basic_trash(Identity id) const {
    const auto& st = variant->suits[id.suit_index].suit_type;
    if (st.reversed) {
      return id.rank >= play_stacks[id.suit_index] || id.rank < max_ranks[id.suit_index];
    }
    return id.rank <= play_stacks[id.suit_index] || id.rank > max_ranks[id.suit_index];
  }
  bool is_useful(Identity id) const {
    const auto& st = variant->suits[id.suit_index].suit_type;
    if (st.reversed) {
      return id.rank < play_stacks[id.suit_index] && id.rank >= max_ranks[id.suit_index];
    }
    return id.rank > play_stacks[id.suit_index] && id.rank <= max_ranks[id.suit_index];
  }
  int playable_away(Identity id) const {
    const auto& st = variant->suits[id.suit_index].suit_type;
    if (st.reversed) {
      return (play_stacks[id.suit_index] - 1) - id.rank;
    }
    return id.rank - (play_stacks[id.suit_index] + 1);
  }
  bool is_playable(Identity id) const { return playable_away(id) == 0; }
  bool is_critical(Identity id) const;

  // Per-suit "cards played so far" — for both normal (stack 0 → 5
  // counts directly) and reversed (stack 6 → 1, count = 6 − stack).
  int played_count(int suit_index) const {
    const auto& st = variant->suits[suit_index].suit_type;
    if (st.reversed) return 6 - play_stacks[suit_index];
    return play_stacks[suit_index];
  }
  // Per-suit "max cards reachable" given current discards.
  int max_played(int suit_index) const {
    const auto& st = variant->suits[suit_index].suit_type;
    if (st.reversed) return 6 - max_ranks[suit_index];
    return max_ranks[suit_index];
  }

  const std::vector<int>& our_hand() const { return hands[our_player_index]; }
  bool can_clue() const { return clue_tokens > 0; }
  int holder_of(int order) const;
  bool in_starting_hand(int order) const {
    return order < num_players * kHandSize[num_players];
  }
  int multiplicity(IdentitySet ids) const;

  bool has_consistent_infs(const Thought& thought) const;

  std::vector<int> clue_touched(const std::vector<int>& orders,
                                 ClueKind kind, int value) const;
  std::vector<Clue> all_colour_clues(int target) const;
  std::vector<Clue> all_valid_clues(int target) const;

  // Substring match on any suit name. The Python version uses re.Pattern;
  // for now we expose simple-substring search (sufficient for current call
  // sites in basics/conventions; revisit if a real regex is required).
  bool includes_variant(std::string_view needle) const;

  Identity expand_short(std::string_view short_) const;
  std::string log_id(std::optional<Identity> id) const;
  std::string log_id_by_order(int order) const;
};

}  // namespace hanabi
