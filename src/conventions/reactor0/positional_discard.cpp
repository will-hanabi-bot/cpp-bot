#include "hanabi/conventions/reactor0/positional_discard.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/convention.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "hanabi/endgame/forced_endgame.h"
#include "hanabi/endgame/helper.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor0 {

namespace {

bool inverted(const State& s, Identity i) {
  return s.variant->suits[i.suit_index].suit_type.inverted;
}

// A card of `player`'s that the TEAM can see certainly advances a stack: every
// reading of it, as the common knowledge holds it, plays (or, on an inverted suit,
// chucks) on the shared stacks. The test Bob makes of Alice, so Alice makes it of
// herself too.
bool common_certain_play(const Game& game, int player) {
  const State& s = game.state;
  const State shared = s.shared_view();
  for (int o : s.hands[player]) {
    const IdentitySet live = game.common.thoughts[o].possibilities();
    if (live.is_empty()) continue;
    const bool plays =
        live.forall([&](Identity i) { return !inverted(s, i) && shared.is_playable(i); });
    const bool chucks = s.clue_tokens < 8 && live.forall([&](Identity i) {
      return inverted(s, i) && shared.is_playable(i);
    });
    if (plays || chucks) return true;
  }
  return false;
}

// A card of `player`'s the team can see certainly plays (`common_certain_play`'s
// test, card by card), or -1.
int common_certain_card(const Game& game, int player) {
  const State& s = game.state;
  const State shared = s.shared_view();
  for (int o : s.hands[player]) {
    const IdentitySet live = game.common.thoughts[o].possibilities();
    if (live.non_empty() &&
        live.forall([&](Identity i) { return !inverted(s, i) && shared.is_playable(i); })) {
      return o;
    }
  }
  return -1;
}

// The reader's cards we can see play when their turn comes, as slot indices
// (0-based, slot 1 first). For Cathy, after Bob's certain play has landed.
std::vector<int> playable_slots(const Game& game, int alice, int reader) {
  const State& s = game.state;
  State at = s;
  const int bob = s.next_player_index(alice);
  if (reader != bob) {
    const int o = common_certain_card(game, bob);
    if (o >= 0) {
      if (const auto id = s.deck[o].id(); id && s.is_playable(*id)) at = s.with_play(*id);
    }
  }
  std::vector<int> out;
  for (std::size_t k = 0; k < s.hands[reader].size(); ++k) {
    const auto id = s.deck[s.hands[reader][k]].id();
    if (!id || inverted(s, *id) || !at.is_playable(*id)) continue;
    out.push_back(static_cast<int>(k));
  }
  return out;
}

// A seat still to act after `reader` in this final round holds the card after `id`.
bool unblocks_a_later_seat(const State& s, int reader, Identity id) {
  if (!s.endgame_turns) return false;
  const auto next = id.next();
  if (!next) return false;
  int seat = reader;
  for (int i = 0; i < s.num_players; ++i) {
    seat = s.next_player_index(seat);
    if (seat == s.current_player_index) break;  // back round to Alice: the round is over
    for (int o : s.hands[seat]) {
      const auto seen = s.deck[o].id();
      if (seen && *seen == *next) return true;
    }
  }
  return false;
}

// A stall: a legal clue to a seat other than the reader where there is one, so that
// it cannot read as a play clue on their last turn.
std::optional<PerformAction> stall_clue(const Game& game, int reader) {
  const State& s = game.state;
  if (!s.can_clue()) return std::nullopt;
  std::optional<PerformAction> to_reader;
  for (int i = 1; i < s.num_players; ++i) {
    const int target = (s.our_player_index + s.num_players - i) % s.num_players;
    for (const Clue& clue : s.all_valid_clues(target)) {
      const PerformAction a = clue.kind == ClueKind::COLOUR
                                  ? PerformAction{PerformColour{clue.target, clue.value}}
                                  : PerformAction{PerformRank{clue.target, clue.value}};
      if (target != reader) return a;
      if (!to_reader) to_reader = a;
    }
  }
  return to_reader;
}

}  // namespace

