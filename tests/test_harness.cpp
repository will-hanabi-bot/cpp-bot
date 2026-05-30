#include "test_harness.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <stdexcept>

#include <gtest/gtest.h>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/player.h"

namespace hanabi::test {

namespace {

constexpr int kSeededOrder = 61;

const char* const kNames[] = {"Alice", "Bob", "Cathy", "Donald", "Emily"};

int parse_player(const State& state, const std::string& name) {
  for (size_t i = 0; i < state.names.size(); ++i) {
    if (state.names[i] == name) return static_cast<int>(i);
  }
  std::string msg = "Player '" + name + "' not found in [";
  for (size_t i = 0; i < state.names.size(); ++i) {
    if (i) msg += ", ";
    msg += state.names[i];
  }
  msg += "]";
  throw std::invalid_argument(msg);
}

void seed_certain_map(Player& p, Identity id) {
  auto& bucket = p.certain_map[id.to_ord()];
  bucket.insert(bucket.begin(), MatchEntry{kSeededOrder, -1});
}

}  // namespace

// --- setup ---------------------------------------------------------------

Game setup(SetupOptions opts) {
  const Variant& v = get_variant(opts.variant_name);
  const int num_players = static_cast<int>(opts.hands.size());
  if (num_players == 0) throw std::invalid_argument("hands must be non-empty");
  for (const auto& h : opts.hands) {
    if (static_cast<int>(h.size()) > kHandSize[num_players]) {
      throw std::invalid_argument("hand size exceeds variant's hand size");
    }
  }

  std::vector<std::string> names;
  for (int i = 0; i < num_players; ++i) names.emplace_back(kNames[i]);

  TableOptions table_opts;
  table_opts.num_players = num_players;
  table_opts.variant_name = v.name;
  State state = State::create(std::move(names), /*our_player_index=*/0, v, std::move(table_opts));
  Game game = opts.constructor ? opts.constructor(0, std::move(state))
                                  : Game::create(0, std::move(state));
  game.catchup = true;

  // 1. Pre-seed play stacks.
  if (opts.play_stacks) {
    const auto& ps = *opts.play_stacks;
    if (ps.size() != v.suits.size()) {
      throw std::invalid_argument("play_stacks length mismatch");
    }
    std::vector<Identity> seeded_ids;
    for (size_t s = 0; s < ps.size(); ++s) {
      for (int r = 1; r <= ps[s]; ++r) seeded_ids.emplace_back(static_cast<int>(s), r);
    }
    for (Identity id : seeded_ids) ++game.state.base_count[id.to_ord()];
    game.state.play_stacks = ps;
    game.common.hypo_stacks = ps;
    for (auto& p : game.players) p.hypo_stacks = ps;
    for (Identity id : seeded_ids) {
      seed_certain_map(game.common, id);
      for (auto& p : game.players) seed_certain_map(p, id);
    }
  }

  // 2. Deal the hands.
  int order_counter = -1;
  for (int player_index = 0; player_index < num_players; ++player_index) {
    const auto& hand = opts.hands[player_index];
    // Scala reverses each hand: slot 1 = leftmost = newest = highest order.
    // We issue draws oldest-first so the final hand prepends to [newest..oldest].
    for (auto it = hand.rbegin(); it != hand.rend(); ++it) {
      ++order_counter;
      const std::string& short_ = *it;
      if (short_ == "xx") {
        game.handle_action(DrawAction{player_index, order_counter, -1, -1});
      } else {
        Identity id = game.state.expand_short(short_);
        game.handle_action(DrawAction{player_index, order_counter, id.suit_index, id.rank});
      }
    }
  }

  // 3. Apply pre-existing discards.
  for (const std::string& short_ : opts.discarded) {
    Identity id = game.state.expand_short(short_);
    game.state = game.state.with_discard(id, 99);
    seed_certain_map(game.common, id);
    for (auto& p : game.players) seed_certain_map(p, id);
  }

  // 4. Sanity check.
  const auto& me_thoughts = game.players[game.state.our_player_index].thoughts;
  for (Identity id : game.state.variant->all_ids()) {
    int visible = 0;
    for (const auto& hand : game.state.hands) {
      for (int o : hand) {
        if (me_thoughts[o].matches(id, /*infer=*/true)) ++visible;
      }
    }
    int count = game.state.base_count[id.to_ord()] + visible;
    if (count > game.state.card_count[id.to_ord()]) {
      std::ostringstream msg;
      msg << "Found " << count << " copies of " << game.state.log_id(id) << "!";
      throw std::invalid_argument(msg.str());
    }
  }

  // 5. Final state tweaks.
  State& s = game.state;
  IdentitySet all_ids_local = s.all_ids;
  IdentitySet playable_set = all_ids_local.filter(
      [&](Identity i) { return s.is_playable(i) && !s.is_basic_trash(i); });
  IdentitySet critical_set = all_ids_local.filter([&](Identity i) { return s.is_critical(i); });
  IdentitySet trash_set = all_ids_local.filter([&](Identity i) { return s.is_basic_trash(i); });
  s.cards_left = s.cards_left - s.score() - static_cast<int>(opts.discarded.size());
  s.current_player_index = static_cast<int>(opts.starting);
  s.clue_tokens = opts.clue_tokens;
  s.strikes = opts.strikes;
  s.playable_set = playable_set;
  s.critical_set = critical_set;
  s.trash_set = trash_set;

  if (opts.init) opts.init(game);
  game.elim();
  game.base = Game::Base{game.state, game.meta, game.players, game.common};
  game.catchup = false;
  return game;
}

// --- Action parsing ------------------------------------------------------

BaseClue str_to_clue(const State& state, std::string_view s) {
  if (s.size() == 1 && s[0] >= '1' && s[0] <= '5') {
    return BaseClue(ClueKind::RANK, s[0] - '0');
  }
  std::string lowered(s);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                  [](unsigned char c) { return std::tolower(c); });
  for (size_t i = 0; i < state.variant->suits.size(); ++i) {
    std::string suit_lower = state.variant->suits[i].name;
    std::transform(suit_lower.begin(), suit_lower.end(), suit_lower.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    if (suit_lower == lowered) return BaseClue(ClueKind::COLOUR, static_cast<int>(i));
  }
  throw std::invalid_argument("Colour '" + std::string(s) + "' not found in variant");
}

namespace {

const std::regex& clue_re() {
  static const std::regex r{R"(^(\w+) clues (\d|\w+) to (\w+)(?: \(slots? ((?:\d)(?:,\d)*)\))?$)"};
  return r;
}
const std::regex& play_re() {
  static const std::regex r{R"(^(\w+) plays (\w\d)(?: \(slot (\d)\))?$)"};
  return r;
}
const std::regex& discard_re() {
  static const std::regex r{R"(^(\w+) (discards|bombs) (\w\d)(?: \(slot (\d)\))?$)"};
  return r;
}

Action build_play_or_discard(const State& state, std::string_view raw,
                                const std::string& player_s, const std::string& short_,
                                const std::string& slot_s, bool is_play, bool failed) {
  int player_index = parse_player(state, player_s);
  Identity id = state.expand_short(short_);

  auto build = [&](int order) -> Action {
    if (is_play) return PlayAction{player_index, order, id.suit_index, id.rank};
    return DiscardAction{player_index, order, id.suit_index, id.rank, failed};
  };

  if (player_index != state.our_player_index) {
    std::vector<int> matching;
    for (int o : state.hands[player_index]) {
      if (state.deck[o].matches(id)) matching.push_back(o);
    }
    if (matching.empty()) {
      throw std::invalid_argument("Unable to find " + short_ + " in " +
                                    state.names[player_index] + "'s hand");
    }
    if (matching.size() == 1) {
      int order = matching.front();
      if (!slot_s.empty()) {
        int slot = std::stoi(slot_s);
        if (state.hands[player_index][slot - 1] != order) {
          throw std::invalid_argument("Identity " + short_ + " not in given slot");
        }
      }
      return build(order);
    }
    if (slot_s.empty()) {
      throw std::invalid_argument(
          "Ambiguous identity in '" + std::string(raw) + "', needs '(slot x)'");
    }
    int order = state.hands[player_index][std::stoi(slot_s) - 1];
    if (!state.deck[order].matches(id)) {
      throw std::invalid_argument("Identity not in given slot");
    }
    return build(order);
  }

  if (slot_s.empty()) {
    std::string verb = is_play ? "play" : "discard";
    throw std::invalid_argument(verb + " from us in '" + std::string(raw) +
                                  "' needs '(slot x)'");
  }
  int order = state.hands[state.our_player_index][std::stoi(slot_s) - 1];
  return build(order);
}

}  // namespace

Action parse_action(const State& state, std::string_view raw) {
  std::string raw_str(raw);
  std::smatch m;

  if (std::regex_match(raw_str, m, clue_re())) {
    const std::string giver_s = m[1];
    const std::string value_s = m[2];
    const std::string target_s = m[3];
    const std::string slots_s = m[4];
    if (!state.can_clue()) throw std::invalid_argument("Tried to clue with 0 tokens");
    if (giver_s == target_s) {
      throw std::invalid_argument(giver_s + " cannot clue themselves");
    }
    int giver = parse_player(state, giver_s);
    int target = parse_player(state, target_s);
    BaseClue clue = str_to_clue(state, value_s);

    if (target != state.our_player_index) {
      auto list_ = state.clue_touched(state.hands[target], clue.kind, clue.value);
      if (list_.empty()) {
        throw std::invalid_argument("No cards touched by clue " + value_s + " to " + target_s);
      }
      return ClueAction(giver, target, std::move(list_), clue);
    }
    if (slots_s.empty()) {
      throw std::invalid_argument("clue to us in '" + raw_str + "' needs '(slot x)'");
    }
    std::vector<int> list_;
    std::stringstream ss(slots_s);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
      list_.push_back(state.our_hand()[std::stoi(tok) - 1]);
    }
    return ClueAction(giver, target, std::move(list_), clue);
  }

