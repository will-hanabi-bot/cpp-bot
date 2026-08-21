#include "hanabi/conventions/reactor0/calls.h"

#include <algorithm>
#include <cstdint>
#include <functional>

#include "hanabi/basics/card.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/conventions/variants/reversed.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor0 {

namespace variants = hanabi::reactor::variants;

PlayerCalls calls_of(const Game& game, int player) {
  PlayerCalls c;
  // `state.hands[p]` runs newest slot first, and rule 1 of
  // `enforce_call_invariants` keeps the CTP calls in play order along it, so a
  // single forward walk builds the deque already ordered.
  for (int o : game.state.hands[player]) {
    const ConvData& m = game.meta[o];
    switch (m.status) {
      case CardStatus::CALLED_TO_PLAY:
        if (m.urgent) {
          if (c.reacter_ctp < 0) c.reacter_ctp = o;
        } else {
          c.receiver_ctp.push_back(o);
        }
        break;
      case CardStatus::CALLED_TO_DISCARD:
        if (m.urgent) {
          if (c.reacter_ctd < 0) c.reacter_ctd = o;
        } else if (c.receiver_ctd < 0) {
          c.receiver_ctd = o;
        }
        break;
      default:
        break;
    }
  }
  return c;
}

bool depends_on(const Game& game, int player, int a_order, int b_order) {
  if (a_order == b_order) return false;
  // "b sits ahead of a in the deque". The deque runs newest slot first, and a
  // newer card has a strictly larger order, so "ahead" is "larger order".
  if (b_order <= a_order) return false;
  // Judged from the deciding player's OWN inferences, not common knowledge —
  // the spec says "(non-global)". `players[player]` is that view.
  const Player& view = game.players[player];
  const Thought& a = view.thoughts[a_order];
  const Thought& b = view.thoughts[b_order];
  const IdentitySet& as = a.inferred.non_empty() ? a.inferred : a.possible;
  const IdentitySet& bs = b.inferred.non_empty() ? b.inferred : b.possible;
  // Could they share a suit? If so, actioning `a` first risks stranding `b`.
  // Collapse each set to the suits it admits and intersect; that is O(|set|)
  // rather than the O(|set|^2) pairwise walk.
  std::uint32_t a_suits = 0, b_suits = 0;
  for (Identity i : as) a_suits |= 1u << i.suit_index;
  for (Identity i : bs) b_suits |= 1u << i.suit_index;
  return (a_suits & b_suits) != 0;
}

namespace {

// Is pressing DISCARD on this card safe and useful — the spec's "chuckable"?
// Trash on a plain suit throws away nothing; a playable card on an inverted
// suit advances its stack, because the inverted rule sends a physical Discard
// to the stacks.
//
// Judged from the HOLDER's view, never from the deck. The spec says "given all
// empathy and inferences from Alice's point of view", and reading
// `state.deck[order].id()` would let Alice chuck a card she has no way of
// knowing is trash — she cannot see her own hand.
//
// A pinned identity is not required: a card every one of whose possibilities is
// basic trash is known trash, even if the holder cannot say which trash it is.
bool is_chuckable(const Game& game, int player, int order) {
  const Thought& t = game.players[player].thoughts[order];
  const IdentitySet& set = t.inferred.non_empty() ? t.inferred : t.possible;
  if (set.is_empty()) return false;
  const State& s = game.state;
  // The spec's two arms: "either trash NON-INVERTED or playable inverted".
  //
  // The non-inverted half of the first arm is load-bearing, not decoration. A
  // card that is trash but could be ORANGE is not safe to chuck at all:
  // pressing Discard on an inverted card is a play attempt, and a trash orange
  // is by definition not playable, so the chuck STRIKES. Replay 1966569 T10 did
  // exactly that -- slot 4 was {r1, o1} with both trash, the card was the o1,
  // and chucking it took a strike. Such a card is pitched, never chucked.
  bool all_plain_trash = true;
  bool all_inverted_playable = true;
  for (Identity i : set) {
    const bool inverted = variants::is_inverted_id(s, i);
    if (!s.is_basic_trash(i) || inverted) all_plain_trash = false;
    if (!inverted || !s.is_playable(i)) all_inverted_playable = false;
  }
  return all_plain_trash || all_inverted_playable;
}

}  // namespace

