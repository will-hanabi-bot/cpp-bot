#include "hanabi/conventions/variants/hole.h"

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"

namespace hanabi::reactor::variants {

State stacks_after_queued_plays(const Game& game,
                                std::optional<int> only_player,
                                std::optional<int> except_order,
                                const std::vector<int>* base) {
  // Not our own belief. A partner's superposed play advanced ours — we watched
  // the card go in — and advanced nobody else's, so walking from `game.state`
  // would have each seat simulate a different game. The views the caller hands
  // us are the MINIMUM across the worlds their seats' hole cards leave open
  // (§1e, v16.24.0; before that, "assume none of the superposed cards were
  // played").
  //
  // Which view, exactly, is the caller's to say: the SHARED one by default, and
  // the pairwise one when the walk is deciding what a clue between two named
  // seats means (§1.3).
  State hypo = base ? game.state.with_stacks(*base) : game.state.shared_view();
  std::vector<int> hands;
  if (only_player) {
    hands.push_back(*only_player);
  } else {
    for (int p = 0; p < hypo.num_players; ++p) hands.push_back(p);
  }

  bool advanced = true;
  while (advanced) {
    advanced = false;
    for (int p : hands) {
      // The hand from `game`, never from `hypo`: `hypo` is reassigned below, and
      // iterating one of its own vectors across that reassignment read freed
      // memory (v18.0.0). A simulated play moves no card, so the hands are the
      // same.
      for (int o : game.state.hands[p]) {
        if (except_order && o == *except_order) continue;
        auto id = game.state.deck[o].id();
        if (!id || !hypo.is_playable(*id)) continue;
        // Queued means the team is already committed to it: a standing call, or
        // a card its holder can name from empathy alone.
        const bool called = game.meta[o].status == CardStatus::CALLED_TO_PLAY;
        const IdentitySet live = game.common.thoughts[o].possibilities();
        const bool empathy_playable =
            live.non_empty() &&
            live.forall([&hypo](Identity i) { return hypo.is_playable(i); });
        if (!called && !empathy_playable) continue;
        hypo = hypo.with_play(*id);
        advanced = true;
      }
    }
  }
  return hypo;
}


bool has_known_play(const Game& game, int player) {
  const State& s = game.state;
  // Only what every seat computes ALIKE (v18.0.0): the card's clue-touch empathy
  // (`possible`, which every seat derives from the same public clues), judged on
  // the shared view. A card is a known play when every identity its touches still
  // allow is playable there.
  //
  // Inferences and a seat's own stacks are left out, because each of them can
  // differ between seats. The dispatch asks `has_standing_play` instead (v18.3.0),
  // whose touch arm is stricter since v20.3.0: none of the identities may be one
  // the hole could already hold (`is_standing_play`).
  const State shared = s.shared_view();
  for (int o : s.hands[player]) {
    const IdentitySet& touched = game.common.thoughts[o].possible;
    if (touched.non_empty() &&
        touched.forall([&shared](Identity i) { return shared.is_playable(i); })) {
      return true;
    }
  }
  return false;
}

bool has_standing_play(const Game& game, int player) {
  // A card called to play counts, not just one whose touches name it playable
  // (v18.2.0 role inversion, v18.3.0 the reverse reactive), when its call is one
  // every seat stamps alike: a CLUED call, which a stable clue stamps by its
  // nature, or a SETTLED one (v18.10.0) -- no longer urgent, like a receiver's
  // call once its reaction has been played. A PENDING reaction call (`urgent`) is
  // left out, being the kind a seat can stamp differently. Replay 2013645 T11:
  // yagami's T8 Blue called will-bot69's o12 as the b3, touched b1-b5; read
  // touch-only, yagami's 4 to will-bot67 was an ordinary reactive and will-bot69
  // blind-played an r3. Replay 2013963 T10: will-bot67's o17 held a settled,
  // unclued receiver's call; read clued-only, yagami's reverse-reactive finesse
  // (will-bot69's b2 into will-bot67's b3) read as a MISTAKE.
  for (int o : game.state.hands[player]) {
    if (is_standing_play(game, o)) return true;
  }
  return false;
}

bool possibly_in_the_hole(const Game& game, Identity id) {
  const State shared = game.state.shared_view();
  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    const ConvData& m = game.meta[o];
    const IdentitySet& team = m.shared_left.non_empty() ? m.shared_left : m.superposition;
    // A settled hole card (one identity) is already on the shared view.
    if (team.length() <= 1) continue;
    const bool reaches = team.exists([&](Identity j) {
      return j.suit_index == id.suit_index &&
             shared.playable_away(j) >= shared.playable_away(id);
    });
    if (reaches) return true;
  }
  return false;
}

