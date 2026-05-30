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

// Pick a one-character short form, avoiding collisions. Port of _pick_short.
char pick_short(const std::string& sname,
                const std::unordered_map<std::string, Suit>& catalog,
                const std::vector<char>& taken) {
  if (sname == "Black") return 'k';
  if (sname == "Pink") return 'i';
  if (sname == "Brown") return 'n';

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
                     bool pink_s, bool brown_s, bool deceptive_s, bool scarce_ones) {
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
  v.suits.reserve(suit_names.size());
  v.short_forms.reserve(suit_names.size());

  for (size_t i = 0; i < suit_names.size(); ++i) {
    const auto& sname = suit_names[i];
    char short_c = pick_short(sname, catalog, v.short_forms);
    auto it = catalog.find(sname);
    Suit suit;
    if (it != catalog.end()) {
      suit = it->second;
    } else {
      suit = Suit{sname, short_c, SuitType::of_name(sname)};
    }
    v.suits.push_back(suit);
    v.short_forms.push_back(short_c);
    if (!is_no_colour(sname)) {
      v.colourable_suit_indices.push_back(static_cast<int>(i));
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
    if (st.prism) {
      return ((rank - 1) % colourable_suit_indices.size()) == static_cast<size_t>(value);
    }
    return suit == suits[colourable_suit_indices[value]];
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
      result.emplace(name, Suit{name, abbrev, SuitType::of_name(name)});
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
      entry.value("scarceOnes", false));
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