bool positional_position(const Game& game, int alice) {
  const State& s = game.state;
  if (!is_reactor0_family(game.convention)) return false;
  if (s.cards_left != 0) return false;
  // Alice's own turn counts in `endgame_turns`, so Bob still acts when it is 2+.
  if (!s.endgame_turns || *s.endgame_turns < 2) return false;
  if (alice < 0 || alice >= s.num_players) return false;
  // A reaction Alice owes is read from the slot she actions, so that is what her
  // discard says -- it is not positional, and as Alice we answer the reaction.
  if (!game.waiting.empty() && game.waiting.front().reacter == alice) return false;
  for (const auto& pr : game.pending_reactions) {
    if (pr && pr->reacter == alice) return false;
  }
  return !common_certain_play(game, alice);
}

// The seat a positional discard by `alice` speaks to (v23.7.0, the user's ruling):
// Bob -- unless Bob has a play the team can see is certain, which always comes
// first; then Cathy, if she still acts and has no such play herself; else nobody.
// Replay 2024746 T56-T57 (reactor0): Noah threw his slot 5 with the deck empty;
// will-bot69 knew its p5 and will-bot67 its r5, so the discard said nothing, but
// will-bot69 played its slot 5 -- a dead g3 -- instead of the p5.
std::optional<int> positional_reader(const Game& game, int alice) {
  const State& s = game.state;
  if (!positional_position(game, alice)) return std::nullopt;
  const int bob = s.next_player_index(alice);
  if (!common_certain_play(game, bob)) return bob;
  if (*s.endgame_turns < 3) return std::nullopt;  // Cathy has no turn left
  const int cathy = s.next_player_index(bob);
  if (cathy == alice || common_certain_play(game, cathy)) return std::nullopt;
  return cathy;
}

std::optional<PerformAction> positional_discard_signal(const Game& game) {
  const State& s = game.state;
  const int us = s.our_player_index;
  if (s.current_player_index != us) return std::nullopt;
  // Where Bob and Cathy would read our discard as a double one, it is not a single
  // positional discard (v23.13.0).
  if (double_reads(game, us)) return std::nullopt;
  const auto reader = positional_reader(game, us);
  if (!reader) return std::nullopt;
  if (!hanabi::endgame::certain_plays(game).empty()) return std::nullopt;
  if (s.clue_tokens >= 8) return std::nullopt;  // a discard is illegal here
  std::optional<int> best;
  bool best_unblocks = false;
  for (int k : playable_slots(game, us, *reader)) {
    if (k >= static_cast<int>(s.hands[us].size())) continue;  // no slot of ours to name it
    const bool unblocks =
        unblocks_a_later_seat(s, *reader, *s.deck[s.hands[*reader][k]].id());
    if (!best || (unblocks && !best_unblocks)) {
      best = k;
      best_unblocks = unblocks;
    }
  }
  if (!best) return std::nullopt;
  hanabi::logging::log_branch("reactor0.positional_discard_given",
                              {{"slot", *best + 1}, {"reader", *reader},
                               {"reader_order", s.hands[*reader][*best]}});
  return PerformAction{PerformDiscard{s.hands[us][*best]}};
}

std::optional<PerformAction> positional_gamble(const Game& game) {
  const State& s = game.state;
  const int us = s.our_player_index;
  if (s.current_player_index != us) return std::nullopt;
  const auto reader = positional_reader(game, us);
  const int bob = s.next_player_index(us);
  if (!reader || *reader != bob) return std::nullopt;  // Bob has a play of his own
  if (!hanabi::endgame::certain_plays(game).empty()) return std::nullopt;
  if (!playable_slots(game, us, bob).empty()) return std::nullopt;  // Bob has a play
  // The play is ours. A gamble that presses Discard (an inverted chuck) would read
  // as positional, so only the Play button is bet on.
  auto is_play = [](const std::optional<PerformAction>& a) {
    return a && std::holds_alternative<PerformPlay>(*a);
  };
  auto a = hanabi::endgame::required_play_action(game, /*narrow=*/false);
  if (!is_play(a)) {
    a = hanabi::endgame::gamble_on(
        game, s.playable_set.filter([&](Identity i) { return !inverted(s, i); }));
  }
  if (!is_play(a)) return std::nullopt;
  hanabi::logging::log_branch("reactor0.positional_gamble",
                              {{"order", std::get<PerformPlay>(*a).target}});
  return a;
}