  if (std::regex_match(raw_str, m, play_re())) {
    return build_play_or_discard(state, raw, m[1], m[2], m[3].matched ? m[3].str() : "",
                                    /*is_play=*/true, /*failed=*/false);
  }

  if (std::regex_match(raw_str, m, discard_re())) {
    const std::string player_s = m[1];
    const std::string verb = m[2];
    const std::string short_ = m[3];
    const std::string slot_s = m[4].matched ? m[4].str() : "";
    bool failed = verb == "bombs";
    if (state.clue_tokens == 8 && !failed) {
      throw std::invalid_argument("Tried to discard with 8 clue tokens");
    }
    return build_play_or_discard(state, raw, player_s, short_, slot_s,
                                    /*is_play=*/false, failed);
  }

  throw std::invalid_argument("Invalid action: " + raw_str);
}

// --- take_turn -----------------------------------------------------------

Game take_turn(Game game, std::string_view raw_action, std::string_view draw) {
  Action action = parse_action(game.state, raw_action);
  const int act_player = player_index(action);
  if (act_player != game.state.current_player_index) {
    throw std::invalid_argument(
        std::string("Expected ") + game.state.names[game.state.current_player_index] +
        "'s turn for action");
  }

  std::optional<Identity> draw_id;
  if (!draw.empty()) draw_id = game.state.expand_short(draw);

  // Cache state snapshot for cards_left/our_player_index checks below.
  const int cards_left_before = game.state.cards_left;
  const int next_order_before = game.state.next_card_order;
  const int turn_count_before = game.state.turn_count;
  const int our_index = game.state.our_player_index;
  const auto& base_count = game.state.base_count;
  const auto& card_count = game.state.card_count;

  game.catchup = true;
  game.handle_action(action);

  bool is_play_or_discard = std::holds_alternative<PlayAction>(action) ||
                             std::holds_alternative<DiscardAction>(action);

  if (is_play_or_discard) {
    if (draw_id && cards_left_before == 0) {
      throw std::invalid_argument("Cannot draw at 0 cards left");
    }
    if (!draw_id && act_player != our_index) {
      throw std::invalid_argument("Missing draw for " + game.state.names[act_player]);
    }
    if (draw_id) {
      // Visibility check vs card_count.
      int visible = 0;
      // Note: `game` has already been mutated by handle_action; use the post-action hands
      // but the pre-action base_count, like Python (which checks pre-handle state).
      for (const auto& hand : game.state.hands) {
        for (int o : hand) {
          if (game.players[our_index].thoughts[o].matches(*draw_id, /*infer=*/true)) ++visible;
        }
      }
      int count = base_count[draw_id->to_ord()] + visible;
      if (count + 1 > card_count[draw_id->to_ord()]) {
        throw std::invalid_argument("Too many copies of identity for draw");
      }
      game.handle_action(DrawAction{act_player, next_order_before, draw_id->suit_index,
                                       draw_id->rank});
    } else {
      game.handle_action(DrawAction{act_player, next_order_before, -1, -1});
    }
  } else if (draw_id) {
    throw std::invalid_argument("Unexpected draw for non-play/discard action");
  }

  game.handle_action(
      TurnAction{turn_count_before, game.state.next_player_index(act_player)});
  game.catchup = false;
  return game;
}

