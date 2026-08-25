#include "hanabi/endgame/forced_endgame.h"

#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/endgame/helper.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::endgame {

namespace {

// The identity of `order` as the CURRENT PLAYER knows it, or nullopt when
// they cannot pin it to one card. `state.deck[o].id()` returns nullopt for
// one's own hand, so any rule that scans hands by deck identity is blind to
// CP's own cards — see the 5-lockout below, whose documented CP exemption was
// unreachable for exactly that reason (replay 1942668 T44).
//
// The intersection rather than either view alone is Rule 2's convention (see
// its comment): common alone can retain basic trash that good-touch never
// stripped, and the per-player view alone can be wider in test setups that
// seed common.thoughts without syncing per-player views.
std::optional<Identity> own_known_identity(const Game& game, int order) {
  const int cp = game.state.current_player_index;
  IdentitySet tight = game.common.thoughts[order].inferred.intersect(
      game.players[cp].thoughts[order].inferred);
  if (tight.length() != 1) return std::nullopt;
  return tight.head();
}

// The identity `order` holds for the purposes of the hand scans below: the
// real deck id when visible, else what CP knows about its own card.
std::optional<Identity> effective_identity(const Game& game, int order) {
  if (auto id = game.state.deck[order].id()) return id;
  if (game.state.holder_of(order) != game.state.current_player_index) {
    return std::nullopt;
  }
  return own_known_identity(game, order);
}

// Rule 1 — "5-lockout".
//
// Precondition: `cards_left == 1`, `play_stacks[suit] < 4`, and the suit's
// 5 still exists in some hand.
//
// Cycle offsets (relative to current player): `offset(p) = (p - cp + n) %
// n`. CP's current turn is "now", offset 0; after CP plays, the deck empties
// and the final round runs offsets `1..n-1` and then **CP again**, so CP's
// second opportunity is offset `n` — last of all.
//
// Rule fires for suit `S` iff every 4-holder has offset >= the 5-holder's
// offset. Same-hand counts (>=) and the "5-holder strictly precedes all
// 4-holders" case both fall under that single predicate. Two CP cases sit
// outside it, in opposite directions:
//   * CP holds a **4** → offset 0, playable now, so the rule does NOT fire:
//     playing it advances the stack and the 5 plays on its endgame turn.
//   * CP holds the **5** → offset `n`, since the stack is `< 4` and the 5
//     cannot be played this turn. No 4-holder can come later, so the rule
//     does NOT fire either.
//
// Holders are resolved with `effective_identity`, so CP's OWN hand counts
// whenever CP can pin the card to a single identity. Using `deck[o].id()`
// alone made the CP exemption above dead code — CP never sees its own cards,
// so it could never be a 4-holder. Replay 1942668 T44: CP held a called,
// empathy-known, playable Ice 4 at offset 0 while the Ice 5 sat at offset 1;
// unseen, the rule fired and forced a stall clue instead of the play.
bool five_lockout_fires(const Game& game, int suit) {
  const State& s = game.state;
  if (s.play_stacks[suit] >= 4) return false;

  Identity five{suit, 5};
  Identity four{suit, 4};

  std::optional<int> five_holder;
  std::vector<int> four_holders;
  for (int p = 0; p < s.num_players; ++p) {
    for (int o : s.hands[p]) {
      auto id = effective_identity(game, o);
      if (!id) continue;
      if (*id == five) five_holder = p;
      if (*id == four) four_holders.push_back(p);
    }
  }
  if (!five_holder) return false;  // 5 already discarded — rule N/A.

  int cp = s.current_player_index;
  int n = s.num_players;
  auto offset = [&](int p) { return (p - cp + n) % n; };

  // CP gets TWO opportunities, not one: this turn (offset 0) and — because
  // drawing the last card sets `endgame_turns = num_players`
  // (src/basics/game.cpp:328), decremented per action (`:250`, `:274`,
  // `:310`) until the game ends at 0 (src/basics/state.cpp:144) — one more
  // turn at the very END of the final round, offset `n`.
  //
  // Which one applies depends on the card. A **4** can be played right now, so
  // offset 0 is right and the CP-holds-the-4 exemption stands. A **5** cannot:
  // the rule's own precondition is `play_stacks[suit] < 4`, so the 5 is not
  // playable this turn and its opportunity is CP's final-round turn. Scoring
  // it at offset 0 made `offset(fh) < 0` unsatisfiable, so the rule fired
  // unconditionally whenever CP held the 5 — and it is precisely then that no
  // lockout can exist, since CP plays after every other holder's 4.
  // bug_report_4_1_0.txt 4.1.0a (replay 1957936 T41): stacks [5,5,5,1] with
  // CP holding Orange 2 and Orange 5, a winning chuck line available, and the
  // rule forcing a stall clue that short-circuited the solver.
  int five_offset = (*five_holder == cp) ? n : offset(*five_holder);
  for (int fh : four_holders) {
    if (offset(fh) < five_offset) return false;
  }
  return true;
}

// Rule 2 — "two-critical play".
//
// Precondition: `cards_left == 1`, `clue_tokens < num_players`, and the
// current player (CP) holds at least two singleton-critical cards in
// hand, with at least one of those also currently playable.
//
// Why play is forced. With `cards_left == 1`, CP has exactly two play
// turns remaining if they play now (this turn + the final-round turn that
// comes back around after the deck empties). Cluing or discarding burns
// one of those two play opportunities. The `clue_tokens < num_players`
// guard rules out the "team cycles clues to keep the deck at 1" stall —
// at least one of the next `n` turns must be a play or discard, which
// empties the deck before CP recovers the play they skipped. With two
// strictly critical cards in hand (each only one copy left in the game),
// every skipped play turn costs one of them permanently.
//
// Singleton check uses `common.thoughts[o].inferred ∩ players[cp]
// .thoughts[o].inferred`. Common alone can include basic-trash that
// good-touch hasn't stripped (`game.good_touch` stays false outside
// specific code paths). Per-player alone can be wider than common in
// test setups that seed common.thoughts directly without syncing
// per-player views (`fully_known` in test_forced_endgame.cpp). The
// intersection is the tight set both agree on. Replay 1899527 T47 —
// common inferred for will-bot69's slot 4 = {y1, y2, y4} (the rank-3-
// stack basic-trash y1/y2 aren't elim'd from common); per-player narrows
// to {y4} via good-touch / visibility. Intersection = {y4} singleton.
//
// Play-target tiebreaker. With multiple playable criticals, prefer the
// one whose play unblocks another critical play. The successor identity
// (rank+1, or rank-1 for reversed suits) is "unblocked" if another
// player holds it. Replay 1899527 T47 — slot 1 = r5 (critical playable,
// nothing in line after), slot 4 = y4 (critical playable, will-bot67
// holds y5 next-up). Playing r5 first leaves will-bot67 unable to play
// y5 in their endgame turn (still needs y4 to land first), losing y5
// permanently. Playing y4 first means y5 becomes playable, will-bot67
// plays y5 on their final-round turn, CP plays r5 on their final-round
// turn = full score.
std::optional<PerformAction> two_critical_play_action(const Game& game) {
  const State& s = game.state;
  int cp = s.current_player_index;
  const Player& me = game.players[cp];

  std::vector<std::pair<int, Identity>> singleton_critical;
  for (int o : s.hands[cp]) {
    IdentitySet tight = game.common.thoughts[o].inferred.intersect(
        me.thoughts[o].inferred);
    if (tight.length() != 1) continue;
    Identity id = tight.head();
    if (!s.is_critical(id)) continue;
    singleton_critical.push_back({o, id});
  }
  if (singleton_critical.size() < 2) return std::nullopt;

  std::vector<std::pair<int, Identity>> playable;
  for (const auto& [o, id] : singleton_critical) {
    if (s.is_playable(id)) playable.push_back({o, id});
  }
  if (playable.empty()) return std::nullopt;

  // Score each playable critical by whether its play unblocks a
  // successor held by another player. The successor identity for a
  // suit's reversed direction is rank-1 (Identity::prev); for normal
  // suits it's rank+1 (Identity::next). The unblock bonus only counts
  // when the successor itself is critical or useful (i.e., not already
  // basic-trash), since unblocking a trash card means nothing.
  auto successor = [&](Identity id) -> std::optional<Identity> {
    const auto& st = s.variant->suits[id.suit_index].suit_type;
    return st.reversed ? id.prev() : id.next();
  };
  auto unblock_score = [&](Identity id) -> int {
    auto succ = successor(id);
    if (!succ) return 0;
    if (s.is_basic_trash(*succ)) return 0;
    for (int p = 0; p < s.num_players; ++p) {
      if (p == cp) continue;
      for (int o : s.hands[p]) {
        auto deck_id = s.deck[o].id();
        if (deck_id && *deck_id == *succ) return 1;
      }
    }
    return 0;
  };

  auto best = playable.front();
  int best_score = unblock_score(best.second);
  for (size_t i = 1; i < playable.size(); ++i) {
    int score = unblock_score(playable[i].second);
    if (score > best_score) {
      best = playable[i];
      best_score = score;
    }
  }
  // The button that stacks the card. On an inverted (Orange / Dark Orange)
  // suit `is_playable` above means "a CHUCK advances the stack" — pressing
  // Play would pitch this singleton-critical card into the discard pile and
  // lose the point outright. Nothing downstream can correct it either: the
  // forced layer short-circuits the solver (the endgame fork in src/basics/decide.cpp).
  if (s.variant->suits[best.second.suit_index].suit_type.inverted) {
    return PerformAction{PerformDiscard{best.first}};
  }
  return PerformAction{PerformPlay{best.first}};
}

// Rule 3 — "sole holder of a blocking card".
//
// Precondition: `cards_left == 1`, `clue_tokens < num_players`, and CP knows
// they hold a currently playable identity X that NO other player holds, whose
// successor is still needed.
//
// Why play is forced. Nobody else can put X on its stack, so if CP does not
// play it now they must play it on their final-round turn instead -- and the
// successor above it can then never be played, because no turns remain after
// that. Playing X now is what buys the team the turn in which the successor
// lands.
//
// This is NOT Rule 2 with the criticality dropped, and the difference is the
// whole reason it exists. Rule 2 wants two SINGLETON-critical cards; replay
// 1966675 T26 has CP holding BOTH copies of the Red 4, so `is_critical(r4)` is
// false, Rule 2 never fires, and the bot discarded. Criticality was never the
// load-bearing property here -- being the only one who can play the card is.
// Holding two copies makes that MORE certain, not less.
//
// The `clue_tokens < num_players` guard is Rule 2's, for Rule 2's reason: it
// rules out the team cycling clues to hold the deck at 1, which would give CP
// the skipped play turn back.
//
// "No other player holds X" reads `state.deck` over the OTHER hands, which is
// giver-side knowledge CP genuinely has -- they can see every hand but their
// own. The last card of the deck may also be an X, and that is deliberately not
// checked: a copy drawn on the final round arrives too late for the successor
// either way, so it cannot make the play unnecessary.
std::optional<PerformAction> sole_holder_play_action(const Game& game) {
  const State& s = game.state;
  const int cp = s.current_player_index;
  const Player& me = game.players[cp];

  auto successor = [&](Identity id) -> std::optional<Identity> {
    const auto& st = s.variant->suits[id.suit_index].suit_type;
    return st.reversed ? id.prev() : id.next();
  };
  auto held_elsewhere = [&](Identity id) {
    for (int p = 0; p < s.num_players; ++p) {
      if (p == cp) continue;
      for (int o : s.hands[p]) {
        auto deck_id = s.deck[o].id();
        if (deck_id && *deck_id == id) return true;
      }
    }
    return false;
  };

  for (int o : s.hands[cp]) {
    // Same tight view Rule 2 uses: common alone can carry basic trash that
    // nothing has stripped, per-player alone can be wider in seeded fixtures.
    IdentitySet tight =
        game.common.thoughts[o].inferred.intersect(me.thoughts[o].inferred);
    if (tight.length() != 1) continue;
    Identity id = tight.head();
    if (!s.is_playable(id)) continue;
    if (held_elsewhere(id)) continue;
    auto succ = successor(id);
    if (!succ) continue;                       // top of its suit, nothing behind
    if (s.is_basic_trash(*succ)) continue;     // successor already played
    // ...and it has to still be OBTAINABLE. `is_basic_trash` only means "at or
    // below the stack", so it says nothing about a successor whose every copy
    // has been discarded; `max_ranks` is what carries that. Without this the
    // rule forces a play to buy a turn for a card that can never be played.
    if (succ->rank > s.max_ranks[succ->suit_index]) continue;
    // The button that stacks the card, as Rule 2 does: on an inverted suit
    // `is_playable` means "a CHUCK advances the stack", and pressing Play would
    // pitch it into the discard pile instead.
    if (s.variant->suits[id.suit_index].suit_type.inverted) {
      return PerformAction{PerformDiscard{o}};
    }
    return PerformAction{PerformPlay{o}};
  }
  return std::nullopt;
}

// Fallback: enumerate every (target, kind, value) triple and return the
// first one that legally touches at least one card in the target's hand.
// Used only if `Game::find_all_clues` returns empty (very rare — would
// mean every valid clue is a mistake or perfectly redundant; we still
// must give *something* since the rule forbids play/discard).
//
// Colour values iterate `colourable_suit_indices.size()`, not `suits
// .size()`. In Ambiguous variants multiple suits share a single clue
// colour (e.g. Tomato + Mahogany both clue with Red), so the count of
// valid colour clue values is the count of distinct clue colours.
// Mirrors the loop bound in `State::all_colour_clues` / `all_valid
// _clues` (src/basics/state.cpp). Surfaced as a server-side rejection
// for "Ambiguous (6 Suits)" (6 suits, 3 colours): the bot tried to
// send colour value 3 and the server warned
// "You cannot give a color clue with a value of \"3\".".
std::optional<PerformAction> any_legal_clue(const Game& game) {
  const State& s = game.state;
  int cp = s.current_player_index;
  for (int target = 0; target < s.num_players; ++target) {
    if (target == cp) continue;
    const int num_colours =
        static_cast<int>(s.variant->colourable_suit_indices.size());
    for (int v = 0; v < num_colours; ++v) {
      auto touched =
          s.clue_touched(s.hands[target], ClueKind::COLOUR, v);
      if (!touched.empty()) return PerformAction{PerformColour{target, v}};
    }
    for (int v = 1; v <= 5; ++v) {
      auto touched = s.clue_touched(s.hands[target], ClueKind::RANK, v);
      if (!touched.empty()) return PerformAction{PerformRank{target, v}};
    }
  }
  return std::nullopt;
}

}  // namespace

