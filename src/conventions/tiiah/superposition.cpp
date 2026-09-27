#include "hanabi/conventions/tiiah/superposition.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"

namespace hanabi::tiiah {

// An antecedent has narrowed, so every world it contradicts is gone — and with
// them the candidates that had no other world left to stand in (§1e).
//
// This is the half of the conditional reading that has to run at collapse time:
// producing the set is one job, and withdrawing the part of it that a later
// fact refutes is the other. Replay 2009367 T4: the `r2` on will-bot69's slot 4
// lives only in the world where the card it threw at T2 was the `r1`, so if that
// card ever settles on the `y1`, the `r2` goes with it.
bool refute_worlds(Game& game, int antecedent, const IdentitySet& still) {
  bool changed = false;
  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    if (!game.meta[o].conditional) continue;
    const ConvData::ConditionalReading& cond = *game.meta[o].conditional;

    std::uint64_t dead = 0;
    for (size_t w = 0; w < cond.worlds.size(); ++w) {
      for (const auto& [ord, id] : cond.worlds[w]) {
        if (ord == antecedent && !still.contains(id)) {
          dead |= (1ULL << w);
          break;
        }
      }
    }
    if (dead == 0) continue;

    // Compact the survivors, so a second refutation composes with this one.
    ConvData::ConditionalReading next;
    std::vector<int> remap(cond.worlds.size(), -1);
    for (size_t w = 0; w < cond.worlds.size(); ++w) {
      if (dead & (1ULL << w)) continue;
      remap[w] = static_cast<int>(next.worlds.size());
      next.worlds.push_back(cond.worlds[w]);
    }
    IdentitySet doomed = IdentitySet::empty();
    for (const auto& [id, mask] : cond.support) {
      std::uint64_t live = 0;
      for (size_t w = 0; w < cond.worlds.size(); ++w) {
        if ((mask & (1ULL << w)) && remap[w] >= 0) live |= (1ULL << remap[w]);
      }
      if (live == 0) {
        doomed = doomed.add(id);
      } else {
        next.support.emplace_back(id, live);
      }
    }

    const IdentitySet& live_set = game.meta[o].superposed()
                                      ? game.meta[o].superposition
                                      : game.common.thoughts[o].inferred;
    const IdentitySet kept = live_set.difference(doomed);
    // The same guard the shared rules carry: a reading is a reading, and one
    // refuted down to nothing is evidence that something else is wrong, not a
    // licence to assert a contradiction.
    if (doomed.non_empty() && kept.non_empty()) {
      if (game.meta[o].superposed()) {
        game.with_meta(o, [kept](ConvData& m) { m.superposition = kept; });
      } else {
        game.with_thought(o, [&kept](const Thought& t) {
          Thought out = t;
          out.old_inferred = t.inferred;
          out.inferred = kept;
          return out;
        });
      }
      changed = true;
    }
    const bool spent = next.worlds.size() <= 1 || next.support.empty();
    game.with_meta(o, [&next, spent](ConvData& m) {
      if (spent) {
        m.conditional.reset();
      } else {
        m.conditional = next;
      }
    });
  }
  return changed;
}


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

// Every copy of `id` is accounted for without looking into the hands of the
// seats in `blind` — the discard pile, the cards already spent, or a hand none of
// them holds — so the card we hold a superposition for cannot have been that
// identity.
//
// A seat cannot see its own hand, so `blind` is the seats whose sight this answer
// has to survive: `{us}` for what WE know, and `{us, p}` for what `p` and we both
// know (`all_copies_visible_to_pair`).
bool copies_accounted_for(const Game& game, int order, Identity id,
                          const std::vector<int>& blind) {
  const State& s = game.state;
  int unaccounted = s.card_count[id.to_ord()] - s.base_count[id.to_ord()];
  if (unaccounted <= 0) return true;
  for (int p = 0; p < s.num_players; ++p) {
    if (std::find(blind.begin(), blind.end(), p) != blind.end()) continue;
    for (int o : s.hands[p]) {
      if (o == order) continue;
      if (s.deck[o].id() == id) --unaccounted;
    }
  }
  return unaccounted <= 0;
}