PerformAction positional_guard(const Game& game, const PerformAction& chosen) {
  const State& s = game.state;
  const auto* d = std::get_if<PerformDiscard>(&chosen);
  if (!d) return chosen;
  const int us = s.our_player_index;
  if (s.current_player_index != us) return chosen;
  // A discard Bob and Cathy would read as a double one goes out only as the signal
  // (v23.13.0); otherwise a stall clue.
  if (double_reads(game, us)) {
    if (auto sig = positional_double_discard_signal(game); sig && *sig == chosen) {
      return chosen;
    }
    if (auto clue = stall_clue(game, -1)) {
      hanabi::logging::log_branch("tiiah.positional_double_guard",
                                  {{"replaced", d->target}});
      return *clue;
    }
    return chosen;
  }
  const auto reader = positional_reader(game, us);
  if (!reader) return chosen;  // nobody reads it: a discard is only a discard
  if (auto sig = positional_discard_signal(game); sig && *sig == chosen) return chosen;
  if (auto clue = stall_clue(game, *reader)) {
    hanabi::logging::log_branch("reactor0.positional_guard", {{"replaced", d->target},
                                                             {"by", "stall_clue"}});
    return *clue;
  }
  // A slot the reader does not hold names nothing.
  const auto& ours = s.hands[us];
  for (std::size_t k = s.hands[*reader].size(); k < ours.size(); ++k) {
    hanabi::logging::log_branch("reactor0.positional_guard",
                                {{"replaced", d->target}, {"by", ours[k]}});
    return PerformAction{PerformDiscard{ours[k]}};
  }
  return chosen;
}

std::optional<PerformAction> positional_play(const Game& game) {
  const State& s = game.state;
  if (s.cards_left != 0) return std::nullopt;
  for (int o : s.our_hand()) {
    if (game.meta[o].positional_play) return PerformAction{PerformPlay{o}};
  }
  return std::nullopt;
}

void read_positional_discard(const Game& prev, Game& game, const DiscardAction& action) {
  if (action.failed) return;  // a misplay pressed Play
  const State& ps = prev.state;
  const int alice = action.player_index_v;
  const auto reader = positional_reader(prev, alice);
  if (!reader) return;
  // A discard that throws a card called to discard says what that call says (one
  // that answers a reaction is ruled out with the position).
  if (action.order < static_cast<int>(prev.meta.size()) &&
      prev.meta[action.order].status == CardStatus::CALLED_TO_DISCARD) {
    return;
  }
  const auto& hand = ps.hands[alice];
  const auto it = std::find(hand.begin(), hand.end(), action.order);
  if (it == hand.end()) return;
  const std::size_t slot = static_cast<std::size_t>(it - hand.begin());
  if (slot >= ps.hands[*reader].size()) return;  // names nothing
  const int card = ps.hands[*reader][slot];
  game.with_meta(card, [](ConvData& m) {
    m.positional_play = true;
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
  });
  // What it can be: a playable, where its reading allows one.
  const IdentitySet live = game.common.thoughts[card].possibilities();
  const State shared = game.state.shared_view();
  const IdentitySet playable =
      live.filter([&](Identity i) { return !inverted(ps, i) && shared.is_playable(i); });
  if (playable.non_empty()) game.narrow_thought(card, playable);
  hanabi::logging::log_branch("reactor0.positional_discard_read",
                              {{"slot", static_cast<int>(slot) + 1}, {"reader", *reader},
                               {"order", card}});
}

// --- Throw It in a Hole endgame rulings (v23.13.0) -------------------------
// TODO: reactor0 and every variant -- both rules are scoped to Throw It in a Hole
// for now (the user's ruling, human diagnostic 2025488).