// Rule 0b -- the play that has to happen, when nothing in hand is certain.
//
// Replay 1970943 T24: stacks [3,5,5] with the deck empty and three turns left.
// p0's hand was visibly all trash; p1 visibly held BOTH r4 and r5 but had one
// turn, so whichever they played, the other died. The only line to 15 was for
// us to lay the r4 so p1 could cash the r5 -- and our slot 4 was clued and read
// {r2,r4,o1,o2,o3,o4}, so it COULD be that r4. It was. The bot chucked a known
// trash b1 instead and the game ended 13/15.
//
// The search cannot reach this. It generated our play and then pruned it,
// because `player_known_plays` -> `thinks_playables` subtracts known trash only
// from TOUCHED cards (src/basics/player_game.cpp:186): our slot 4 is clued so
// its reading collapsed to {r4} and was offered, while p1's r5 is UNCLUED so
// {r1..r5} never collapsed and p1 read as having no play. The line therefore
// looked unwinnable and the action was dropped (src/endgame/solver.cpp:168).
// The solver can imagine our gamble but not p1 cashing it. See TODO.md.
//
// WHEN IT FIRES. The deck must be empty, nothing in hand may be certain (Rule 0
// above already took that case, and a sure point outranks a gamble), and
// playing must actually raise the ceiling: `best_reachable_plays` prices the
// rest of the final round with full sight of every other hand, once as-is and
// once per currently-playable identity. An identity that lifts the ceiling is
// REQUIRED -- nobody else is going to cash it in time.
//
// At 1970943 T24 the baseline is 1 (p1 can lay the r4 themselves) and laying r4
// ourselves makes it 2 (p1 then lays the r5), so r4 is required.
//
// It deliberately carries NO strike guard: it fires even when a miss would be
// the game-ending third strike. On these turns the alternative is almost always
// a trash discard, and across the log corpus the rule picks the right card 11
// times in 17 where ground truth is recoverable.
//
// SELECTION is the reported convention: among our cards that could be a
// required identity, the leftmost CLUED one, else the leftmost of any. Clued
// first is load-bearing -- at T24 the leftmost candidate overall was slot 1, an
// omni 1, and only the clued slot 4 was the r4.
//
// The BUTTON is part of the answer. On an inverted (Orange / Dark Orange) suit
// the action that advances the stack is the chuck (PerformDiscard); pressing
// Play would pitch the card away. Paired exactly as `certainly_advances`
// (src/endgame/helper.cpp) pairs it, including its refusal of a chuck at 8
// tokens, where discarding is illegal.
//
// `narrow` is rule 0c: the same question asked one card earlier, where the
// answer is weaker and the candidate test has to be stronger. See the call
// sites.
std::optional<PerformAction> required_play_action(const Game& game, bool narrow) {
  const State& s = game.state;

  // The seats that act after us. With the deck already empty that is the rest
  // of the final round, and we are index 0 of the window and excluded -- the
  // whole question is what happens if we do NOT contribute.
  //
  // With ONE card left the round has not started: our play draws the last card,
  // which sets `endgame_turns = num_players` (src/basics/game.cpp:416), so the
  // window is the next `num_players` seats. Our own seat comes round again
  // inside it and is left in deliberately -- `best_reachable_plays` reads
  // `state.deck[o].id()` and our cards are hidden, so it contributes nothing.
  // That under-counts what the team can still reach, which is the safe
  // direction: it makes the ceiling harder to raise, not easier.
  std::vector<int> rest;
  if (s.endgame_turns) {
    for (int i = 1; i < *s.endgame_turns; ++i) {
      rest.push_back((s.current_player_index + i) % s.num_players);
    }
  } else if (s.cards_left == 1) {
    for (int i = 1; i <= s.num_players; ++i) {
      rest.push_back((s.current_player_index + i) % s.num_players);
    }
  } else {
    return std::nullopt;
  }

  const int base = best_reachable_plays(s, s.play_stacks, rest);
  IdentitySet required = s.playable_set.filter([&](Identity id) {
    std::vector<int> stacks = s.play_stacks;
    stacks[id.suit_index] = id.rank;
    return 1 + best_reachable_plays(s, stacks, rest) > base;
  });
  if (required.is_empty()) return std::nullopt;

  // The action that would lay `order`, or nullopt if it is not a candidate.
  auto attempt = [&](int order) -> std::optional<PerformAction> {
    const IdentitySet live = game.me().thoughts[order].possibilities();
    IdentitySet hits = live.intersect(required);
    if (hits.is_empty()) return std::nullopt;
    // Rule 0c only bets on a card it can very nearly name. With a card still in
    // the deck the ceiling test is a much weaker signal -- there is an extra
    // turn and an unseen draw -- so the candidate has to carry the confidence
    // instead: it must be CLUED, and everything it could be that is not already
    // trash must be the one required identity. Replay 1972670 T25's slot 4
    // reads {r2,r4,ra2,ra4} and the stacks kill all but the r4.
    if (narrow) {
      if (!s.deck[order].clued) return std::nullopt;
      const IdentitySet useful =
          live.filter([&s](Identity i) { return !s.is_basic_trash(i); });
      if (useful.length() != 1) return std::nullopt;
      if (!required.contains(useful.head())) return std::nullopt;
    }
    // Every reading we are betting on must want the SAME button, or there is no
    // single action that serves them -- the same reason a set spanning a plain
    // and an inverted suit is never a certain play.
    const bool inverted =
        s.variant->suits[hits.head().suit_index].suit_type.inverted;
    if (!hits.forall([&](Identity i) {
          return s.variant->suits[i.suit_index].suit_type.inverted == inverted;
        })) {
      return std::nullopt;
    }
    if (inverted) {
      if (s.clue_tokens >= 8) return std::nullopt;  // a discard is illegal here
      return PerformAction{PerformDiscard{order}};
    }
    return PerformAction{PerformPlay{order}};
  };

  // `our_hand()` runs newest-first, so the front IS slot 1 -- the leftmost.
  std::optional<PerformAction> leftmost, leftmost_clued;
  int chosen = -1;
  for (int order : s.our_hand()) {
    auto act = attempt(order);
    if (!act) continue;
    if (!leftmost) {
      leftmost = act;
      chosen = order;
    }
    if (s.deck[order].clued) {
      leftmost_clued = act;
      chosen = order;
      break;
    }
  }
  if (!leftmost && !leftmost_clued) return std::nullopt;
  hanabi::logging::log_branch(
      "endgame.required_play",
      {{"order", chosen}, {"clued", leftmost_clued.has_value()}});
  return leftmost_clued ? leftmost_clued : leftmost;
}

