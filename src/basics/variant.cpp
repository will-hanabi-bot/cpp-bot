#include "hanabi/basics/variant.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string_view>

#include <nlohmann/json.hpp>

namespace hanabi {
namespace {

// Substring predicates matching the Python regex set:
//   WHITISH    = White|Gray|Light|Null
//   RAINBOWISH = Rainbow|Omni
//   PINKISH    = Pink|Omni
//   BROWNISH   = Brown|Muddy|Cocoa|Null
//   DARK       = Black|Dark|Gray|Cocoa
//   PRISM      = Prism
//   MUDDY      = Muddy|Cocoa
//   NO_COLOUR  = White|Gray|Light|Null|Rainbow|Omni|Prism
constexpr bool contains_any(std::string_view name,
                            std::initializer_list<std::string_view> needles) {
  for (auto n : needles) {
    if (name.find(n) != std::string_view::npos) return true;
  }
  return false;
}

bool is_no_colour(std::string_view name) {
  return contains_any(name, {"White", "Gray", "Light", "Null", "Rainbow", "Omni", "Prism"});
}

std::string load_data_file(const std::string& filename) {
  const std::string path = std::string(HANABI_DATA_DIR) + "/" + filename;
  std::ifstream f(path);
  if (!f.is_open()) {
    throw std::runtime_error("missing data file: " + path);
  }
  return std::string{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

// Resolve a variant's DISPLAY suit name to its catalog entry.
//
// A reversed suit is named "<Base> Reversed" in `data/variants.json`, but the
// catalog is keyed on the base suit -- the reversal is a variant-level modifier
// (the newID for "Orange Reversed (4 Suits)" is "R+G+B+Or:R", i.e. the `Or` suit
// with `:R` applied). Looking the display name up directly misses, and the stub
// that results carries an EMPTY `clue_colors` -- which is a suit's entire link
// to its colour clue. The colour then matches no identity at all, so every card
// it touches is narrowed to nothing.
//
// Replay 1969696 is what that looks like from the table: an Orange clue in
// "Orange Reversed (4 Suits)" emptied `possible` on all four cards it touched,
// and the orange ladder -- which asks `id_touched` and reads `possible` -- could
// not see the clue at either of its gates. 33 shipped variants across 12 such
// names are affected, and in each the bot is blind to one whole colour.
const Suit* find_catalog_suit(const std::unordered_map<std::string, Suit>& catalog,
                              const std::string& name) {
  auto it = catalog.find(name);
  if (it != catalog.end()) return &it->second;
  static constexpr std::string_view kReversed = " Reversed";
  if (name.size() > kReversed.size() &&
      std::string_view(name).substr(name.size() - kReversed.size()) == kReversed) {
    it = catalog.find(name.substr(0, name.size() - kReversed.size()));
    if (it != catalog.end()) return &it->second;
  }
  return nullptr;
}

// Pick a one-character short form, avoiding collisions. Port of _pick_short.
char pick_short(const std::string& sname,
                const std::unordered_map<std::string, Suit>& catalog,
                const std::vector<char>& taken) {
  if (sname == "Black") return 'k';
  if (sname == "Pink") return 'i';
  if (sname == "Brown") return 'n';

  // Deliberately NOT resolved through `find_catalog_suit`. A short form is only
  // a display/test convenience, and every recorded log and replay fixture that
  // uses a "<Base> Reversed" variant already spells its cards with the letter
  // this fallback picks -- "Black Reversed" is 'l', not Black's 'k'. Resolving
  // the base here would be tidier and would break all of them for nothing.
  auto it = catalog.find(sname);
  char candidate = '\0';
  if (it != catalog.end() && it->second.abbreviation) {
    candidate = *it->second.abbreviation;
  } else {
    candidate = static_cast<char>(std::tolower(static_cast<unsigned char>(sname.front())));
  }
  if (std::find(taken.begin(), taken.end(), candidate) == taken.end()) return candidate;

  for (char c : sname) {
    char lc = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (std::find(taken.begin(), taken.end(), lc) == taken.end()) return lc;
  }
  throw std::runtime_error("No unused character found for suit '" + sname + "'");
}

Variant make_variant(int id, std::string name, std::vector<std::string> suit_names,
                     std::optional<int> critical_rank, bool clue_starved,
                     std::optional<int> special_rank, bool rainbow_s, bool white_s,
                     bool pink_s, bool brown_s, bool deceptive_s, bool scarce_ones,
                     bool funnels, bool chimneys, bool odds_and_evens,
                     bool alternating_clues, bool synesthesia,
                     std::vector<int> clue_ranks) {
  const auto& catalog = load_suit_catalog();
  Variant v;
  v.id = id;
  v.name = std::move(name);
  v.critical_rank = critical_rank;
  v.clue_starved = clue_starved;
  v.special_rank = special_rank;
  v.rainbow_s = rainbow_s;
  v.white_s = white_s;
  v.pink_s = pink_s;
  v.brown_s = brown_s;
  v.deceptive_s = deceptive_s;
  v.scarce_ones = scarce_ones;
  v.funnels = funnels;
  v.chimneys = chimneys;
  v.alternating_clues = alternating_clues;
  v.synesthesia = synesthesia;
  v.odds_and_evens = odds_and_evens;
  v.clue_ranks = std::move(clue_ranks);
  v.suits.reserve(suit_names.size());
  v.short_forms.reserve(suit_names.size());

  for (size_t i = 0; i < suit_names.size(); ++i) {
    const auto& sname = suit_names[i];
    char short_c = pick_short(sname, catalog, v.short_forms);
    const Suit* entry = find_catalog_suit(catalog, sname);
    Suit suit;
    if (entry && entry->name == sname) {
      suit = *entry;
    } else if (entry) {
      // A "<Base> Reversed" name. Take the base's `clue_colors` -- and ONLY
      // that. The display name, the short form and the name-derived SuitType
      // all stay exactly as the old stub built them, so logs, snapshots and
      // `suit_index_of` are untouched; `SuitType::of_name` already carries
      // `reversed` alongside `inverted` / `dark` / `whitish` / `prism` for
      // every one of these twelve names.
      suit = Suit{sname, short_c, SuitType::of_name(sname), entry->clue_colors};
    } else {
      suit = Suit{sname, short_c, SuitType::of_name(sname), {}};
    }
    v.suits.push_back(suit);
    v.short_forms.push_back(short_c);
  }

  // Build `clue_colour_names` + `colourable_suit_indices` together so
  // the sizes match: one entry per DISTINCT colour name appearing in
  // any suit's `clue_colors`, in order of first encounter. Each entry
  // in `colourable_suit_indices` is the index of the first suit that
  // contributes that colour. Ambiguous variants collapse multiple
  // suits to fewer colours (e.g. Ambiguous (6 Suits) → 3 colours).
  // Prism, whitish, and rainbowish suits don't contribute to the
  // colour name list (their touch rules are flag-driven inside
  // id_touched).
  for (size_t i = 0; i < v.suits.size(); ++i) {
    const Suit& suit = v.suits[i];
    if (suit.suit_type.rainbowish || suit.suit_type.whitish ||
        suit.suit_type.prism) {
      continue;
    }
    for (const std::string& c : suit.clue_colors) {
      auto seen = std::find(v.clue_colour_names.begin(),
                             v.clue_colour_names.end(), c);
      if (seen == v.clue_colour_names.end()) {
        v.clue_colour_names.push_back(c);
        v.colourable_suit_indices.push_back(static_cast<int>(i));
      }
    }
  }
  // Fallback for variants whose suit catalog entries are missing
  // clueColors (shouldn't happen for shipped data but keeps legacy
  // tests that construct synthetic Variants working). Pre-fix
  // behaviour: one colour per non-no-colour suit, matched by suit
  // equality.
  if (v.clue_colour_names.empty()) {
    for (size_t i = 0; i < v.suits.size(); ++i) {
      if (!is_no_colour(v.suits[i].name)) {
        v.clue_colour_names.push_back(v.suits[i].name);
        v.colourable_suit_indices.push_back(static_cast<int>(i));
      }
    }
  }
  return v;
}

}  // namespace

// --- SuitType -------------------------------------------------------------

SuitType SuitType::of_name(std::string_view name) {
  SuitType st;
  st.whitish = contains_any(name, {"White", "Gray", "Light", "Null"});
  st.rainbowish = contains_any(name, {"Rainbow", "Omni"});
  st.pinkish = contains_any(name, {"Pink", "Omni"});
  st.brownish = contains_any(name, {"Brown", "Muddy", "Cocoa", "Null"});
  st.dark = contains_any(name, {"Black", "Dark", "Gray", "Cocoa"});
  st.prism = name.find("Prism") != std::string_view::npos;
  st.muddy = contains_any(name, {"Muddy", "Cocoa"});
  st.inverted = name.find("Orange") != std::string_view::npos;
  st.reversed = name.find("Reversed") != std::string_view::npos;
  return st;
}

// --- Variant accessors ----------------------------------------------------

std::vector<Suit> Variant::colourable_suits() const {
  std::vector<Suit> out;
  out.reserve(colourable_suit_indices.size());
  for (int i : colourable_suit_indices) out.push_back(suits[i]);
  return out;
}

std::vector<Identity> Variant::all_ids() const {
  std::vector<Identity> out;
  out.reserve(suits.size() * 5);
  for (int s = 0; s < static_cast<int>(suits.size()); ++s) {
    for (int r = 1; r <= 5; ++r) out.emplace_back(s, r);
  }
  return out;
}

int Variant::card_count(Identity id) const {
  const Suit& s = suits[id.suit_index];
  if (s.suit_type.dark || (critical_rank && *critical_rank == id.rank)) return 1;
  if (id.rank == 1 && scarce_ones) return 2;
  static constexpr int kCounts[5] = {3, 2, 2, 2, 1};
  // Reversed suits play 5 → 1, so the rarity is flipped: the unique
  // "rank-5 stays last" criticality lives at rank-1 instead.
  static constexpr int kCountsReversed[5] = {1, 2, 2, 2, 3};
  if (s.suit_type.reversed) return kCountsReversed[id.rank - 1];
  return kCounts[id.rank - 1];
}

int Variant::total_cards() const {
  int total = 0;
  for (Identity id : all_ids()) total += card_count(id);
  return total;
}

bool Variant::id_touched(Identity id, ClueKind kind, int value) const {
  const Suit& suit = suits[id.suit_index];
  const SuitType& st = suit.suit_type;
  const int rank = id.rank;

  if (kind == ClueKind::COLOUR) {
    if (st.rainbowish) return true;
    if (st.whitish) return false;
    if (special_rank && *special_rank == rank) {
      if (rainbow_s) return true;
      if (white_s) return false;
    }
    // Synesthesia: on top of its own colour, a card of rank N answers to the
    // Nth colour clue. Two carve-outs, and the position of this branch is what
    // implements one of them:
    //
    //  * BROWN is exempt by the rule itself -- a brown card is clued as brown
    //    and never as the colour of its rank. It still reaches the name match
    //    below, so `Brown` alone touches it.
    //  * WHITE is exempt by SITTING BELOW `st.whitish`, which already returned
    //    false. That matches how hanab.live currently behaves: White in a
    //    Synesthesia variant is indistinguishable from Null -- untouched by
    //    everything -- rather than being reachable through its rank.
    //
    // Rainbow returned true further up, so it is unaffected.
    if (synesthesia && !st.brownish && rank - 1 == value) return true;
    if (st.prism) {
      return ((rank - 1) % clue_colour_names.size()) == static_cast<size_t>(value);
    }
    // Multiple suits may share the same colour clue name (Ambiguous
    // variants) and a single suit may list multiple colour names
    // (Lime → Yellow + Green). Match by colour name lookup, not by
    // suit equality against a single rep.
    if (value < 0 ||
        value >= static_cast<int>(clue_colour_names.size())) {
      return false;
    }
    const std::string& target = clue_colour_names[value];
    for (const std::string& c : suit.clue_colors) {
      if (c == target) return true;
    }
    return false;
  }

  // Rank clue.
  if (st.pinkish) return true;
  if (st.brownish) return false;
  if (special_rank && *special_rank == rank) {
    if (pink_s) return rank != value;
    if (brown_s) return false;
    if (deceptive_s) {
      const int offset = (rank == 1) ? 2 : 1;
      return (id.suit_index % 4) + offset == value;
    }
  }
  // Odds and Evens: the clue value names a PARITY, not a rank. 1 is "odd"
  // (ranks 1/3/5), 2 is "even" (2/4). Sits below the pinkish / brownish /
  // special-rank branches, which keep their own rules here as they do inside a
  // funnels or chimneys variant.
  if (odds_and_evens) return (rank % 2 == 1) == (value == 1);
  // Funnels / Chimneys apply only to non-pinkish, non-brownish suits
  // (those branches returned above). Pink/brown keep their own rules
  // even inside a funnels/chimneys variant.
  if (funnels) return rank <= value;
  if (chimneys) return rank >= value;
  return rank == value;
}

std::vector<Identity> Variant::touch_possibilities(ClueKind kind, int value) const {
  std::vector<Identity> out;
  for (Identity id : all_ids()) {
    if (id_touched(id, kind, value)) out.push_back(id);
  }
  return out;
}

// --- Loaders --------------------------------------------------------------

const std::unordered_map<std::string, Suit>& load_suit_catalog() {
  static const std::unordered_map<std::string, Suit> cache = [] {
    std::unordered_map<std::string, Suit> result;
    const auto raw = nlohmann::json::parse(load_data_file("suits.json"));
    for (const auto& entry : raw) {
      const std::string name = entry.at("name").get<std::string>();
      std::optional<char> abbrev;
      if (entry.contains("abbreviation") && entry["abbreviation"].is_string()) {
        std::string s = entry["abbreviation"].get<std::string>();
        if (!s.empty()) abbrev = static_cast<char>(std::tolower(static_cast<unsigned char>(s.front())));
      }
      SuitType st = SuitType::of_name(name);
      // Belt-and-suspenders: honor the JSON "inverted" field so any future
      // suit whose name doesn't match the "Orange" substring still gets the
      // flag if the data marks it.
      if (entry.value("inverted", false)) st.inverted = true;
      if (entry.value("reversed", false)) st.reversed = true;
      // Resolve the suit's clue colors. Precedence: explicit `clueColors`
      // array in JSON > the `noClueColors` / `allClueColors` flags (which
      // make the colour set empty / universal — handled at touch time
      // via the rainbowish / whitish flags below) > implicit default of
      // {suit_name} for primary-colour suits (Red, Yellow, Black, Pink,
      // Brown, Orange, ...) whose JSON entry omits clueColors. Prism
      // suits use rank-based touch (no name list).
      std::vector<std::string> clue_colors;
      const bool no_colours = entry.value("noClueColors", false);
      const bool all_colours = entry.value("allClueColors", false);
      if (entry.contains("clueColors") && entry["clueColors"].is_array()) {
        for (const auto& c : entry["clueColors"]) {
          if (c.is_string()) clue_colors.push_back(c.get<std::string>());
        }
      } else if (!no_colours && !all_colours && !st.prism &&
                  !st.rainbowish && !st.whitish) {
        clue_colors.push_back(name);
      }
      result.emplace(name, Suit{name, abbrev, st, std::move(clue_colors)});
    }
    return result;
  }();
  return cache;
}

Variant variant_from_json(const nlohmann::json& entry) {
  std::vector<std::string> suit_names;
  for (const auto& s : entry.at("suits")) suit_names.push_back(s.get<std::string>());

  auto get_opt_int = [&](const char* key) -> std::optional<int> {
    if (entry.contains(key) && !entry[key].is_null()) return entry[key].get<int>();
    return std::nullopt;
  };

  return make_variant(
      entry.at("id").get<int>(),
      entry.at("name").get<std::string>(),
      std::move(suit_names),
      get_opt_int("criticalRank"),
      entry.value("clueStarved", false),
      get_opt_int("specialRank"),
      entry.value("specialRankAllClueColors", false),
      entry.value("specialRankNoClueColors", false),
      entry.value("specialRankAllClueRanks", false),
      entry.value("specialRankNoClueRanks", false),
      entry.value("specialRankDeceptive", false),
      entry.value("scarceOnes", false),
      entry.value("funnels", false),
      entry.value("chimneys", false),
      entry.value("oddsAndEvens", false),
      entry.value("alternatingClues", false),
      entry.value("synesthesia", false),
      // Absent `clueRanks` means the full 1-5. Present-but-empty is the Number
      // Mute family, which offers no rank clues at all -- so `value()` with a
      // 1-5 default would be wrong, and the key has to be probed explicitly.
      entry.contains("clueRanks")
          ? entry.at("clueRanks").get<std::vector<int>>()
          : std::vector<int>{1, 2, 3, 4, 5});
}

const std::unordered_map<std::string, Variant>& load_variants() {
  static const std::unordered_map<std::string, Variant> cache = [] {
    std::unordered_map<std::string, Variant> result;
    (void)load_suit_catalog();
    const auto raw = nlohmann::json::parse(load_data_file("variants.json"));
    for (const auto& entry : raw) {
      Variant v = variant_from_json(entry);
      const std::string name = v.name;
      result.emplace(name, std::move(v));
    }
    return result;
  }();
  return cache;
}

const Variant& get_variant(const std::string& name) {
  const auto& vs = load_variants();
  auto it = vs.find(name);
  if (it == vs.end()) {
    throw std::out_of_range("Variant '" + name + "' not found");
  }
  return it->second;
}

}  // namespace hanabi