// --- pre_clue / fully_known ----------------------------------------------

namespace {

struct TestClue {
  ClueKind kind;
  int value;
  TestPlayer giver;
};

Game apply_pre_clue(Game game, TestPlayer player, int slot,
                      const std::vector<TestClue>& clues) {
  const State& state = game.state;
  int order = state.hands[static_cast<int>(player)][slot - 1];

  auto card_id = state.deck[order].id();
  if (card_id) {
    for (const auto& c : clues) {
      if (!state.variant->id_touched(*card_id, c.kind, c.value)) {
        throw std::invalid_argument("Pre-clue doesn't touch the underlying identity");
      }
    }
  }

  IdentitySet possibilities = IdentitySet::from_iter(state.variant->all_ids());
  possibilities = possibilities.filter([&](Identity i) {
    for (const auto& c : clues) {
      if (!state.variant->id_touched(i, c.kind, c.value)) return false;
    }
    return true;
  });

  game.state.deck[order].clued = true;
  game.state.deck[order].clues.clear();
  for (const auto& c : clues) {
    game.state.deck[order].clues.emplace_back(c.kind, c.value, static_cast<int>(c.giver), 0);
  }

  IdentitySet poss_copy = possibilities;
  game.common = game.common.with_thought(order, [&](const Thought& t) {
    Thought out = t;
    out.inferred = poss_copy;
    out.possible = poss_copy;
    return out;
  });
  return game;
}

}  // namespace