// Our-seat-only by construction, and the shape of `reactor0::sight_narrowed`:
// it reads our own eyes, so it is knowledge no partner has. §1e keeps it out of
// the shared set for exactly that reason.
bool all_copies_visible(const Game& game, int order, Identity id) {
  return copies_accounted_for(game, order, id, {game.state.our_player_index});
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

// Advance every pairwise row that has just learned `id`.
//
// A hidden play is known to every seat EXCEPT the one who made it — they threw
// it in the hole — and to that seat as well when they could name it themselves.
// Each row is at its own height, so each is asked separately whether `id` is the
// next card for it; a row that has not seen the rank below is simply not ready
// and stays put (§1.3).
void advance_pairwise(Game& game, Identity id, int player, bool self_knew) {
  const State& s = game.state;
  if (s.pairwise_play_stacks.empty()) return;
  std::vector<int> knowers;
  for (int p = 0; p < s.num_players; ++p) {
    if (p == player && !self_knew) continue;
    if (p >= static_cast<int>(s.pairwise_play_stacks.size())) continue;
    if (!playable_on(s, s.pairwise_play_stacks[p], id)) continue;
    knowers.push_back(p);
  }
  if (knowers.empty()) return;
  game.with_state([&](State& st) { st = st.with_pairwise_play(id, knowers); });
}

// Advance ONE row, for evidence only that pair holds.
//
// `advance_pairwise` is every-seat-but-one, which is the right shape for a play
// somebody made: everyone watching it learns the same thing at the same moment.
// The pair form of rule 3 is not like that — it is a deduction one partner can
// follow and another cannot — so it moves a single row (§1.3).
bool advance_one_row(Game& game, Identity id, int p) {
  const State& s = game.state;
  if (p < 0 || p >= static_cast<int>(s.pairwise_play_stacks.size())) return false;
  if (!playable_on(s, s.pairwise_play_stacks[p], id)) return false;
  game.with_state([id, p](State& st) { st = st.with_pairwise_play(id, {p}); });
  return true;
}

// A collapsed card leaves the map, and the view it was hiding from advances.
// Our own card is the one our BELIEVED stacks never advanced for, because
// `resolve_hidden_action` could not name it; a partner's already advanced ours
// at play time and only the shared view is owed the update.
// A CLUE FRAME is frozen at clue time, and our own hole plays are the one thing that
// copy can be WRONG about (§1d, v16.21.0).
//
// `ReactorWC::clue_play_stacks` is our estimate of the stacks the giver chose the
// target in — `stacks_known_to_both(giver, receiver)` as it read when the clue landed.
// The giver could see the card we threw in the hole all along, so their frame already
// counted it; ours could not. Naming it therefore does not MOVE the frame, it restores
// what was frozen, and v12.0.0's reason for freezing at all — a deferral must be read
// as it was meant — is untouched.
//
// Replay 2011133 T16-T17 is the cost of leaving it stale. will-bot67's frozen frame had
// blue on 0 because it could not name the `b1` it played at T6; at T17 rule 6 named it,
// and without this the promise on its clued `b3` came out `{b2}` — already played — so
// `stamp_receiver_call`'s Rule 5 dropped the whole call and the b3 was never played.
//
// Raises only, and reversed-aware, like every other stack write in this file.
void correct_frozen_frames(Game& game, Identity id) {
  const auto& st = game.state.variant->suits[id.suit_index].suit_type;
  auto raise = [&](ReactorWC& wc) {
    if (id.suit_index >= static_cast<int>(wc.clue_play_stacks.size())) return;
    int& h = wc.clue_play_stacks[id.suit_index];
    if (st.reversed ? id.rank < h : id.rank > h) h = id.rank;
  };
  for (ReactorWC& wc : game.waiting) raise(wc);
  for (auto& pending : game.pending_reactions) {
    if (pending) raise(*pending);
  }
}

void settle(Game& game, int order, Identity id, bool shared) {
  const State& s = game.state;
  const bool ours = s.holder_of(order) == s.our_player_index;
  // Before the set is cleared below: anything whose reading leaned on this card
  // being something else has just lost that world (§1e).
  refute_worlds(game, order, IdentitySet::single(id));
  if (shared && playable_on(s, s.common_play_stacks, id)) {
    game.with_state([id](State& st) { st = st.with_common_play(id); });
  }
  if (shared) {
    // Evidence every seat shares, so every pairwise row learns it too. The
    // PRIVATE collapse below deliberately does not: `all_copies_visible` is our
    // own eyes, and a row may only hold what we know its seat holds.
    // `self_knew` makes the player argument moot — everyone learns it — which
    // is just as well, since the card left its hand when it was played.
    advance_pairwise(game, id, /*player=*/-1, /*self_knew=*/true);
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
    correct_frozen_frames(game, id);
  }
  game.with_meta(order, [](ConvData& m) { m.superposition = IdentitySet::empty(); });
}

// Keep only what the SURVIVING worlds still allow each of our hole cards to be,
// and settle anything that leaves at one identity.
//
// The second half of rule 6, shared by both of its forms: which worlds survive is
// the caller's question — "would this world let the partner's play land?" for
// `presume_play_lands`, "is this world free of a strike of our own?" for
// `presume_own_plays_land` — and this is what follows from either answer.
//
// `shared` is the caller's too, and the two forms differ on it (v16.21.0):
//
//   * a PARTNER's play is a public event, and its rescue rests on the candidate set
//     (built from `common`), the player's own reading of what they played, and the
//     no-strike rule — so every seat reaches the same answer and the shared stacks
//     move with our belief;
//   * our OWN plays are judged against our OWN belief, which no partner can
//     reproduce, so `presume_own_plays_land` stays private.
bool prune_to_worlds(Game& game, const std::vector<OpenWorld>& worlds,
                     const std::vector<const OpenWorld*>& surviving, bool shared) {
  if (worlds.empty() || surviving.empty()) return false;
  if (surviving.size() == worlds.size()) return false;  // nothing refuted

  std::vector<std::pair<int, IdentitySet>> narrowed;
  for (const auto& [ord, unused] : worlds.front().assignment) {
    (void)unused;
    IdentitySet allowed = IdentitySet::empty();
    for (const OpenWorld* w : surviving) {
      for (const auto& [o2, id2] : w->assignment) {
        if (o2 == ord) allowed = allowed.add(id2);
      }
    }
    if (allowed.is_empty()) continue;
    if (allowed == game.meta[ord].superposition) continue;  // nothing refuted
    narrowed.emplace_back(ord, allowed);
  }
  if (narrowed.empty()) return false;

  for (const auto& [ord, allowed] : narrowed) {
    game.with_meta(ord, [&allowed](ConvData& m) { m.superposition = allowed; });
    refute_worlds(game, ord, allowed);
    if (auto only = only_one(allowed)) settle(game, ord, *only, shared);
  }
  game.elim();
  return true;
}

// §1e's third source of evidence: a clue between two OTHER seats tells us what
// WE threw in the hole.
//
// A third seat cannot compute what a pair shares — the plays missing from the
// shared view are its own, and it cannot name them (§1.3). But it can SEE the
// card the pair called, and a call says "this is playable", so the pair's stack
// for that suit is one below the card. Anything that stack has above ours can
// only be our own hidden plays: nobody else's are missing from our belief.
//
// Replay 2008489 T33. yagami calls will-bot69's next blue and will-bot67 can see
// the card is a `b4`, so the two of them hold blue on 3. will-bot67 has it on 2,
// and the only blue play it cannot account for is its own — so the card it threw
// at T29 was the `b3`, and it learns its own stack is on 3.
//
// Narrow on purpose. Only a STABLE call qualifies: a reactive one can name a
// card that is one away rather than playable (a finesse), and then the arithmetic
// below says nothing. A rank that no superposition of ours can supply is left
// alone rather than forced.
bool back_solve_own_plays(Game& game, const Game& prev, const Action& action) {
  const auto* clue = std::get_if<ClueAction>(&action);
  if (!clue) return false;
  if (!game.waiting.empty()) return false;  // reactive: see above
  const int me = game.state.our_player_index;
  bool changed = false;

  for (size_t o = 0; o < game.meta.size() && o < prev.common.thoughts.size(); ++o) {
    const int owner = game.state.holder_of(static_cast<int>(o));
    if (owner < 0 || owner == me) continue;  // our own card: we cannot see it
    // Tied to THIS clue: the reading it just wrote is the evidence.
    if (game.common.thoughts[o].inferred == prev.common.thoughts[o].inferred) {
      continue;
    }
    auto read = only_one(game.common.thoughts[o].inferred);
    auto seen = game.state.deck[o].id();
    if (!read || !seen) continue;
    // Same suit, and they called a card further along than we thought the suit
    // had got. Anything else is not this inference at all.
    if (read->suit_index != seen->suit_index || read->rank >= seen->rank) continue;
    const auto& st = game.state.variant->suits[seen->suit_index].suit_type;
    if (st.reversed || st.inverted) continue;  // the arithmetic is plain-suit

    bool solved = false;
    for (int rank = read->rank; rank < seen->rank; ++rank) {
      const Identity missing{seen->suit_index, rank};
      for (size_t s = 0; s < game.meta.size(); ++s) {
        if (!game.meta[s].superposed()) continue;
        // OURS: `holders` keeps the seat that held a card after it has gone, so
        // this still names us for something we threw in the hole.
        if (game.state.holder_of(static_cast<int>(s)) != me) continue;
        if (!game.meta[s].superposition.contains(missing)) continue;
        settle(game, static_cast<int>(s), missing, /*shared=*/false);
        // Every other seat watched us play it, so their rows hold it too — we
        // simply could not write it down until now.
        advance_pairwise(game, missing, me, /*self_knew=*/false);
        solved = true;
        changed = true;
        break;
      }
    }
    if (!solved) continue;
    // Re-read the call now our own stacks have caught up with the pair's. The
    // suit is what the call named; which card of it is what we just learned.
    const State view = game.state.pairwise_view(owner);
    const int suit = seen->suit_index;
    const IdentitySet allowed = IdentitySet::create(
        [&](Identity i) { return i.suit_index == suit && view.is_playable(i); },
        static_cast<int>(game.state.variant->suits.size()) * 5);
    if (game.common.thoughts[o].possible.intersect(allowed).non_empty()) {
      game.narrow_thought(static_cast<int>(o), allowed);
    }
  }
  return changed;
}

}  // namespace

