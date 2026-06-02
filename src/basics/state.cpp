#include "hanabi/basics/state.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace hanabi {

// --- Factory --------------------------------------------------------------

State State::create(std::vector<std::string> names, int our_player_index,
                     const Variant& variant, TableOptions options) {
  const int num_suits = static_cast<int>(variant.suits.size());
  const int num_ids = num_suits * 5;

  State s;
  s.variant = &variant;
  s.options = std::move(options);
  s.num_players = static_cast<int>(names.size());
  s.names = std::move(names);
  s.our_player_index = our_player_index;
  s.cards_left = variant.total_cards();
  s.cards_total = s.cards_left;

  s.play_stacks.assign(num_suits, 0);
  s.discard_stacks.assign(num_suits, std::array<std::vector<int>, 5>{});
  s.max_ranks.assign(num_suits, 5);
  s.base_count.assign(num_ids, 0);
  s.card_count.assign(num_ids, 0);

  IdentitySet playable = IdentitySet::empty();
  IdentitySet critical = IdentitySet::empty();
  for (int suit_index = 0; suit_index < num_suits; ++suit_index) {
    for (int rank = 1; rank <= 5; ++rank) {
      Identity id(suit_index, rank);
      int count = variant.card_count(id);
      s.card_count[id.to_ord()] = count;
      if (rank == 1) playable = playable.add(id);
      if (count == 1) critical = critical.add(id);
    }
  }
  s.all_ids = IdentitySet::from_iter(variant.all_ids());
  s.playable_set = playable;
  s.critical_set = critical;
  s.trash_set = IdentitySet::empty();

  s.hands.assign(s.num_players, {});

  return s;
}

// --- Mutators -------------------------------------------------------------

State State::with_discard(Identity id, int order) const {
  State out = *this;
  const int suit_index = id.suit_index;
  const int rank_idx = id.rank - 1;

  // Cons: newer orders go to the head of the per-identity discard pile.
  auto& pile = out.discard_stacks[suit_index][rank_idx];
  pile.insert(pile.begin(), order);

  const int ord = id.to_ord();
  ++out.base_count[ord];

  if (critical_set.contains(id)) {
    out.max_ranks[suit_index] = std::min(out.max_ranks[suit_index], id.rank - 1);
    out.critical_set = critical_set.difference(id);
    out.trash_set = trash_set.union_with(id);
    out.playable_set = playable_set.difference(id);
  } else {
    const bool became_critical =
        card_count[ord] - out.base_count[ord] == 1 && !is_basic_trash(id);
    if (became_critical) out.critical_set = critical_set.union_with(id);
  }
  return out;
}

State State::with_play(Identity id) const {
  State out = *this;
  IdentitySet new_playable = playable_set.difference(id);
  if (auto next = id.next()) new_playable = new_playable.union_with(*next);

  out.play_stacks[id.suit_index] = id.rank;
  ++out.base_count[id.to_ord()];
  out.playable_set = new_playable;
  out.trash_set = trash_set.union_with(id);
  if (id.rank == 5) out = out.regain_clue();
  return out;
}

State State::try_play(Identity id) const {
  return is_playable(id) ? with_play(id) : *this;
}

State State::regain_clue() const {
  State out = *this;
  if (variant->clue_starved) {
    if (half_clue_token) {
      ++out.clue_tokens;
      out.half_clue_token = false;
    } else {
      out.half_clue_token = clue_tokens < 8;
    }
    return out;
  }
  out.clue_tokens = std::min(8, clue_tokens + 1);
  return out;
}

// --- Pure helpers ---------------------------------------------------------

bool State::ended() const {
  return strikes == 3 || score() == max_score() ||
         (endgame_turns.has_value() && *endgame_turns == 0);
}

int State::score() const {
  int total = 0;
  for (int v : play_stacks) total += v;
  return total;
}

int State::max_score() const {
  int total = 0;
  for (int v : max_ranks) total += v;
  return total;
}

bool State::is_critical(Identity id) const {
  if (is_basic_trash(id)) return false;
  return static_cast<int>(discard_stacks[id.suit_index][id.rank - 1].size()) ==
         card_count[id.to_ord()] - 1;
}

int State::holder_of(int order) const {
  if (order >= static_cast<int>(holders.size())) {
    throw std::invalid_argument("Tried to get holder of a card that hasn't been drawn yet");
  }
  return holders[order];
}

int State::multiplicity(IdentitySet ids) const {
  int total = 0;
  for (Identity id : ids) total += card_count[id.to_ord()];
  return total;
}

bool State::has_consistent_infs(const Thought& thought) const {
  if (thought.possible.length() == 1) return true;
  auto true_id = deck[thought.order].id();
  return !true_id.has_value() || thought.inferred.contains(*true_id);
}

std::vector<int> State::clue_touched(const std::vector<int>& orders,
                                       ClueKind kind, int value) const {
  std::vector<int> result;
  for (int order : orders) {
    const Card& card = deck[order];
    if (auto id = card.id();
        id && variant->id_touched(*id, kind, value)) {
      result.push_back(order);
    }
  }
  return result;
}

std::vector<Clue> State::all_colour_clues(int target) const {
  std::vector<Clue> result;
  const int n = static_cast<int>(variant->colourable_suit_indices.size());
  for (int suit_index = 0; suit_index < n; ++suit_index) {
    if (!clue_touched(hands[target], ClueKind::COLOUR, suit_index).empty()) {
      result.emplace_back(ClueKind::COLOUR, suit_index, target);
    }
  }
  return result;
}

std::vector<Clue> State::all_valid_clues(int target) const {
  std::vector<Clue> clues;
  const Variant& v = *variant;
  const bool rank_blocked =
      v.special_rank.has_value() && (v.pink_s || v.brown_s || v.deceptive_s);

  for (int rank = 1; rank <= 5; ++rank) {
    if (rank_blocked && v.special_rank == rank) continue;
    if (!clue_touched(hands[target], ClueKind::RANK, rank).empty()) {
      clues.emplace_back(ClueKind::RANK, rank, target);
    }
  }
  const int num_colours = static_cast<int>(v.colourable_suit_indices.size());
  for (int suit_index = 0; suit_index < num_colours; ++suit_index) {
    if (!clue_touched(hands[target], ClueKind::COLOUR, suit_index).empty()) {
      clues.emplace_back(ClueKind::COLOUR, suit_index, target);
    }
  }
  return clues;
}

bool State::includes_variant(std::string_view needle) const {
  for (const auto& suit : variant->suits) {
    if (suit.name.find(needle) != std::string::npos) return true;
  }
  return false;
}

Identity State::expand_short(std::string_view short_) const {
  if (short_.size() != 2) {
    throw std::invalid_argument("Short should be exactly 2 characters");
  }
  auto it = std::find(variant->short_forms.begin(), variant->short_forms.end(), short_[0]);
  if (it == variant->short_forms.end()) {
    throw std::invalid_argument("Colour doesn't exist in selected variant");
  }
  if (!std::isdigit(static_cast<unsigned char>(short_[1]))) {
    throw std::invalid_argument("Rank doesn't exist in selected variant");
  }
  return Identity(static_cast<int>(it - variant->short_forms.begin()),
                   short_[1] - '0');
}

std::string State::log_id(std::optional<Identity> id) const {
  if (!id) return "xx";
  return std::string{variant->short_forms[id->suit_index]} + std::to_string(id->rank);
}

std::string State::log_id_by_order(int order) const {
  return log_id(deck[order].id());
}

}  // namespace hanabi