namespace {

// Would pressing DISCARD on this card be a misplay? A chuck strikes exactly
// when the card is on an inverted suit and is not the next card for that stack;
// on a plain suit Discard always just discards. "Would strike" means EVERY
// remaining possibility strikes -- anything less and the holder still has a
// reading under which the call is sound.
bool chuck_would_strike(const Game& game, int player, int order) {
  const Thought& t = game.players[player].thoughts[order];
  const IdentitySet& set = t.inferred.non_empty() ? t.inferred : t.possible;
  if (set.is_empty()) return false;
  const State& s = game.state;
  for (Identity i : set) {
    if (!variants::is_inverted_id(s, i) || s.is_playable(i)) return false;
  }
  return true;
}

// The mirror. A pitch strikes exactly when the card is on a PLAIN suit and is
// not playable; on an inverted suit Play always just throws the card away.
bool pitch_would_strike(const Game& game, int player, int order) {
  const Thought& t = game.players[player].thoughts[order];
  const IdentitySet& set = t.inferred.non_empty() ? t.inferred : t.possible;
  if (set.is_empty()) return false;
  const State& s = game.state;
  for (Identity i : set) {
    if (variants::is_inverted_id(s, i) || s.is_playable(i)) return false;
  }
  return true;
}

}  // namespace

bool call_is_actionable(const Game& game, int player, int order) {
  switch (game.meta[order].status) {
    case CardStatus::CALLED_TO_DISCARD:
      return !chuck_would_strike(game, player, order);
    case CardStatus::CALLED_TO_PLAY:
      return !pitch_would_strike(game, player, order);
    default:
      return true;
  }
}

ActionLists action_lists(const Game& game, int player) {
  ActionLists out;
  const PlayerCalls calls = calls_of(game, player);

  // --- the pitch list -----------------------------------------------------
  // Pitchable = every card the holder would press PLAY on: the receiver-CTP
  // deque, plus anything their own empathy already marks playable. The spec's
  // worked example includes a fully-clued g2 that no clue ever called, which is
  // exactly the second group.
  // A standing CTP is ALWAYS pitchable, for the same reason, so the deque goes
  // in whole -- minus any call that has since become unpitchable.
  std::vector<int> pitchable;
  for (int o : calls.receiver_ctp) {
    if (call_is_actionable(game, player, o)) pitchable.push_back(o);
  }
  // `thinks_playables`, not `obvious_playables`: the spec says "given all
  // empathy and inferences from Alice's point of view", which is the empathy
  // notion. `obvious_playables` additionally demands the card be touched, and
  // would drop a card whose identity the holder has worked out by elimination.
  //
  // A possibly-INVERTED card is excluded, however playable empathy thinks it
  // is. Pressing Play on an inverted card is a PITCH -- it reaches the discard
  // pile -- so a playable orange is played by CHUCKING it, and `is_chuckable`
  // puts it on the other list for exactly that reason. Without this, replay
  // 1959065 pitches a called Dark Orange 2 (oneOfEach, so critical) rather than
  // chucking it onto its stack: the right card, the wrong button.
  //
  // This filters only the empathy group. An explicit call is exempt, because a
  // CTP stamped on an inverted card IS a deliberate pitch -- reactor0 stamps
  // CTD when it wants an inverted card played -- so the deque keeps its members
  // whatever their suit.
  for (int o : game.players[player].thinks_playables(game, player)) {
    if (variants::possible_has_inverted(
            game.state, game.players[player].thoughts[o].possible)) {
      continue;
    }
    if (std::find(pitchable.begin(), pitchable.end(), o) == pitchable.end()) {
      pitchable.push_back(o);
    }
  }
  // Newest slot first, matching the deque's own order.
  std::sort(pitchable.begin(), pitchable.end(), std::greater<int>());

  // Partition into dependence chains. Dependence is only defined inside the
  // receiver-CTP deque, so a card outside it can never join another's chain and
  // falls out as a singleton.
  const auto& deque = calls.receiver_ctp;
  auto in_deque = [&deque](int o) {
    return std::find(deque.begin(), deque.end(), o) != deque.end();
  };
  std::vector<bool> placed(pitchable.size(), false);
  for (size_t i = 0; i < pitchable.size(); ++i) {
    if (placed[i]) continue;
    std::vector<int> chain{pitchable[i]};
    placed[i] = true;
    if (in_deque(pitchable[i])) {
      // Walk to older cards, appending each that depends on the chain's front.
      for (size_t j = i + 1; j < pitchable.size(); ++j) {
        if (placed[j] || !in_deque(pitchable[j])) continue;
        if (depends_on(game, player, pitchable[j], chain.front())) {
          chain.push_back(pitchable[j]);
          placed[j] = true;
        }
      }
    }
    out.pitch.push_back(chain.front());
    out.pitch_chains.push_back(std::move(chain));
  }

  // --- the chuck list -----------------------------------------------------
  for (int o : game.state.hands[player]) {
    const CardStatus st = game.meta[o].status;
    // A standing CTD is ALWAYS chuckable -- the stamp is the instruction, and
    // the holder presses Discard on it until something proves that would
    // strike. `is_chuckable` is the test for cards carrying no call at all.
    if (st == CardStatus::CALLED_TO_DISCARD) {
      if (call_is_actionable(game, player, o)) out.chuck.push_back(o);
    } else if (is_chuckable(game, player, o)) {
      out.chuck.push_back(o);
    }
  }
  return out;
}