bool all_copies_visible_to_pair(const Game& game, int order, Identity id, int p) {
  if (p == game.state.our_player_index) return false;
  // Both of our hands are out: a copy in either is one the OTHER of us cannot
  // see, so it cannot be part of an answer we are claiming both of us reach.
  // Everything else -- the discard pile, the spent copies, the third seat's hand
  // -- we both read the same way, and each of us can see that the other does.
  //
  // One residue, and rule 6 is what keeps it small: the spent copies come from
  // OUR accounting, so a card `p` itself misplayed into the hole counts here
  // while `p` cannot name it. That requires us to have concluded `p` struck,
  // which rule 6 now refuses wherever a world without the strike is open.
  return copies_accounted_for(game, order, id, {game.state.our_player_index, p});
}

bool narrow_superposition(Game& game, int order, const IdentitySet& allowed) {
  if (order < 0 || order >= static_cast<int>(game.meta.size())) return false;
  if (!game.meta[order].superposed()) return false;
  const IdentitySet kept = game.meta[order].superposition.intersect(allowed);
  // The same guard the collapsing rules carry: a set refuted down to nothing says
  // something else is wrong, and is not a licence to assert a contradiction.
  if (kept.is_empty()) return false;
  if (kept == game.meta[order].superposition) return false;
  game.with_meta(order, [kept](ConvData& m) { m.superposition = kept; });
  refute_worlds(game, order, kept);
  if (auto only = only_one(kept)) settle(game, order, *only, /*shared=*/true);
  return true;
}

