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
  // The shared view exists only where the two can differ. Leaving it empty in
  // every other variant is what makes `shared_score` / `shared_pace` free.
  if (variant.throw_it_in_a_hole) {
    s.common_play_stacks.assign(num_suits, 0);
    s.pairwise_play_stacks.assign(s.num_players, std::vector<int>(num_suits, 0));
  }
  s.discard_stacks.assign(num_suits, std::array<std::vector<int>, 5>{});
  s.max_ranks.assign(num_suits, 5);
  s.base_count.assign(num_ids, 0);
  s.card_count.assign(num_ids, 0);

  // Reversed-direction initial values: play_stacks starts at 6 (sentinel
  // "nothing played; next playable = 5") and max_ranks at 1 (the lowest
  // achievable rank; rises as low-rank criticals get discarded).
  for (int suit_index = 0; suit_index < num_suits; ++suit_index) {
    if (variant.suits[suit_index].suit_type.reversed) {
      s.play_stacks[suit_index] = 6;
      if (!s.common_play_stacks.empty()) s.common_play_stacks[suit_index] = 6;
      for (auto& row : s.pairwise_play_stacks) row[suit_index] = 6;
      s.max_ranks[suit_index] = 1;
    }
  }
  s.pairwise_evidence = s.pairwise_play_stacks;
  s.common_evidence = s.common_play_stacks;
  if (variant.throw_it_in_a_hole) s.play_evidence = s.play_stacks;

  IdentitySet playable = IdentitySet::empty();
  IdentitySet critical = IdentitySet::empty();
  for (int suit_index = 0; suit_index < num_suits; ++suit_index) {
    const bool reversed = variant.suits[suit_index].suit_type.reversed;
    for (int rank = 1; rank <= 5; ++rank) {
      Identity id(suit_index, rank);
      int count = variant.card_count(id);
      s.card_count[id.to_ord()] = count;
      // Initially playable rank: 1 for normal, 5 for reversed.
      if ((reversed && rank == 5) || (!reversed && rank == 1)) {
        playable = playable.add(id);
      }
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
    // For normal suits: discarding critical-rank-N drops the max
    // achievable to N-1. For reversed suits (play direction is high→
    // low): discarding critical-rank-N raises the min achievable to
    // N+1 (where max_ranks stores the *lowest* still-achievable rank).
    if (variant->suits[suit_index].suit_type.reversed) {
      out.max_ranks[suit_index] = std::max(out.max_ranks[suit_index], id.rank + 1);
    } else {
      out.max_ranks[suit_index] = std::min(out.max_ranks[suit_index], id.rank - 1);
    }
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
  const bool reversed = variant->suits[id.suit_index].suit_type.reversed;
  // Reversed plays advance the stack downward, so the "next playable"
  // is id.prev() (= rank-1). The clue-regain happens on the *final*
  // rank in the play direction: rank-5 for normal, rank-1 for reversed.
  std::optional<Identity> next = reversed ? id.prev() : id.next();
  if (next) new_playable = new_playable.union_with(*next);

  out.play_stacks[id.suit_index] = id.rank;
  // Throw It in a Hole: the evidence half of our belief advances only through
  // the prefix -- a world floor raised `play_stacks` without it (v16.24.0).
  if (!out.play_evidence.empty()) {
    int& e = out.play_evidence[id.suit_index];
    if (reversed ? id.rank == e - 1 : id.rank == e + 1) e = id.rank;
  }
  ++out.base_count[id.to_ord()];
  out.playable_set = new_playable;
  out.trash_set = trash_set.union_with(id);
  // Throw It in a Hole pays nothing for the final rank: a 5 into the hole
  // returns no clue token. Gated here rather than inside `regain_clue`,
  // because an ordinary DISCARD still refunds under TIIAH and both callers
  // share that helper.
  const bool is_final = reversed ? (id.rank == 1) : (id.rank == 5);
  if (is_final && !variant->throw_it_in_a_hole) out = out.regain_clue();
  return out;
}

namespace {

// Is `rank` further along its suit than a stack standing at `height`? A
// reversed suit counts down, so "further" is the smaller number there.
bool beyond(const Variant& v, int suit, int height, int rank) {
  return v.suits[suit].suit_type.reversed ? rank < height : rank > height;
}

// Is `rank` the NEXT card on a stack standing at `height`?
bool next_on(const Variant& v, int suit, int height, int rank) {
  return v.suits[suit].suit_type.reversed ? rank == height - 1 : rank == height + 1;
}

// The EVIDENCE half of a view (v16.24.0) advances only through the prefix: it is
// the plays the seats can name in sequence. A card named above a gap raises the
// view itself to at least that card, and the gap is a band of cards the seats
// know are down without being able to say which card each was -- the band a
// hole card of theirs can be (`tiiah::open_worlds`).
void advance_evidence(const Variant& v, std::vector<int>& ev, Identity id) {
  if (id.suit_index >= static_cast<int>(ev.size())) return;
  if (next_on(v, id.suit_index, ev[id.suit_index], id.rank)) ev[id.suit_index] = id.rank;
}

}  // namespace

// Advance the SHARED view only. Throw It in a Hole's second stack vector:
// the caller has decided that this play's identity was common knowledge, or
// that a superposition collapsed on evidence every seat shares.
//
// Deliberately narrow — it moves `common_play_stacks` and nothing else. The
// accounting (`base_count`, `playable_set`, the clue refund) belongs to the
// believed view and is `with_play`'s job.
State State::with_common_play(Identity id) const {
  State out = *this;
  if (out.common_play_stacks.empty()) return out;
  out.common_play_stacks[id.suit_index] = id.rank;
  advance_evidence(*out.variant, out.common_evidence, id);
  return out;
}

State State::with_common_floor(const std::vector<int>& floor) const {
  if (common_play_stacks.empty()) return *this;
  State out = *this;
  for (size_t k = 0; k < out.common_play_stacks.size() && k < floor.size(); ++k) {
    if (beyond(*variant, static_cast<int>(k), out.common_play_stacks[k], floor[k])) {
      out.common_play_stacks[k] = floor[k];
    }
  }
  return out;
}

// As narrow as `with_common_play`, and for the same reason: it moves one row of
// one vector. WHO learned the play is the caller's judgement -- a hidden play is
// known to every seat but the one who made it, unless they knew it themselves,
// in which case it is common knowledge and belongs in the shared view instead.
State State::with_pairwise_play(Identity id,
                                const std::vector<int>& knowers) const {
  State out = *this;
  if (out.pairwise_play_stacks.empty()) return out;
  for (int p : knowers) {
    if (p < 0 || p >= static_cast<int>(out.pairwise_play_stacks.size())) continue;
    out.pairwise_play_stacks[p][id.suit_index] = id.rank;
    if (p < static_cast<int>(out.pairwise_evidence.size())) {
      advance_evidence(*out.variant, out.pairwise_evidence[p], id);
    }
  }
  return out;
}

State State::with_common_at_least(Identity id) const {
  if (common_play_stacks.empty()) return *this;
  State out = *this;
  const int k = id.suit_index;
  if (beyond(*variant, k, out.common_play_stacks[k], id.rank)) {
    out.common_play_stacks[k] = id.rank;
  }
  advance_evidence(*variant, out.common_evidence, id);
  return out;
}

State State::with_pairwise_at_least(Identity id,
                                    const std::vector<int>& knowers) const {
  if (pairwise_play_stacks.empty()) return *this;
  State out = *this;
  const int k = id.suit_index;
  for (int p : knowers) {
    if (p < 0 || p >= static_cast<int>(out.pairwise_play_stacks.size())) continue;
    // Both vectors, each on its own: the row takes the card as a floor, the
    // evidence only when it is the next card it can name.
    if (beyond(*variant, k, out.pairwise_play_stacks[p][k], id.rank)) {
      out.pairwise_play_stacks[p][k] = id.rank;
    }
    if (p < static_cast<int>(out.pairwise_evidence.size())) {
      advance_evidence(*variant, out.pairwise_evidence[p], id);
    }
  }
  return out;
}

State State::with_rows_at_least_common() const {
  if (pairwise_play_stacks.empty() || common_play_stacks.empty()) return *this;
  State out = *this;
  auto raise = [&](std::vector<std::vector<int>>& rows, const std::vector<int>& to) {
    for (auto& row : rows) {
      for (size_t k = 0; k < row.size() && k < to.size(); ++k) {
        if (beyond(*variant, static_cast<int>(k), row[k], to[k])) row[k] = to[k];
      }
    }
  };
  raise(out.pairwise_play_stacks, common_play_stacks);
  if (!common_evidence.empty()) raise(out.pairwise_evidence, common_evidence);
  return out;
}

State State::with_pairwise_floor(int p, const std::vector<int>& floor) const {
  if (p < 0 || p >= static_cast<int>(pairwise_play_stacks.size())) return *this;
  State out = *this;
  auto& row = out.pairwise_play_stacks[p];
  for (size_t k = 0; k < row.size() && k < floor.size(); ++k) {
    if (beyond(*variant, static_cast<int>(k), row[k], floor[k])) row[k] = floor[k];
  }
  return out;
}

State State::private_base() const {
  return play_evidence.empty() ? *this : with_band(play_evidence);
}

std::vector<int> State::evidence_known_to_both(int a, int b) const {
  if (pairwise_evidence.empty()) return stacks_known_to_both(a, b);
  const int rows = static_cast<int>(pairwise_evidence.size());
  auto row = [&](int p) { return p >= 0 && p < rows; };
  if (a == our_player_index && row(b)) return pairwise_evidence[b];
  if (b == our_player_index && row(a)) return pairwise_evidence[a];
  return common_evidence.empty() ? common_play_stacks : common_evidence;
}

State State::with_stacks(const std::vector<int>& stacks) const {
  if (stacks.size() != play_stacks.size()) return *this;
  State out = *this;
  out.play_stacks = stacks;
  // Bound to the variant's own identities, as `receiver_ctp_set` is: `create`
  // otherwise walks every ordinal and would invent suits this variant lacks.
  const int n = static_cast<int>(variant->suits.size()) * 5;
  out.playable_set = IdentitySet::create(
      [&out](Identity i) { return out.is_playable(i) && !out.is_basic_trash(i); },
      n);
  out.trash_set =
      IdentitySet::create([&out](Identity i) { return out.is_basic_trash(i); }, n);
  return out;
}

State State::shared_view() const {
  if (common_play_stacks.empty()) return *this;
  return with_stacks(common_play_stacks);
}

// Row `other` is what we know that seat knows. Our own row is never maintained
// -- against ourselves the answer is our own belief, which is strictly more.
State State::pairwise_view(int other) const {
  if (other == our_player_index) return *this;
  if (other < 0 || other >= static_cast<int>(pairwise_play_stacks.size())) {
    return *this;
  }
  return with_stacks(pairwise_play_stacks[other]);
}

std::vector<int> State::stacks_known_to_both(int a, int b) const {
  if (pairwise_play_stacks.empty()) return play_stacks;  // not TIIAH
  const int rows = static_cast<int>(pairwise_play_stacks.size());
  auto row = [&](int p) { return p >= 0 && p < rows; };
  // Symmetric: whichever of the pair we are, the row we want is the other's.
  if (a == our_player_index && row(b)) return pairwise_play_stacks[b];
  if (b == our_player_index && row(a)) return pairwise_play_stacks[a];
  return common_play_stacks;
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
  for (int s = 0; s < static_cast<int>(play_stacks.size()); ++s) {
    total += played_count(s);
  }
  return total;
}

int State::shared_score() const {
  // Outside Throw It in a Hole there is no second view: every play is public,
  // so the shared score IS the score and the vector is never populated.
  if (common_play_stacks.empty()) return score();
  int total = 0;
  for (int s = 0; s < static_cast<int>(common_play_stacks.size()); ++s) {
    const auto& st = variant->suits[s].suit_type;
    total += st.reversed ? 6 - common_play_stacks[s] : common_play_stacks[s];
  }
  return total;
}

int State::max_score() const {
  int total = 0;
  for (int s = 0; s < static_cast<int>(max_ranks.size()); ++s) {
    total += max_played(s);
  }
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
  // Alternating Clues: the server rejects a clue of the same kind as the last
  // one given, by anyone. Filtered HERE rather than in the callers so that all
  // six of them agree -- including `eval.cpp` and the endgame solver, which
  // would otherwise search lines built on clues that cannot be played.
  // Nullopt before the first clue leaves both kinds available.
  const bool block_rank = v.alternating_clues &&
                          last_clue_kind == std::optional<ClueKind>{ClueKind::RANK};
  const bool block_colour =
      v.alternating_clues &&
      last_clue_kind == std::optional<ClueKind>{ClueKind::COLOUR};

  for (int rank = 1; !block_rank && rank <= 5; ++rank) {
    if (rank_blocked && v.special_rank == rank) continue;
    // `clue_ranks` is what the variant actually offers. For the Pink-Ones /
    // Pink-Fives families it agrees exactly with `rank_blocked` above, so
    // nothing moves; it is load-bearing for Odds and Evens ({1,2}) and for the
    // Number Mute family ({}, no rank clues at all).
    if (std::find(v.clue_ranks.begin(), v.clue_ranks.end(), rank) ==
        v.clue_ranks.end()) {
      continue;
    }
    // A clue must touch something -- EXCEPT in a BLIND family, where no clue of
    // that kind ever touches anything and the server allows it anyway. Without
    // the exception the bot enumerates zero clues in Number Blind and Totally
    // Blind and can never clue at all.
    if (v.rank_clues_touch_nothing ||
        !clue_touched(hands[target], ClueKind::RANK, rank).empty()) {
      clues.emplace_back(ClueKind::RANK, rank, target);
    }
  }
  const int num_colours =
      block_colour ? 0 : static_cast<int>(v.colourable_suit_indices.size());
  for (int suit_index = 0; suit_index < num_colours; ++suit_index) {
    // The colour half of the same exception -- Color Blind and Totally Blind.
    if (v.colour_clues_touch_nothing ||
        !clue_touched(hands[target], ClueKind::COLOUR, suit_index).empty()) {
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