// =========================================================================
// Actionable Card Priority (DECISION_MAKING.md "Decision phase 2").
//
// Thirteen rungs, walked in order, first match wins. Rung 1 is absent by
// design -- see the header. Rungs 2-8 PITCH (press Play); 9-11 CHUCK (press
// Discard); 12-13 are the floor.
// =========================================================================

namespace {

// The identity Alice can pin for one of her own cards, or nullopt. Her own
// view, not common knowledge -- the spec says "given all empathy and
// inferences from Alice's point of view".
std::optional<Identity> alice_knows(const Game& game, int order) {
  return game.me().thoughts[order].id(/*infer=*/true);
}

// Does actioning this card set up a card Bob or Cathy is holding?
bool sets_up_a_partner(const Game& game, Identity id) {
  const State& s = game.state;
  const int alice = s.our_player_index;
  for (int p = 0; p < s.num_players; ++p) {
    if (p == alice) continue;
    if (connects_to_hand(game, id, p)) return true;
  }
  return false;
}

// `discard_button_is_safe`, which lives as a lambda inside `Game::take_action`
// (src/basics/decide.cpp) and so cannot be called from here. Same three
// clauses: an explicit discard call is always safe; a pinned identity is safe
// because the engine routes it correctly; otherwise the card must not be able
// to be on an inverted suit, since pressing Discard there is a play attempt.
bool chuck_button_is_safe(const Game& game, int order) {
  if (game.meta[order].status == CardStatus::CALLED_TO_DISCARD) return true;
  if (game.me().thoughts[order].id(/*infer=*/true)) return true;
  return !variants::possible_has_inverted(game.state,
                                          game.me().thoughts[order].possible);
}

}  // namespace

namespace {

// Every rung returns through here, so a trace always names the rung that fired
// and the button it pressed. Decision phase 2 had no branch logging at all
// until this was added, which made "why did it pitch that?" unanswerable from a
// log.
std::optional<PerformAction> taken(const Game& game, const char* rung, int order,
                                   bool pitch) {
  hanabi::logging::log_branch(
      "reactor0.choose_action",
      {{"rung", rung},
       {"order", order},
       {"button", pitch ? "play(pitch)" : "discard(chuck)"},
       {"clue_tokens", game.state.clue_tokens}});
  if (pitch) return PerformAction{PerformPlay{order}};
  return PerformAction{PerformDiscard{order}};
}

}  // namespace