Game pre_clue(Game game, TestPlayer player, int slot,
                std::initializer_list<std::string> clues) {
  TestPlayer other =
      (player == TestPlayer::ALICE) ? TestPlayer::BOB : TestPlayer::ALICE;
  std::vector<TestClue> tc;
  for (const auto& raw : clues) {
    BaseClue bc = str_to_clue(game.state, raw);
    tc.push_back({bc.kind, bc.value, other});
  }
  return apply_pre_clue(std::move(game), player, slot, tc);
}

Game fully_known(Game game, TestPlayer player, int slot, std::string_view short_) {
  const State& state = game.state;
  int order = state.hands[static_cast<int>(player)][slot - 1];
  const Card& card = state.deck[order];
  Identity id = state.expand_short(short_);
  if (card.id() && *card.id() != id) {
    throw std::invalid_argument("Card at slot does not match short");
  }
  TestPlayer giver =
      (player == TestPlayer::ALICE) ? TestPlayer::BOB : TestPlayer::ALICE;

  int colour_value;
  if (state.variant->suits[id.suit_index].suit_type.prism) {
    colour_value = (id.rank - 1) % static_cast<int>(state.variant->colourable_suit_indices.size());
  } else {
    colour_value = id.suit_index;
  }
  std::vector<TestClue> clues = {
      {ClueKind::RANK, id.rank, giver},
      {ClueKind::COLOUR, colour_value, giver},
  };
  return apply_pre_clue(std::move(game), player, slot, clues);
}

// --- Assertion helpers ---------------------------------------------------

namespace {

const Player& perspective_player(const Game& game, std::optional<TestPlayer> according_to) {
  return according_to ? game.players[static_cast<int>(*according_to)] : game.common;
}

}  // namespace

void expect_infs(const Game& game, std::optional<TestPlayer> according_to,
                  TestPlayer target, int slot,
                  std::initializer_list<std::string> expected) {
  const auto& hand = game.state.hands[static_cast<int>(target)];
  ASSERT_GE(slot, 1) << "Slot " << slot << " doesn't exist";
  ASSERT_LE(slot, static_cast<int>(hand.size())) << "Slot " << slot << " doesn't exist";
  int order = hand[slot - 1];
  const Player& p = perspective_player(game, according_to);
  IdentitySet actual = p.thoughts[order].inferred;
  IdentitySet expected_set = IdentitySet::empty();
  for (const auto& s : expected) {
    expected_set = expected_set.add(game.state.expand_short(s));
  }
  ASSERT_EQ(actual, expected_set)
      << "Differing inferences (order " << order << "). Expected "
      << expected_set.bits() << ", got " << actual.bits();
}

void expect_poss(const Game& game, std::optional<TestPlayer> according_to,
                  TestPlayer target, int slot,
                  std::initializer_list<std::string> expected) {
  const auto& hand = game.state.hands[static_cast<int>(target)];
  ASSERT_GE(slot, 1);
  ASSERT_LE(slot, static_cast<int>(hand.size()));
  int order = hand[slot - 1];
  const Player& p = perspective_player(game, according_to);
  IdentitySet actual = p.thoughts[order].possible;
  IdentitySet expected_set = IdentitySet::empty();
  for (const auto& s : expected) {
    expected_set = expected_set.add(game.state.expand_short(s));
  }
  ASSERT_EQ(actual, expected_set)
      << "Differing possibilities (order " << order << "). Expected "
      << expected_set.bits() << ", got " << actual.bits();
}

void expect_status(const Game& game, TestPlayer target, int slot, CardStatus status) {
  const auto& hand = game.state.hands[static_cast<int>(target)];
  ASSERT_GE(slot, 1);
  ASSERT_LE(slot, static_cast<int>(hand.size()));
  int order = hand[slot - 1];
  ASSERT_EQ(game.meta[order].status, status)
      << "Differing status (order " << order << ")";
}

}  // namespace hanabi::test