std::vector<OpenWorld> open_worlds(const Game& game, const State& base,
                                   const std::vector<int>& holders, int cap,
                                   int except_order) {
  std::vector<OpenWorld> out;
  out.push_back(OpenWorld{{}, base});
  if (!game.state.variant->throw_it_in_a_hole) return out;

  // Oldest first: they were played in that order, and a chain only lands if it
  // is replayed in it.
  std::vector<int> pending;
  std::size_t product = 1;
  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    if (!game.meta[o].superposed()) continue;
    if (o == except_order) continue;
    const int who = game.state.holder_of(o);
    if (std::find(holders.begin(), holders.end(), who) == holders.end()) continue;
    pending.push_back(o);
    product *= static_cast<std::size_t>(game.meta[o].superposition.length());
    if (product > static_cast<std::size_t>(cap)) return out;  // read it flat
  }

  for (int o : pending) {
    std::vector<OpenWorld> next;
    for (const OpenWorld& w : out) {
      for (Identity id : game.meta[o].superposition) {
        OpenWorld n = w;
        n.assignment.emplace_back(o, id);
        // A play that did not land struck instead, and the stacks stay put --
        // which is a world too, and `struck` is how rule 6 tells it apart.
        if (n.state.is_playable(id)) {
          n.state = n.state.with_play(id);
        } else {
          n.struck = true;
        }
        next.push_back(std::move(n));
      }
    }
    out = std::move(next);
  }
  return out;
}