std::optional<PerformAction> choose_action(const Game& game) {
  const State& s = game.state;
  const int alice = s.our_player_index;
  if (s.hands[alice].empty()) return std::nullopt;

  const ActionLists lists = action_lists(game, alice);

  // 2 -- pitch a card that sets up a card Bob or Cathy already holds.
  for (int o : lists.pitch) {
    auto id = alice_knows(game, o);
    if (id && sets_up_a_partner(game, *id)) return taken(game, "2.sets_up_partner", o, true);
  }

  // 3 -- chuck a KNOWN inverted card that does the same. Pressing Discard on
  // an inverted card advances its stack, so this is a play in all but name.
  for (int o : lists.chuck) {
    auto id = alice_knows(game, o);
    // Playability is the whole point: chucking an inverted card advances its
    // stack only if it is the next card. A trash orange chucked here strikes.
    if (id && variants::is_inverted_id(s, *id) && s.is_playable(*id) &&
        sets_up_a_partner(game, *id)) {
      return taken(game, "3.chuck_sets_up_partner", o, false);
    }
  }

  // 4 -- pitch the head of a chain that has dependants. Playing it is what
  // unblocks the rest of its chain, so it is worth more than a lone card.
  for (const auto& chain : lists.pitch_chains) {
    if (chain.size() > 1) return taken(game, "4.chain_head", chain.front(), true);
  }

  // 5, 6, 7 -- pitch a critical card, lowest first in play direction, then the
  // clue-regain rank. `direction_rank` folds the reversed-suit case away.
  auto pitch_critical = [&](const char* label,
                            auto&& pred) -> std::optional<PerformAction> {
    for (int o : lists.pitch) {
      auto id = alice_knows(game, o);
      if (id && s.is_critical(*id) && pred(*id)) return taken(game, label, o, true);
    }
    return std::nullopt;
  };
  if (auto a = pitch_critical(
          "5.critical_rank1",
          [&](Identity i) { return variants::direction_rank(s, i) == 1; })) {
    return a;
  }
  if (auto a = pitch_critical(
          "6.critical_rank2",
          [&](Identity i) { return variants::direction_rank(s, i) == 2; })) {
    return a;
  }
  if (auto a = pitch_critical(
          "7.critical_clue_regain",
          [&](Identity i) { return variants::is_clue_regain_rank(s, i); })) {
    return a;
  }

  // 8 -- the leftmost card of the lowest stack rank. `lists.pitch` is already
  // newest slot first, so a strict improvement keeps the leftmost on a tie.
  {
    int best = -1;
    int best_rank = 0;
    for (int o : lists.pitch) {
      auto id = alice_knows(game, o);
      if (!id) continue;
      const int r = variants::direction_rank(s, *id);
      if (best < 0 || r < best_rank) {
        best = o;
        best_rank = r;
      }
    }
    if (best >= 0) return taken(game, "8.lowest_stack_rank", best, true);
    // Nothing on the pitch list has a pinned identity. Its cards are still
    // playable by empathy, so pitching the leftmost is better than falling
    // through to a discard.
    if (!lists.pitch.empty()) return taken(game, "8.leftmost_unpinned", lists.pitch.front(), true);
  }

  // 9 -- chuck a known inverted card. No connection to set up, but it still
  // advances a stack.
  for (int o : lists.chuck) {
    auto id = alice_knows(game, o);
    if (id && variants::is_inverted_id(s, *id) && s.is_playable(*id)) {
      return taken(game, "9.chuck_known_inverted", o, false);
    }
  }

  // 10 -- chuck a card that COULD be inverted. Weaker than rung 9's "known",
  // and the distinction is the point: it might advance a stack.
  for (int o : lists.chuck) {
    if (variants::possible_has_inverted(s, game.me().thoughts[o].possible)) {
      return taken(game, "10.chuck_maybe_inverted", o, false);
    }
  }

  // 11 -- chuck the leftmost card on the list.
  if (!lists.chuck.empty()) return taken(game, "11.chuck_leftmost", lists.chuck.front(), false);

  // 12 / 13 -- the floor, both lists empty.
  //
  // The spec lists 13 after 12, but it is a condition on 12 rather than a rung
  // below it: at 8 clue tokens a discard is ILLEGAL, so 12's chop discard
  // cannot be taken and the chop is pitched instead. Reaching here at 8 tokens
  // also means section 4 found no clue, which is the case 13 describes.
  auto chop = game.chop(alice);
  if (!chop) {
    // A locked hand has no chop. Pitch the leftmost card rather than return
    // nothing -- `take_action` must produce a move.
    return taken(game, "12.locked_no_chop", s.hands[alice].front(), true);
  }
  if (s.clue_tokens == 8) return taken(game, "13.pitch_chop_at_eight", *chop, true);
  if (chuck_button_is_safe(game, *chop)) return taken(game, "12.discard_chop", *chop, false);
  // Neither button is safe for a card straddling an inverted and a plain suit
  // (decide.cpp says so at its own orange-safety filter). Pitching is the
  // lesser evil: it can lose the card, but it cannot strike.
  return taken(game, "12.pitch_chop_unsafe_button", *chop, true);
}

}  // namespace hanabi::reactor0
