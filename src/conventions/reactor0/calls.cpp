#include "hanabi/conventions/reactor0/calls.h"

#include <algorithm>
#include <cstdint>
#include <functional>

#include "hanabi/basics/card.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/variants/inverted.h"

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
bool is_chuckable(const Game& game, int order) {
  auto id = game.state.deck[order].id();
  if (!id) return false;
  const bool inverted = variants::is_inverted_id(game.state, *id);
  if (inverted) return game.state.is_playable(*id);
  return game.state.is_basic_trash(*id);
}

}  // namespace

ActionLists action_lists(const Game& game, int player) {
  ActionLists out;
  const PlayerCalls calls = calls_of(game, player);

  // --- the pitch list -----------------------------------------------------
  // Pitchable = every card the holder would press PLAY on: the receiver-CTP
  // deque, plus anything their own empathy already marks playable. The spec's
  // worked example includes a fully-clued g2 that no clue ever called, which is
  // exactly the second group.
  std::vector<int> pitchable = calls.receiver_ctp;
  // `thinks_playables`, not `obvious_playables`: the spec says "given all
  // empathy and inferences from Alice's point of view", which is the empathy
  // notion. `obvious_playables` additionally demands the card be touched, and
  // would drop a card whose identity the holder has worked out by elimination.
  for (int o : game.players[player].thinks_playables(game, player)) {
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
    if (st == CardStatus::CALLED_TO_DISCARD || is_chuckable(game, o)) {
      out.chuck.push_back(o);
    }
  }
  return out;
}

}  // namespace hanabi::reactor0