std::vector<OpenWorld> open_worlds(const Game& game, const State& base,
                                   int holder, int cap, int except_order) {
  return open_worlds(game, base, std::vector<int>{holder}, cap, except_order);
}

namespace {

// The height every surviving world reaches, per suit. A stack only ever goes
// forward, so that is the least advanced of them -- which is `max` on a reversed
// suit, where the numbers run down.
std::vector<int> world_floor(const State& s,
                             const std::vector<const OpenWorld*>& surviving) {
  std::vector<int> floor;
  for (const OpenWorld* w : surviving) {
    if (floor.empty()) {
      floor = w->state.play_stacks;
      continue;
    }
    for (size_t k = 0; k < floor.size() && k < w->state.play_stacks.size(); ++k) {
      const bool rev = s.variant->suits[k].suit_type.reversed;
      floor[k] = rev ? std::max(floor[k], w->state.play_stacks[k])
                     : std::min(floor[k], w->state.play_stacks[k]);
    }
  }
  return floor;
}

}  // namespace

std::vector<const OpenWorld*> strike_free(const std::vector<OpenWorld>& worlds) {
  std::vector<const OpenWorld*> out;
  for (const OpenWorld& w : worlds) {
    if (!w.struck) out.push_back(&w);
  }
  if (!out.empty()) return out;
  // Every world has a strike in it, so the strike is not an assumption anybody
  // made -- it happened, and rule 6 has nothing to refute.
  for (const OpenWorld& w : worlds) out.push_back(&w);
  return out;
}

bool collapse_refused_target(Game& game, int giver, Identity gone) {
  if (!game.state.variant->throw_it_in_a_hole) return false;
  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    if (!game.meta[o].superposed()) continue;
    if (o >= static_cast<int>(game.state.holders.size())) continue;
    if (game.state.holders[o] != giver) continue;
    if (!game.meta[o].superposition.contains(gone)) continue;
    // Shared: the refusal is a public event, so this is not our own deduction
    // about a partner but the team's about all of them.
    settle(game, o, gone, /*shared=*/true);
    return true;
  }
  return false;
}

void presume_play_lands(Game& game, const Action& raw) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return;
  const auto* play = std::get_if<PlayAction>(&raw);
  if (!play || play->suit_index != -1) return;  // only a card that reached the hole
  const int me = s.our_player_index;
  if (play->player_index_v == me) return;  // our own: we cannot see it to judge
  const int order = play->order;
  if (order < 0 || order >= static_cast<int>(s.deck.size())) return;
  auto played = s.deck[order].id();
  if (!played) return;
  if (s.is_playable(*played)) return;  // it lands already; nothing to explain

  // Which of the worlds our own hole cards leave open would let it land? Chains
  // and the 64-world cap come from `open_worlds` (§1e); one world back means we
  // have nothing in the hole to blame and the strike is simply real.
  const auto worlds = open_worlds(game, s, me);
  if (worlds.size() <= 1) return;

  std::vector<const OpenWorld*> surviving;
  for (const OpenWorld& w : worlds) {
    if (w.state.is_playable(*played)) surviving.push_back(&w);
  }
  if (surviving.empty()) return;  // no world rescues it: a partner really misplayed

  // Everything the surviving worlds still allow each of our hole cards to be.
  // One identity left means we have just learned what we played.
  //
  // SHARED (v16.21.0). The evidence is a public play, the candidate set is built from
  // `common`, and "never presume a strike" is the convention rather than one seat's
  // opinion -- so every seat reaches the same conclusion about what WE threw in the
  // hole, and the shared stacks have to move with our belief. Replay 2011133 is what
  // the private form cost: will-bot67 alone knew blue was on 1, every view a clue is
  // read against still said 0, and its clued `b3` was never called for the rest of
  // the game.
  prune_to_worlds(game, worlds, surviving, /*shared=*/true);
}

