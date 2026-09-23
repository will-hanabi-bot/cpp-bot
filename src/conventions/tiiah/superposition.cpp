#include "hanabi/conventions/tiiah/superposition.h"

#include <optional>
#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"

namespace hanabi::tiiah {

namespace {

// The one identity in a set, or nullopt when it holds none or several.
std::optional<Identity> only_one(IdentitySet set) {
  if (set.length() != 1) return std::nullopt;
  return set.head();
}

// Is `id` the next card on `stacks`? The question `State::is_playable` asks of
// the believed stacks, asked of whichever view the caller hands it.
bool playable_on(const State& s, const std::vector<int>& stacks, Identity id) {
  if (stacks.empty()) return false;
  const auto& st = s.variant->suits[id.suit_index].suit_type;
  return st.reversed ? stacks[id.suit_index] - 1 == id.rank
                     : stacks[id.suit_index] + 1 == id.rank;
}

// Every copy of `id` is accounted for somewhere we can SEE — the discard pile,
// or a hand that is not ours — so the card we hold a superposition for cannot
// have been that identity.
//
// Our-seat-only by construction, and the shape of `reactor0::sight_narrowed`:
// it reads our own eyes, so it is knowledge no partner has. §1e keeps it out of
// the shared set for exactly that reason.
bool all_copies_visible(const Game& game, int order, Identity id) {
  const State& s = game.state;
  int unaccounted = s.card_count[id.to_ord()] - s.base_count[id.to_ord()];
  if (unaccounted <= 0) return true;
  for (int p = 0; p < s.num_players; ++p) {
    if (p == s.our_player_index) continue;  // we cannot see our own hand
    for (int o : s.hands[p]) {
      if (o == order) continue;
      if (s.deck[o].id() == id) --unaccounted;
    }
  }
  return unaccounted <= 0;
}

// What this action proved is STILL NEEDED, and who proved it.
//
// Both shared rules reduce to that one claim: an identity somebody just played,
// or that a clue just called to play, had not been played yet — so a card
// superposed earlier was not it.
//
// Only evidence every seat shares counts. A play whose own identity was a
// superposition is not common knowledge — the player who made it does not know
// what they played — so narrowing on it would desync them from the seats that
// watched it.
struct Evidence {
  std::vector<Identity> ids;
  int from = -1;  // the seat that produced it; its own cards are not narrowed
};

Evidence evidence_from(const Game& prev, const Game& game, const Action& action) {
  Evidence ev;
  if (const auto* play = std::get_if<PlayAction>(&action)) {
    ev.from = play->player_index_v;
    // The card went into the hole knowing what it was iff it was not stamped a
    // superposition on the way in.
    const bool was_superposed =
        play->order < static_cast<int>(game.meta.size()) &&
        game.meta[play->order].superposed();
    if (!was_superposed) {
      auto id = prev.state.deck[play->order].id();
      if (!id) id = only_one(prev.common.thoughts[play->order].possibilities());
      if (id) ev.ids.push_back(*id);
    }
    return ev;
  }
  if (const auto* clue = std::get_if<ClueAction>(&action)) {
    ev.from = clue->giver;
    // A card this clue newly called to play, whose identity every seat can
    // name. Calling it says the team still needs it.
    for (size_t o = 0; o < game.meta.size() && o < prev.meta.size(); ++o) {
      if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      if (prev.meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
      if (auto id = only_one(game.common.thoughts[o].possibilities())) {
        ev.ids.push_back(*id);
      }
    }
  }
  return ev;
}

// A collapsed card leaves the map, and the view it was hiding from advances.
// Our own card is the one our BELIEVED stacks never advanced for, because
// `resolve_hidden_action` could not name it; a partner's already advanced ours
// at play time and only the shared view is owed the update.
void settle(Game& game, int order, Identity id, bool shared) {
  const State& s = game.state;
  const bool ours = s.holder_of(order) == s.our_player_index;
  if (shared && playable_on(s, s.common_play_stacks, id)) {
    game.with_state([id](State& st) { st = st.with_common_play(id); });
  }
  if (ours) {
    const bool playable = s.is_playable(id);
    game.with_state([&](State& st) {
      // A card that did not land is gone all the same: `with_discard` is what
      // books the copy as spent and shrinks `max_ranks` if it was the last one.
      // It never reached the pile — nobody can see it — but our accounting has
      // to know, or every count built on `base_count` stays wrong forever.
      st = playable ? st.with_play(id) : st.with_discard(id, order);
      if (!playable) ++st.strikes;
    });
  }
  game.with_meta(order, [](ConvData& m) { m.superposition = IdentitySet::empty(); });
}

}  // namespace

void note_hidden_action(Game& game, const Action& raw) {
  if (!game.state.variant->throw_it_in_a_hole) return;
  // Only a card that reached the HOLE can be a superposition; a discard is
  // visible to the whole table, so nobody is left guessing.
  const auto* play = std::get_if<PlayAction>(&raw);
  if (!play || play->suit_index != -1) return;

  const int order = play->order;
  if (order < 0 || order >= static_cast<int>(game.common.thoughts.size())) return;

  // `common` is the one view every seat computes identically, which is what
  // lets all three track this set on the player's behalf. Read BEFORE the
  // action is dispatched: for a partner's card `on_play` is about to pin the
  // thought to the identity we could see and they could not.
  const IdentitySet candidates = game.common.thoughts[order].possibilities();

  if (auto known = only_one(candidates)) {
    // The player knew what they were playing, so every seat can follow it and
    // the shared view advances. Our own believed view is advanced by the
    // engine, which is handed the same identity.
    if (playable_on(game.state, game.state.common_play_stacks, *known)) {
      game.with_state([known](State& st) { st = st.with_common_play(*known); });
    }
    return;
  }
  if (candidates.is_empty()) return;
  game.with_meta(order, [candidates](ConvData& m) { m.superposition = candidates; });
}

void collapse_superpositions(Game& game, const Game& prev, const Action& action) {
  if (!game.state.variant->throw_it_in_a_hole) return;
  if (game.meta.empty()) return;

  const Evidence ev = evidence_from(prev, game, action);
  bool changed = false;

  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    if (!game.meta[o].superposed()) continue;
    const int owner = game.state.holder_of(o);

    // --- the shared rules ------------------------------------------------
    IdentitySet shared = game.meta[o].superposition;
    if (owner != ev.from) {
      for (Identity id : ev.ids) shared = shared.difference(id);
    }
    if (shared.is_empty()) {
      // Every candidate refuted. The set is a reading, not a fact, so keep the
      // old one rather than assert a contradiction into the model.
      continue;
    }
    if (shared != game.meta[o].superposition) {
      changed = true;
      game.with_meta(o, [shared](ConvData& m) { m.superposition = shared; });
    }
    if (auto only = only_one(shared)) {
      settle(game, o, *only, /*shared=*/true);
      changed = true;
      continue;
    }

    // --- the private rule, ours alone ------------------------------------
    //
    // It narrows what WE believe and never the set a partner predicts from, so
    // the stored set is left as it is and only our own stacks move.
    if (owner != game.state.our_player_index) continue;
    IdentitySet mine =
        shared.filter([&](Identity id) { return !all_copies_visible(game, o, id); });
    if (auto only = only_one(mine)) {
      settle(game, o, *only, /*shared=*/false);
      changed = true;
    }
  }

  // A stack that moved changes what every hand could be holding.
  if (changed) game.elim();
}

}  // namespace hanabi::tiiah