namespace {

bool tiiah_three_seats(const State& s) {
  return s.variant->throw_it_in_a_hole && s.num_players == 3;
}

// The distinct still-needed identities among `player`'s cards we can see, each
// with the slot (0-based) of its leftmost copy. On our own stacks.
std::vector<std::pair<Identity, int>> useful_cards(const State& s, int player) {
  std::vector<std::pair<Identity, int>> out;
  for (std::size_t k = 0; k < s.hands[player].size(); ++k) {
    const auto id = s.deck[s.hands[player][k]].id();
    if (!id || !s.is_useful(*id)) continue;
    if (std::any_of(out.begin(), out.end(),
                    [&](const auto& p) { return p.first == *id; })) {
      continue;
    }
    out.emplace_back(*id, static_cast<int>(k));
  }
  return out;
}

// The double discard's slot arithmetic: slots 1-5, wrapped mod 5 (0 is slot 5).
int wrap5(int slot) { return ((slot - 1) % 5 + 5) % 5 + 1; }

// How many still-needed identities no card in `seen`'s hands accounts for, on our
// stacks; -1 when an inverted suit still needs one (the double discard is for plain
// suits only).
int unaccounted_useful(const State& s, std::initializer_list<int> seen) {
  int n = 0;
  for (int ord = 0; ord < static_cast<int>(s.variant->suits.size()) * 5; ++ord) {
    const Identity i = Identity::from_ord(ord);
    if (!s.is_useful(i)) continue;
    if (s.variant->suits[i.suit_index].suit_type.inverted) return -1;
    bool held = false;
    for (int p : seen) {
      for (int o : s.hands[p]) {
        if (const auto id = s.deck[o].id(); id && *id == i) held = true;
      }
    }
    if (!held) ++n;
  }
  return n;
}

// What makes `reader` read a discard by `alice` as a double one, judged from the
// reader's own seat: Alice holds no card left to play, the other reader exactly one,
// and exactly one still-needed identity is unaccounted for -- the reader's own.
bool reader_reads_double(const State& s, int alice, int other) {
  return useful_cards(s, alice).empty() && useful_cards(s, other).size() == 1 &&
         unaccounted_useful(s, {alice, other}) == 1;
}

}  // namespace

std::optional<PerformAction> called_play_in_thin_endgame(const Game& game) {
  const State& s = game.state;
  const int us = s.our_player_index;
  if (!tiiah_three_seats(s) || s.current_player_index != us) return std::nullopt;
  if (s.pace() > 1 || s.cards_left < 3) return std::nullopt;
  const PlayerCalls calls = calls_of(game, us);
  int called = calls.reacter_ctp;
  if (called < 0 && !calls.receiver_ctp.empty()) called = calls.receiver_ctp.front();
  if (called < 0 || game.meta[called].status != CardStatus::CALLED_TO_PLAY ||
      !call_is_actionable(game, us, called)) {
    return std::nullopt;
  }
  // Bob's critical good cards: a still-needed identity of which every copy not yet
  // discarded is in his hand. Both copies of one count once.
  const int bob = s.next_player_index(us);
  int critical = 0;
  for (const auto& [id, slot] : useful_cards(s, bob)) {
    (void)slot;
    int held = 0;
    for (int o : s.hands[bob]) {
      if (const auto seen = s.deck[o].id(); seen && *seen == id) ++held;
    }
    const int left = s.card_count[id.to_ord()] -
                     static_cast<int>(s.discard_stacks[id.suit_index][id.rank - 1].size());
    if (held >= left) ++critical;
  }
  if (critical >= 2) return std::nullopt;
  hanabi::logging::log_branch("tiiah.thin_endgame_called_play",
                              {{"order", called}, {"bob_critical", critical}});
  return PerformAction{PerformPlay{called}};
}

bool double_reads(const Game& game, int alice) {
  const State& s = game.state;
  if (!double_position(game, alice)) return false;
  const int bob = s.next_player_index(alice);
  const int cathy = s.next_player_index(bob);
  // What each reader will see (`reader_reads_double`): the other holding exactly one
  // card left, and nothing left unaccounted beyond their own -- so, from here, both
  // holding exactly one and every still-needed identity in their hands.
  return useful_cards(s, bob).size() == 1 && useful_cards(s, cathy).size() == 1 &&
         unaccounted_useful(s, {bob, cathy}) == 0;
}