void presume_discard_was_played(Game& game, const Action& raw) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return;
  const auto* dc = std::get_if<DiscardAction>(&raw);
  if (!dc || dc->suit_index == -1 || dc->rank == -1) return;
  const int me = s.our_player_index;
  if (dc->player_index_v == me) return;  // our own hole cards are what we cannot see
  const int order = dc->order;
  if (order < 0 || order >= static_cast<int>(game.common.thoughts.size())) return;
  const Identity id{dc->suit_index, dc->rank};
  if (!s.is_playable(id)) return;  // not playable to us: nothing to explain

  // The team must have NAMED it: a card merely touched may have been thrown for
  // any reason. The same test as `useful_dc` (decide.cpp), asked before the
  // discard reveals it.
  auto known = game.common.thoughts[order].id(/*infer=*/true, /*symmetric=*/true);
  if (!known || *known != id) return;

  const auto worlds = open_worlds(game, s, me);
  if (worlds.size() <= 1) return;

  std::vector<const OpenWorld*> surviving;
  for (const OpenWorld* w : strike_free(worlds)) {
    if (!w->state.is_playable(id) && w->state.is_basic_trash(id)) surviving.push_back(w);
  }
  if (surviving.empty()) return;  // no world has it played: a partner threw a useful card

  // Replay 2011319 T12: yagami threw the p1 our purple clue had called, because
  // she could see the p1 and p2 we had thrown in the hole. Read against purple 0,
  // it looked playable, and the gentleman's-discard reading pinned our only
  // unknown card to p1; at T14 we played it, a b2. Strike.
  prune_to_worlds(game, worlds, surviving, /*shared=*/true);
}

bool presume_own_plays_land(Game& game) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return false;
  // Our own hole cards alone: this is the rule read from our own seat, and what a
  // PARTNER can work out about theirs is `advance_rows_from_own_worlds`.
  const auto worlds = open_worlds(game, s, s.our_player_index);
  if (worlds.size() <= 1) return false;
  const auto surviving = strike_free(worlds);
  // PRIVATE: the base is our own belief, which no partner can reproduce.
  bool changed = prune_to_worlds(game, worlds, surviving, /*shared=*/false);

  // ...and the height every survivor reaches is one we HOLD, even when no single
  // card can be named (v16.19.0). Two cards each reading `{r1,y1}` were one of each,
  // so red and yellow are both on 1 — a fact about the stacks that no fact about
  // either card carries, and one the rows have had since v16.18.0 while our own
  // belief did not.
  //
  // Replay 2010512 is why it has to be our belief too: §1.3 gives the reacter its
  // OWN stacks to read what its card is, and on stacks two cards short the receiver's
  // `y2` looked one away, so the pairing read as a finesse demanding a `y1` the
  // reacter could not be holding. The call died on the spot.
  //
  // `with_stacks` and not `with_play`: the copies are NOT booked as spent, because we
  // cannot say WHICH card was the `r1`. A later collapse that names one settles it and
  // books it exactly once; booking here would double-count. So the accounting lags the
  // stacks by design, in the direction that only ever under-eliminates.
  const std::vector<int> floor = world_floor(s, surviving);
  std::vector<int> raised = s.play_stacks;
  bool moved = false;
  for (size_t k = 0; k < raised.size() && k < floor.size(); ++k) {
    const bool rev = s.variant->suits[k].suit_type.reversed;
    if (rev ? floor[k] >= raised[k] : floor[k] <= raised[k]) continue;
    raised[k] = floor[k];
    moved = true;
  }
  if (moved) {
    game.with_state([&raised](State& st) { st = st.with_stacks(raised); });
    changed = true;
  }
  return changed;
}

