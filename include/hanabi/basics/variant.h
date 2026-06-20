// Port of python-bot/src/hanabi_bot/basics/variant.py.
// Original Scala: scala-bot/src/scala_bot/basics/Variant.scala.
//
// A Variant is a named ruleset for Hanabi: suit composition + clue-touch rules
// + score-curve modifiers. Suit classification is done via substring match on
// the suit name (faithful to the Scala regex predicates).
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"

namespace hanabi {

struct SuitType {
  bool whitish = false;
  bool rainbowish = false;
  bool pinkish = false;
  bool brownish = false;
  bool dark = false;
  bool prism = false;
  bool muddy = false;
  bool inverted = false;
  // Reversed suits play 5 → 4 → 3 → 2 → 1 (instead of 1 → 5). Detected
  // by "Reversed" in the suit name. Orthogonal to `inverted` (Orange's
  // action-button swap) — a suit can in principle be both.
  bool reversed = false;

  static SuitType of_name(std::string_view name);

  bool operator==(const SuitType&) const = default;
};

struct Suit {
  std::string name;
  std::optional<char> abbreviation;  // single lowercase character, or none
  SuitType suit_type;
  // Names of colour clues that touch this suit. Loaded from the
  // `clueColors` field in data/suits.json. Empty for whitish/prism
  // suits (which use suit-flag logic instead of name matching) and
  // implicitly "all" for rainbowish suits (also flag-driven).
  // Multi-colour suits like "Lime" have ["Yellow", "Green"], and
  // Ambiguous variants reuse colour names across suits (Tomato and
  // Mahogany both list ["Red"]).
  std::vector<std::string> clue_colors;

  bool operator==(const Suit&) const = default;
};

struct Variant {
  int id = 0;
  std::string name;
  std::vector<Suit> suits;
  std::vector<char> short_forms;
  // Distinct colour clue names available in this variant, in order of
  // first appearance across `suits[].clue_colors`. A clue with
  // `ClueKind::COLOUR` and value `i` matches the colour
  // `clue_colour_names[i]`. For most variants this length equals the
  // number of non-special suits, but Ambiguous variants collapse
  // multiple suits to a single colour (e.g. Ambiguous (6 Suits)
  // exposes only Red/Green/Blue across six suits).
  std::vector<std::string> clue_colour_names;
  // One representative suit index per distinct colour, in the same
  // order as `clue_colour_names`. `colourable_suit_indices.size()`
  // always equals `clue_colour_names.size()`; existing callers that
  // iterate this vector as "the list of colour clues" continue to
  // work unchanged.
  std::vector<int> colourable_suit_indices;
  std::optional<int> critical_rank;
  bool clue_starved = false;
  std::optional<int> special_rank;
  bool rainbow_s = false;
  bool white_s = false;
  bool pink_s = false;
  bool brown_s = false;
  bool deceptive_s = false;
  bool scarce_ones = false;
  // Funnels: rank-K clue touches all cards of rank ≤ K in non-pinkish,
  // non-brownish suits. Chimneys: same but rank ≥ K. Pink/brown suits
  // still follow their own touch rules inside these variants.
  bool funnels = false;
  bool chimneys = false;

  // Convenience: dereference colourable_suit_indices into actual Suit refs.
  std::vector<Suit> colourable_suits() const;

  std::vector<Identity> all_ids() const;
  int card_count(Identity id) const;
  int total_cards() const;
  bool id_touched(Identity id, ClueKind kind, int value) const;
  std::vector<Identity> touch_possibilities(ClueKind kind, int value) const;
};

// Load the global suit catalog from data/suits.json. Cached.
const std::unordered_map<std::string, Suit>& load_suit_catalog();

// Load all variants from data/variants.json. Cached. Map keyed by variant name.
const std::unordered_map<std::string, Variant>& load_variants();

// Look up by name. Throws std::out_of_range if not found.
const Variant& get_variant(const std::string& name);

// Build a Variant from a single variants.json entry (test/replay support).
Variant variant_from_json(const nlohmann::json& entry);

}  // namespace hanabi