bool double_position(const Game& game, int alice) {
  const State& s = game.state;
  if (!tiiah_three_seats(s)) return false;
  if (alice < 0 || alice >= s.num_players) return false;
  if (s.cards_left > 1) return false;
  // Bob and Cathy both act after Alice: the deck is not yet empty (her draw empties
  // it, and the final round follows), or it is and the round still holds their turns.
  if (s.cards_left == 0 && (!s.endgame_turns || *s.endgame_turns < 3)) return false;
  if (!game.waiting.empty() && game.waiting.front().reacter == alice) return false;
  for (const auto& pr : game.pending_reactions) {
    if (pr && pr->reacter == alice) return false;
  }
  return true;
}

std::optional<PerformAction> positional_double_discard_signal(const Game& game) {
  const State& s = game.state;
  const int us = s.our_player_index;
  if (s.current_player_index != us || !double_position(game, us)) return std::nullopt;
  if (s.clue_tokens >= 8) return std::nullopt;  // a discard is illegal here
  if (!hanabi::endgame::certain_plays(game).empty()) return std::nullopt;
  const int bob = s.next_player_index(us);
  const int cathy = s.next_player_index(bob);
  const auto b = useful_cards(s, bob);
  const auto c = useful_cards(s, cathy);
  if (b.size() != 1 || c.size() != 1 || b[0].first == c[0].first) return std::nullopt;
  // No required play of ours: every still-needed identity is one of those two.
  for (int ord = 0; ord < static_cast<int>(s.variant->suits.size()) * 5; ++ord) {
    const Identity i = Identity::from_ord(ord);
    if (s.variant->suits[i.suit_index].suit_type.inverted) {
      if (s.is_useful(i)) return std::nullopt;
      continue;
    }
    if (s.is_useful(i) && i != b[0].first && i != c[0].first) return std::nullopt;
  }
  // Bob's card plays now, and Cathy's once his has landed.
  if (!s.is_playable(b[0].first)) return std::nullopt;
  if (!s.with_play(b[0].first).is_playable(c[0].first)) return std::nullopt;
  const int slot = wrap5(b[0].second + 1 + c[0].second + 1);
  if (slot > static_cast<int>(s.hands[us].size())) return std::nullopt;
  hanabi::logging::log_branch("tiiah.positional_double_discard_given",
                              {{"slot", slot}, {"bob_slot", b[0].second + 1},
                               {"cathy_slot", c[0].second + 1}});
  return PerformAction{PerformDiscard{s.hands[us][slot - 1]}};
}

bool read_positional_double_discard(const Game& prev, Game& game,
                                    const DiscardAction& action) {
  if (action.failed) return false;  // a misplay pressed Play
  const State& ps = prev.state;
  const int alice = action.player_index_v;
  if (!double_position(prev, alice)) return false;
  if (action.order < static_cast<int>(prev.meta.size()) &&
      prev.meta[action.order].status == CardStatus::CALLED_TO_DISCARD) {
    return false;
  }
  const auto& hand = ps.hands[alice];
  const auto it = std::find(hand.begin(), hand.end(), action.order);
  if (it == hand.end()) return false;
  const int d = static_cast<int>(it - hand.begin()) + 1;
  const int bob = ps.next_player_index(alice);
  const int cathy = ps.next_player_index(bob);
  const int us = ps.our_player_index;
  // As Alice we know what we gave; as a reader, the position must be one we can see
  // through: Alice holds nothing left to play, the other reader exactly one card, and
  // only our own is unaccounted for (`reader_reads_double`). Self-play read every
  // discard with one card in the deck as a double one where only the other reader's
  // count was checked, and the readers struck (v23.13.0 A/B).
  if (us != bob && us != cathy) return double_reads(prev, alice);
  const int other = us == bob ? cathy : bob;
  if (!reader_reads_double(ps, alice, other)) return false;
  const int slot = wrap5(d - (useful_cards(ps, other)[0].second + 1));
  if (slot > static_cast<int>(ps.hands[us].size())) return true;
  const int card = ps.hands[us][slot - 1];
  game.with_meta(card, [](ConvData& m) {
    m.positional_play = true;
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
  });
  hanabi::logging::log_branch("tiiah.positional_double_discard_read",
                              {{"slot", slot}, {"alice_slot", d}, {"order", card}});
  return true;
}

}  // namespace hanabi::reactor0