std::optional<PerformAction> forced_endgame_action(const Game& game) {
  hanabi::instr::ScopedTimer st("endgame.forced_endgame_action");
  hanabi::logging::LogScope ls("endgame.forced_endgame_action");
  const State& s = game.state;

  // Rule 0: we hold a card that certainly scores.
  //
  // A guaranteed point is a FORCED action, and a forced action takes precedence
  // over any conventional interpretation -- including a standing reacter call,
  // which `decide.cpp` honours immediately below this function. No clue anyone
  // gives can make a certain point worth less, and no search result can be
  // worth more than a point already in hand.
  //
  // `certain_plays` is "known from empathy or inferences": every reading
  // advances a stack, on the button that does so. That is wider than the
  // `id(infer=true)` singleton the solver's own trivial-win shortcut needs,
  // which is how replay 1969779 T68 came to gamble a trash card while holding
  // one read {a5, d5} with both those stacks on 4.
  //
  // The `cards_left == 0` gate is LOAD-BEARING, not incidental. With one card
  // still in the deck a guaranteed point is sometimes worth less than a stall,
  // and the solver is trusted to see it: replay 1874799 must stall rather than
  // play its null-5 or the team ends a point short, 1875304's winning line is a
  // stall clue that an urgent play must not shortcut, and 1885855 exists so the
  // 5-lockout BLOCKS an r5 play. Making this rule unconditional breaks all
  // three. With the deck empty none of that applies -- there is no future turn
  // the point could be traded for.
  //
  // It is nonetheless the only rule here not scoped to `cards_left == 1`, hence
  // its position above that gate.
  if (s.cards_left == 0) {
    auto certain = certain_plays(game);
    if (!certain.empty()) {
      // Prefer one carrying a standing call, then hand order -- the same
      // tie-break `prefer_certain_play` applies in `decide.cpp`.
      for (const auto& c : certain) {
        const int o = std::holds_alternative<PerformPlay>(c)
                          ? std::get<PerformPlay>(c).target
                          : std::get<PerformDiscard>(c).target;
        const CardStatus st = game.meta[o].status;
        if (st == CardStatus::CALLED_TO_PLAY || st == CardStatus::CALLED_TO_DISCARD) {
          return c;
        }
      }
      return certain.front();
    }
    // Rule 0b: a play is REQUIRED to improve the score, and none is certain.
    if (auto a = required_play_action(game, /*narrow=*/false)) return a;
  }

  // Rule 0c: the same required-play question with ONE card still in the deck.
  //
  // Replay 1972670 T25. Stacks [3,5,4] at 12 of 15, no clues, one card left.
  // will-bot69 held BOTH remaining playables -- the other r4 and the critical
  // ra5 -- but gets a single turn, and with zero clues there is no way to stall
  // round for a second one, so their r4 was dead whatever they did. Our slot 4
  // was the only copy that could be played, and playing it drew the last card,
  // let them cash the ra5, and left us a final turn for the r5: 15/15. The bot
  // discarded trash and the game ended at 12.
  //
  // Deliberately ABOVE the `cards_left == 1` gate below rather than inside it:
  // this is a play that must happen now, and the rules below can answer with a
  // stall clue. The narrow candidate test is what makes that safe to assert --
  // see `attempt`.
  if (s.cards_left == 1) {
    if (certain_plays(game).empty()) {
      if (auto a = required_play_action(game, /*narrow=*/true)) return a;
    }
  }

  if (s.cards_left != 1) return std::nullopt;

  // Rule 2 takes precedence over Rule 1: when both fire (e.g., CP holds the
  // suit's 4 and 5, both critical, 4 playable), playing the playable
  // critical card is the concrete winning move; the 5-lockout's "clue to
  // delay" would skip a play turn and lose a critical.
  if (s.clue_tokens < s.num_players) {
    if (auto a = two_critical_play_action(game)) return a;
    // Rule 3 after Rule 2: when both fire, Rule 2's tiebreak has already
    // reasoned about which critical to lead with.
    if (auto a = sole_holder_play_action(game)) return a;
  }

  if (s.clue_tokens == 0) return std::nullopt;

  bool any_lockout = false;
  for (int suit = 0; suit < static_cast<int>(s.variant->suits.size()); ++suit) {
    if (five_lockout_fires(game, suit)) {
      any_lockout = true;
      break;
    }
  }
  if (!any_lockout) return std::nullopt;

  auto clues = game.find_all_clues(s.current_player_index);
  if (!clues.empty()) return clues.front();
  return any_legal_clue(game);
}

}  // namespace hanabi::endgame