bool advance_rows_from_own_worlds(Game& game) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return false;
  if (s.pairwise_play_stacks.empty()) return false;
  const int me = s.our_player_index;
  bool changed = false;

  for (int p = 0; p < s.num_players; ++p) {
    if (p == me) continue;  // against ourselves there is nothing we do not know
    if (p >= static_cast<int>(s.pairwise_play_stacks.size())) continue;
    // `p`'s own hole cards AND ours. `p` watched ours leave our hand, so what `p`
    // knows rests on both, and a claim drawn from the row's base alone could
    // exceed what `p` believes (§1.3).
    const auto worlds = open_worlds(game, s.pairwise_view(p), {p, me});
    if (worlds.size() <= 1) continue;
    const auto surviving = strike_free(worlds);

    const std::vector<int> floor = world_floor(s, surviving);

    for (size_t k = 0; k < floor.size() && k < s.pairwise_play_stacks[p].size();
         ++k) {
      const int have = s.pairwise_play_stacks[p][k];
      const bool rev = s.variant->suits[k].suit_type.reversed;
      if (rev ? floor[k] >= have : floor[k] <= have) continue;  // rows only advance
      game.with_state([&](State& st) {
        st = st.with_pairwise_play(Identity{static_cast<int>(k), floor[k]}, {p});
      });
      changed = true;
    }
  }
  return changed;
}

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
    advance_pairwise(game, *known, play->player_index_v, /*self_knew=*/true);
    return;
  }
  if (candidates.is_empty()) return;
  // The player could not name it — but WE may still be able to, because we
  // watched the card leave their hand. That is the case the pairwise rows
  // exist for: everyone but the player knows this play, so every other seat's
  // row advances while the shared view stays put. Our own card is the one we
  // cannot name, and it contributes to nobody's row (seat p watched it, but we
  // cannot say what it was, so we cannot write it down).
  if (auto seen = game.state.deck[order].id()) {
    advance_pairwise(game, *seen, play->player_index_v, /*self_knew=*/false);
  }
  game.with_meta(order, [candidates](ConvData& m) { m.superposition = candidates; });
}

void collapse_superpositions(Game& game, const Game& prev, const Action& action) {
  if (!game.state.variant->throw_it_in_a_hole) return;
  if (game.meta.empty()) return;

  const Evidence ev = evidence_from(prev, game, action);
  bool changed = false;

  // To a FIXPOINT, because the rules feed each other: settling one card refutes
  // worlds on another, which can leave that one a singleton, which settles it in
  // turn. A single forward pass over the orders would only ever catch a cascade
  // that happened to run in increasing order.
  bool again = true;
  for (int pass = 0; again && pass < 8; ++pass) {
    again = false;
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
      again = true;
      game.with_meta(o, [shared](ConvData& m) { m.superposition = shared; });
      if (refute_worlds(game, o, shared)) changed = true;
    }
    if (auto only = only_one(shared)) {
      settle(game, o, *only, /*shared=*/true);
      changed = true;
      again = true;
      continue;
    }

    // --- the private rule, ours alone ------------------------------------
    //
    // It narrows what WE believe and never the set a partner predicts from, so
    // the stored set is left as it is and only our own stacks move.
    if (owner != game.state.our_player_index) continue;
    IdentitySet mine =
        shared.filter([&](Identity id) { return !all_copies_visible(game, o, id); });

    // ...and its PAIR form, read before the settle below clears the set. A
    // partner who can account for the same copies reaches the same answer, so
    // their row learns it -- one row rather than the shared view, because the
    // third seat may not be able to follow it (§1.3, v16.18.0).
    //
    // Replay 2010329: our order 3 was a `{r4,y1}` and both `r4` copies sit in
    // will-bot69's hand, so yagami rules the `r4` out exactly as we do and holds
    // yellow on 1 -- which is the frame its rank 2 at T14 has to be read in.
    for (int p = 0; p < game.state.num_players; ++p) {
      if (p == game.state.our_player_index) continue;
      const IdentitySet pair_set = shared.filter(
          [&](Identity id) { return !all_copies_visible_to_pair(game, o, id, p); });
      if (auto only = only_one(pair_set)) {
        if (advance_one_row(game, *only, p)) changed = true;
      }
    }

    if (auto only = only_one(mine)) {
      settle(game, o, *only, /*shared=*/false);
      changed = true;
      again = true;
    }
  }

  // --- rule 6, on our OWN plays ------------------------------------------
  //
  // Inside the loop: refuting the worlds that assume a strike of ours can leave a
  // card named, which is evidence the rules above then feed on.
  if (presume_own_plays_land(game)) {
    changed = true;
    again = true;
  }
  }

  if (back_solve_own_plays(game, prev, action)) changed = true;

  // The rows last, since every rule above can raise one: what a partner can work
  // out about its own hidden plays is not evidence anybody produced, it is what
  // follows from everything already known (§1.3).
  if (advance_rows_from_own_worlds(game)) changed = true;

  // A stack that moved changes what every hand could be holding.
  if (changed) game.elim();
}

}  // namespace hanabi::tiiah