bool is_standing_play(const Game& game, int order) {
  const State& s = game.state;
  if (order < 0 || order >= static_cast<int>(game.meta.size())) return false;
  // A SURE play (v20.3.0): every identity its touches allow is playable on the
  // shared view, and none of them could already be in the hole. The shared view
  // is the MINIMUM across the hole's worlds, so "playable there" alone is not
  // enough -- and there is no good touch on an ancillary touched card, so a card
  // that was touched but not called may be exactly such a dupe.
  //
  // Replay 2018428 T16: black's T7 1 had called will-bot69's o15 and only touched
  // o5, `{r1,y1,g1,b1,p1,ra1}`. Every 1 was playable on the shared view, so o5 put
  // the table in the reverse position, and black's next 1 to will-bot69 read as a
  // reverse reactive: will-bot67 blind-played its o11 as a g2 into a strike. But
  // o8 `{g1,b1}`, o13 `{r1,y1}` and o15 (any 1) could each hold one of those 1s.
  //
  // A card need not be called, and its identity need not be a singleton: a yellow
  // card filled in as a 1 is a sure play (the user's ruling), and so is a `{y1,g1}`
  // when nothing in the hole could be either.
  const IdentitySet& touched = game.common.thoughts[order].possible;
  const State shared = s.shared_view();
  if (touched.non_empty() && touched.forall([&](Identity i) {
        return shared.is_playable(i) && !possibly_in_the_hole(game, i);
      })) {
    return true;
  }
  // A call every seat stamps alike: clued, or settled (see `has_standing_play`).
  const ConvData& m = game.meta[order];
  if (m.status != CardStatus::CALLED_TO_PLAY) return false;
  return s.deck[order].clued || !m.urgent;
}

bool inverted_stable(const Game& prev, int giver, int target) {
  const State& s = prev.state;
  const int bob = s.next_player_index(giver);
  const int cathy = s.next_player_index(bob);
  if (cathy == giver || target != cathy) return false;
  // One position since v18.3.0: the reverse reactive keys on the same standing
  // play, so role inversion is the clue-to-Cathy half of it.
  return reverse_reactive_position(prev, giver);
}

bool reverse_reactive_position(const Game& prev, int giver) {
  const State& s = prev.state;
  const int bob = s.next_player_index(giver);
  const int cathy = s.next_player_index(bob);
  // Fewer than three seats leaves nobody to react.
  if (cathy == giver) return false;
  // A STANDING play, any called card included (v18.3.0). From v18.0.0 to v18.2.0
  // only a touch-known play counted, which turned the reverse reactive off in
  // practice: a stable colour clue calls a card whose touches still allow
  // unplayable identities. Human diagnostic 2013726 T30
  // (v18_human_vs_bot_diagnostics/2013726.md): black held a called r4 touched as
  // r1-r5, so blue's Brown to black -- a reverse-reactive finesse of green's n3
  // into black's n4, which v16.29.0 gave -- was not available. An uncalled card
  // counts only as a SURE play, one nothing in the hole could have made trash
  // (v20.3.0, replay 2018428; `is_standing_play`).
  return has_standing_play(prev, bob) && !has_standing_play(prev, cathy);
}

bool reverse_reactive(const Game& prev, int giver, int target) {
  if (!reverse_reactive_position(prev, giver)) return false;
  // Not once the deck is nearly out (v22.7.0): there a clue to Bob is a stable
  // stall. The clue to Cathy stays stable as before.
  if (reverse_reactive_off_late(prev)) return false;
  return target == prev.state.next_player_index(giver);
}

bool reverse_reactive_off_late(const Game& prev) {
  return prev.state.cards_left < prev.state.num_players;
}

bool reverse_reactive(const Game& prev, const ClueAction& action) {
  return reverse_reactive(prev, action.giver, action.target);
}

}  // namespace hanabi::reactor::variants
