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
  game.with_meta(order, [shared, id](ConvData& m) {
    // A private settle leaves the shared view's candidate set behind (v16.25.0):
    // no other seat followed us, so the shared worlds keep the card.
    if (!shared && m.shared_left.is_empty()) m.shared_left = m.superposition;
    if (shared) {
      m.shared_left = IdentitySet::empty();
      m.named_in_hole = IdentitySet::single(id);  // the team names it (v16.28.0)
    }
    m.superposition = IdentitySet::empty();
  });
}

// A card WE settled privately that a shared argument now names (v16.25.0): the
// shared view and every row learn it, our own belief already has it.
void settle_shared_only(Game& game, int order, Identity id) {
  const State& s = game.state;
  refute_worlds(game, order, IdentitySet::single(id));
  if (playable_on(s, s.common_play_stacks, id)) {
    game.with_state([id](State& st) { st = st.with_common_play(id); });
  }
  advance_pairwise(game, id, /*player=*/-1, /*self_knew=*/true);
  game.with_meta(order, [id](ConvData& m) {
    m.shared_left = IdentitySet::empty();
    m.named_in_hole = IdentitySet::single(id);
  });
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
    // The set these worlds were built from: the SHARED set when the argument is
    // shared and we have narrowed the card privately (v16.25.0).
    const ConvData& m0 = game.meta[ord];
    const IdentitySet& current =
        shared && m0.shared_left.non_empty() ? m0.shared_left : m0.superposition;
    if (allowed == current) continue;  // nothing refuted
    narrowed.emplace_back(ord, allowed);
  }
  if (narrowed.empty()) return false;

  for (const auto& [ord, allowed] : narrowed) {
    const bool superposed = game.meta[ord].superposed();
    if (!shared) {
      // A PRIVATE narrowing: remember what the shared view still allows first,
      // since no other seat followed us.
      game.with_meta(ord, [&allowed](ConvData& m) {
        if (m.shared_left.is_empty()) m.shared_left = m.superposition;
        m.superposition = allowed;
      });
      refute_worlds(game, ord, allowed);
      if (auto only = only_one(allowed)) settle(game, ord, *only, /*shared=*/false);
      continue;
    }
    // A SHARED narrowing: the shared set takes it, and our own set -- never wider
    // -- keeps what is left of it.
    game.with_meta(ord, [&allowed](ConvData& m) {
      if (m.shared_left.non_empty()) m.shared_left = allowed;
      if (m.superposed()) {
        const IdentitySet kept = m.superposition.intersect(allowed);
        if (kept.non_empty()) m.superposition = kept;
      }
    });
    if (superposed) refute_worlds(game, ord, game.meta[ord].superposition);
    if (auto only = only_one(allowed)) {
      if (game.meta[ord].superposed()) {
        settle(game, ord, *only, /*shared=*/true);
      } else {
        settle_shared_only(game, ord, *only);
      }
    } else if (game.meta[ord].superposed()) {
      if (auto mine = only_one(game.meta[ord].superposition)) {
        settle(game, ord, *mine, /*shared=*/false);
      }
    }
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

namespace {

// Every assignment, feasible or not -- `open_worlds` is this with the targeting
// rules applied, and `prune_infeasible_worlds` needs both to see what they refute.
bool s_reversed(const Game& game, int suit) {
  return game.state.variant->suits[suit].suit_type.reversed;
}

std::vector<OpenWorld> enumerate_worlds(const Game& game, const State& base,
                                        const std::vector<int>& holders, int cap,
                                        int except_order, bool shared = false,
                                        bool row = false) {
  // The candidate set a card carries in these worlds. The SHARED view also
  // carries the cards we settled privately, with the set it still allows.
  auto cands = [&game, shared](int o) -> const IdentitySet& {
    if (shared && game.meta[o].shared_left.non_empty()) return game.meta[o].shared_left;
    return game.meta[o].superposition;
  };
  std::vector<OpenWorld> out;
  out.push_back(OpenWorld{{}, base});
  if (!game.state.variant->throw_it_in_a_hole) return out;

  // Oldest first: they were played in that order, and a chain only lands if it
  // is replayed in it.
  std::vector<int> pending;
  std::size_t product = 1;
  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    if (cands(o).is_empty()) continue;
    if (o == except_order) continue;
    const int who = game.state.holder_of(o);
    if (std::find(holders.begin(), holders.end(), who) == holders.end()) continue;
    pending.push_back(o);
    product *= static_cast<std::size_t>(cands(o).length());
    if (product > static_cast<std::size_t>(cap)) return out;  // read it flat
  }
  // PLAY order, not card order (v16.24.0): a card drawn early can be played late,
  // and replaying it before a card that went in ahead of it strikes a world that
  // is sound. Replay 2011397: o8 went in at T11, after o9 at T8, and (o9 = b2,
  // o8 = b3) replayed as b3-then-b2 looked struck.
  std::stable_sort(pending.begin(), pending.end(), [&game](int a, int b) {
    return game.meta[a].hole_turn < game.meta[b].hole_turn;
  });

  // What the team has already NAMED in the hole (v16.28.0). A band rank is a
  // card the view holds without knowing which card it was; a named card is not
  // that, so a world card of a named identity is a duplicate, not the band card.
  IdentitySet named = IdentitySet::empty();
  for (const ConvData& m : game.meta) named = named.union_with(m.named_in_hole);

  // For a ROW: what the hole cards this replay leaves out -- settled by us in
  // private, so not enumerated here -- could still be to the PAIR: their shared
  // set, less what the pair rules out by sight (rule 3's pair form). Replay
  // 2010329: our o3 `{r4,y1}` cannot be the r4 to the pair, both r4s being in
  // will-bot69's hand, so it bridges nothing to a red 5.
  IdentitySet unseen_links = IdentitySet::empty();
  if (row) {
    int partner = -1;
    for (int h : holders) {
      if (h != game.state.our_player_index) partner = h;
    }
    for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
      if (game.meta[o].shared_left.is_empty() || cands(o).non_empty()) continue;
      const int who = game.state.holder_of(o);
      if (std::find(holders.begin(), holders.end(), who) == holders.end()) continue;
      for (Identity id : game.meta[o].shared_left) {
        if (partner >= 0 && all_copies_visible_to_pair(game, o, id, partner)) continue;
        unseen_links = unseen_links.add(id);
      }
    }
  }

  for (int o : pending) {
    std::vector<OpenWorld> next;
    for (const OpenWorld& w : out) {
      for (Identity id : cands(o)) {
        OpenWorld n = w;
        n.assignment.emplace_back(o, id);
        const int k = id.suit_index;
        // A card in the base's BAND -- above what the view can name, at or below
        // what it holds -- is one of the cards the view already counts without
        // knowing which card it was (v16.24.0). It is absorbed as that card,
        // once per rank, rather than read as a duplicate: a view raised by a
        // floor, or by a known card above a gap, is not evidence that this card
        // struck. Replay 2011327 T36: a row at blue 3 by floor, replayed with its
        // own `{b3,b4}` card, struck the b3 world and left only the b4.
        //
        // Except an identity the team already NAMED (`named`, v16.28.0). Replay
        // 2011885: will-bot69's known g3 landed at T16 above an unnamed g2, so the
        // shared view held green 3 with a band of 2-3; will-bot67's `{g3,n1}` at
        // T17 was absorbed as that g3 instead of striking as its duplicate, the
        // n1 never reached the shared view, and at T32 no reactive through the
        // brown suit could be read.
        const auto& band = base.band_floor;
        // The band is the BASE's -- between its evidence and its own height --
        // not whatever this world has since played onto it.
        const bool in_band =
            k < static_cast<int>(band.size()) &&
            (s_reversed(game, k) ? id.rank < band[k] &&
                                       id.rank >= base.play_stacks[k]
                                 : id.rank > band[k] &&
                                       id.rank <= base.play_stacks[k]);
        if (n.state.is_playable(id)) {
          n.state = n.state.with_play(id);
        } else if (in_band && !named.contains(id)) {
          if (n.absorbed.empty()) n.absorbed.assign(band.size(), 0u);
          const unsigned bit = 1u << id.rank;
          if (n.absorbed[k] & bit) {
            n.struck = true;  // two cards cannot both be the one band card
          } else {
            n.absorbed[k] |= bit;
          }
        } else if (row && !s_reversed(game, k) &&
                   id.rank > n.state.play_stacks[k] + 1 &&
                   unseen_links.contains(Identity{k, n.state.play_stacks[k] + 1})) {
          // A PAIRWISE ROW, and a card too HIGH to land -- but the card it is
          // waiting for could be one of the hole cards this replay leaves out,
          // because we settled them privately and the row never learned which
          // they were (v16.28.0). That is not a strike the row can see; the
          // world stays, and nothing lands. Replay 2011887 T18: will-bot69's
          // own o6 was the g1, settled privately; replayed without it, its
          // o9 = g2 struck on green 0, only o9 = r2 survived, and the row for
          // will-bot67 claimed red 2 with red really on 1.
        } else {
          // A play that did not land struck instead, and the stacks stay put --
          // which is a world too, and `struck` is how rule 6 tells it apart.
          n.struck = true;
        }
        next.push_back(std::move(n));
      }
    }
    out = std::move(next);
  }
  return out;
}

}  // namespace

std::vector<OpenWorld> open_worlds(const Game& game, const State& base,
                                   const std::vector<int>& holders, int cap,
                                   int except_order, bool shared, bool row) {
  std::vector<OpenWorld> out =
      enumerate_worlds(game, base, holders, cap, except_order, shared, row);
  // Only the worlds the targeting rules allow (§1e, v16.24.0) -- every one of them
  // kept when none survives, since the evidence is then contradicting itself and
  // is not a licence to assert that.
  std::vector<OpenWorld> feasible;
  for (const OpenWorld& w : out) {
    if (world_feasible(game, w)) feasible.push_back(w);
  }
  if (!feasible.empty() && feasible.size() != out.size()) return feasible;
  return out;
}

std::vector<OpenWorld> open_worlds(const Game& game, const State& base,
                                   int holder, int cap, int except_order, bool shared) {
  return open_worlds(game, base, std::vector<int>{holder}, cap, except_order, shared);
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

void record_conditional(Game& game, int order, const std::vector<OpenWorld>& worlds,
                        const std::vector<std::pair<Identity, std::uint64_t>>& support) {
  if (worlds.size() <= 1) return;
  const std::uint64_t all =
      (worlds.size() >= 64) ? ~0ULL : ((1ULL << worlds.size()) - 1);
  ConvData::ConditionalReading cond;
  for (const auto& [id, mask] : support) {
    if ((mask & all) == all) continue;  // every world agrees: no condition
    cond.support.emplace_back(id, mask);
  }
  if (cond.support.empty()) return;
  for (const OpenWorld& w : worlds) cond.worlds.push_back(w.assignment);
  game.with_meta(order, [&cond](ConvData& m) { m.conditional = cond; });
}

// WORLD FEASIBILITY (§1e, v16.24.0). A world assigns an identity to each card in
// the hole; it is only the world we are in if every reactive play clue those
// cards were given could have produced the answer we watched.
//
// The target walk (`receiver_targets`) takes a DIRECT playable before a one-away
// finesse, and each leftmost first. So for a recorded reaction whose receiver
// held two or more of this world's cards: the called card, if it is one of them,
// must be direct or one away in this world; no other of them may be a direct
// playable to its left, nor a direct playable at all when the called card is a
// finesse; and none may be a finesse to its left when the called card is one too.
// A card that is NOT in the hole is judged only by what that leaves certain -- we
// cannot name it, so it constrains nothing.
//
// The frame is the shared view at clue time, what the receiver could reconstruct
// of the walk, and the same at every seat.
//
// Replay 2011397 T6: will-bot67's Green reactive, and yagami's slot 1 called
// will-bot69's slot 2. o9 and o8, slots 2 and 3, later went into the hole as
// {b2,p2} and {b2,b3}. In the world (p2, b2) slot 2 is a finesse and slot 3 a
// direct playable, so yagami would have called slot 3 -- that world is refuted,
// and the cards were the b2 and the b3.
bool world_feasible(const Game& game, const OpenWorld& world) {
  // A joint fact rules 7 and 8 proved (v16.25.0): one of these cards WAS `id`.
  // Judged only when the world assigns every one of them.
  for (const HoleRequirement& req : game.hole_requirements) {
    bool all_there = !req.orders.empty();
    bool met = false;
    for (int o : req.orders) {
      bool there = false;
      for (const auto& [ao, aid] : world.assignment) {
        if (ao != o) continue;
        there = true;
        if (aid == req.id) met = true;
      }
      if (!there) all_there = false;
    }
    if (all_there && !met) return false;
  }
  if (world.assignment.size() < 2) return true;
  const State& s = game.state;
  for (const ReactionRecord& r : game.reaction_records) {
    if (r.frame.size() != s.play_stacks.size()) continue;
    int target_slot = -1;
    for (size_t i = 0; i < r.receiver_hand.size(); ++i) {
      if (r.receiver_hand[i] == r.target_order) target_slot = static_cast<int>(i) + 1;
    }
    if (target_slot < 0) continue;

    struct Held { int order; int slot; Identity id; };
    std::vector<Held> held;
    for (const auto& [o, id] : world.assignment) {
      if (std::find(r.called.begin(), r.called.end(), o) != r.called.end()) continue;
      for (size_t i = 0; i < r.receiver_hand.size(); ++i) {
        if (r.receiver_hand[i] == o) held.push_back({o, static_cast<int>(i) + 1, id});
      }
    }
    if (held.size() < 2) continue;

    const State frame = s.with_stacks(r.frame);
    // 0 = direct, 1 = one away, -1 = neither (and an inverted suit, which the walk
    // skips while anything else is on offer).
    auto kind = [&](Identity id) {
      if (s.variant->suits[id.suit_index].suit_type.inverted) return -1;
      const int away = frame.playable_away(id);
      return (away == 0 || away == 1) ? away : -1;
    };
    std::optional<int> target_kind;
    for (const Held& h : held) {
      if (h.order == r.target_order) target_kind = kind(h.id);
    }
    if (target_kind && *target_kind < 0) return false;
    for (const Held& h : held) {
      if (h.order == r.target_order) continue;
      const int k = kind(h.id);
      if (k == 0 && (h.slot < target_slot || (target_kind && *target_kind == 1))) {
        return false;
      }
      if (k == 1 && target_kind && *target_kind == 1 && h.slot < target_slot) {
        return false;
      }
    }
  }
  return true;
}

namespace {

// THE TEAM LEARNS that `gone` is already on the stacks, and that one of `orders`
// -- hole cards, possibly several -- was the copy that put it there (v16.27.0).
//
// A played card means its whole suit prefix is down, so every view is floored at
// its rank on its suit: the shared view directly, and each pairwise row through
// `with_rows_at_least_common`. Which hole card it was is left to the worlds: the
// joint fact is recorded, so every later enumeration honours it and collapses
// the cards as soon as it fits under the cap -- which a table with seven
// superpositions does not (TODO.md 50), and is why the floor is written here
// rather than waited for.
void team_learns_already_played(Game& game, Identity gone,
                                std::vector<int> orders) {
  if (!orders.empty()) {
    game.hole_requirements.push_back(HoleRequirement{gone, std::move(orders)});
  }
  std::vector<int> floor = game.state.common_play_stacks;
  if (gone.suit_index < 0 || gone.suit_index >= static_cast<int>(floor.size())) return;
  floor[gone.suit_index] = gone.rank;
  game.with_state([&floor](State& st) {
    st = st.with_common_floor(floor).with_rows_at_least_common();
  });
}

}  // namespace

bool collapse_refused_target(Game& game, int giver, Identity gone) {
  if (!game.state.variant->throw_it_in_a_hole) return false;
  std::vector<int> could;
  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    if (!game.meta[o].superposed()) continue;
    if (o >= static_cast<int>(game.state.holders.size())) continue;
    if (game.state.holders[o] != giver) continue;
    if (game.meta[o].superposition.contains(gone)) could.push_back(o);
  }
  if (could.empty()) return false;
  if (could.size() == 1) {
    // Shared: the refusal is a public event, so this is not our own deduction
    // about a partner but the team's about all of them.
    settle(game, could.front(), gone, /*shared=*/true);
    return true;
  }
  // SEVERAL of the giver's hole cards could have been it (v16.27.0). The refusal
  // says one of them was, not which, and settling the first by card order is a
  // guess. Replay 2011854 T27: yagami's o5 `{r1,y1,b1,p1}` (the b1), o18
  // `{r1,g1,p1}` and o24 `{r3,p1,p2}` (the p1) all admitted the refused p1; the
  // guess named o5, and the shared view jumped from 23300 to 34301 with red
  // really on 2.
  team_learns_already_played(game, gone, std::move(could));
  return true;
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
  const auto worlds = open_worlds(game, s.private_base(), me);
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

namespace {

// RULE 8, SHARED (v16.27.0): a partner's STRIKE on a card the whole table can tell
// was already down is common knowledge, and every view learns it.
//
// The strike itself is hidden -- the striker never sees what struck -- so it is
// public only when every seat can reach the same conclusion without that sight:
//
//   * we WATCHED it: the card is X, and X is already down on our own stacks;
//   * the shared view has X's suit at X.rank-1 or above, so to any seat that saw it
//     the card could only have struck as a duplicate (a 1 always qualifies);
//   * the copy that is down is a hole card we can name as X, held by a seat OTHER
//     than the striker -- who therefore watched it go in and knows X is down too.
//
// Replay 2011854: yagami's o24 `{r3,p1,p2}` was the p1; at T28 will-bot69's o29,
// another p1, struck. Every seat knew purple was on 1, but the shared view stayed
// at 0 all game, so yagami's T41 Rank 5 -- will-bot67's b4 paired with will-bot69's
// p2 -- was unreadable to the reacter and will-bot67 discarded.
//
// The striker's own seat learns nothing here: it cannot see X, and would have to
// deduce it from the reading of the call (TODO.md).
void strike_was_a_watched_dupe(Game& game, int striker, int order, Identity id) {
  const State& s = game.state;
  const int me = s.our_player_index;
  if (striker == me) return;
  if (s.common_play_stacks.empty()) return;
  if (s.variant->suits[id.suit_index].suit_type.reversed) return;
  if (!s.is_basic_trash(id)) return;  // not down on our stacks: an ordinary strike
  const int shared = s.common_play_stacks[id.suit_index];
  if (shared >= id.rank) return;      // the team already knows
  if (shared < id.rank - 1) return;   // could have struck as not-yet-playable
  // A card in the hole: drawn, no longer in a hand, and not in the discard pile
  // as X (a strike is filed there, which is why `order` itself is skipped).
  const auto& thrown = s.discard_stacks[id.suit_index][id.rank - 1];
  bool watched = false;
  for (int o = 0; o < static_cast<int>(std::min(s.holders.size(), s.deck.size())); ++o) {
    if (o == order) continue;
    const int holder = s.holder_of(o);
    if (holder < 0 || holder == striker) continue;
    const auto& hand = s.hands[holder];
    if (std::find(hand.begin(), hand.end(), o) != hand.end()) continue;
    if (std::find(thrown.begin(), thrown.end(), o) != thrown.end()) continue;
    auto x = holder == me ? game.me().thoughts[o].id(/*infer=*/true) : s.deck[o].id();
    if (x && *x == id) {
      watched = true;
      break;
    }
  }
  if (!watched) return;
  // Which hole card was the copy is the worlds' business, over the SHARED set.
  std::vector<int> could;
  for (int o = 0; o < static_cast<int>(game.meta.size()); ++o) {
    const IdentitySet& c = game.meta[o].shared_left.non_empty()
                               ? game.meta[o].shared_left
                               : game.meta[o].superposition;
    if (c.contains(id)) could.push_back(o);
  }
  team_learns_already_played(game, id, std::move(could));
}

}  // namespace

void presume_discard_was_played(Game& game, const Action& raw) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return;
  const auto* dc = std::get_if<DiscardAction>(&raw);
  if (!dc) return;
  const int me = s.our_player_index;
  if (dc->player_index_v == me) return;  // our own hole cards are what we cannot see
  const int order = dc->order;
  if (order < 0 || order >= static_cast<int>(game.common.thoughts.size())) return;
  // A STRIKE arrives as a failed discard with its identity withheld; we watched
  // the card in the partner's hand, so we can name it (§1e rule 8, v16.25.0).
  std::optional<Identity> seen;
  if (dc->suit_index != -1 && dc->rank != -1) {
    seen = Identity{dc->suit_index, dc->rank};
  } else if (order < static_cast<int>(s.deck.size())) {
    seen = s.deck[order].id();
  }
  if (!seen) return;
  const Identity id = *seen;
  if (dc->failed) strike_was_a_watched_dupe(game, dc->player_index_v, order, id);
  if (!s.is_playable(id)) return;  // not playable to us: nothing to explain
  const bool struck = dc->failed;

  if (!struck) {
    // The team must have NAMED it: a card merely touched may have been thrown for
    // any reason. The same test as `useful_dc` (decide.cpp), asked before the
    // discard reveals it.
    auto known = game.common.thoughts[order].id(/*infer=*/true, /*symmetric=*/true);
    // ...or named up to the worlds of somebody's hole cards (v16.24.0): a CALLED
    // card whose reading is one identity per world, and the discard shows which.
    // Replay 2011319 T8: our Purple on yagami's o14 reads {p1, p2} -- p2 in the
    // world where our own o5 was the p1 -- and her discard of the p1 at T12 is what
    // says that world is the one we are in.
    const bool named_in_some_world =
        game.meta[order].status == CardStatus::CALLED_TO_PLAY &&
        game.common.thoughts[order].possibilities().contains(id);
    if (!(known && *known == id) && !named_in_some_world) return;
  }
  // A strike needs no naming: the card physically failed to land, and a card that
  // looks playable on our stacks can only fail if it was already played (rule 8).

  const auto worlds = open_worlds(game, s.private_base(), me);
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
  //
  // Replay 2011475 T43 (rule 8): yagami's b3 STRUCK with blue on 2 in will-bot67's
  // belief, so one of will-bot67's own hole cards -- o7 {b3,p3} or o33 {g5,b3} --
  // was the b3. It stayed on blue 2 and at T47 gave a useless rank 2 instead of
  // the Blue for will-bot69's b4. The striker cannot name what struck, so the
  // argument is the watchers' and stays out of the shared view.
  // Which of our hole cards it was need not be decidable card by card -- (g5,b3)
  // and (b3,p3) leave each card both ways -- so the joint fact is kept as well,
  // and every enumeration after this one honours it.
  HoleRequirement req{id, {}};
  for (const auto& [o, unused] : worlds.front().assignment) {
    (void)unused;
    if (game.meta[o].superposition.contains(id)) req.orders.push_back(o);
  }
  if (!req.orders.empty()) game.hole_requirements.push_back(std::move(req));
  prune_to_worlds(game, worlds, surviving, /*shared=*/!struck);
}

bool presume_own_plays_land(Game& game) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return false;
  // Our own hole cards alone: this is the rule read from our own seat, and what a
  // PARTNER can work out about theirs is `advance_rows_from_own_worlds`.
  const auto worlds = open_worlds(game, s.private_base(), s.our_player_index);
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

std::vector<int> floor_over_worlds(const Game& game, const std::vector<int>& base,
                                   const std::vector<int>& holders,
                                   const std::vector<int>& band, bool shared) {
  const State& s = game.state;
  if (base.size() != s.play_stacks.size()) return base;
  const auto worlds =
      open_worlds(game, s.with_stacks(base).with_band(band), holders, 64, -1, shared);
  if (worlds.size() <= 1) return base;
  const std::vector<int> floor = world_floor(s, strike_free(worlds));
  std::vector<int> out = base;
  for (size_t k = 0; k < out.size() && k < floor.size(); ++k) {
    const bool rev = s.variant->suits[k].suit_type.reversed;
    if (rev ? floor[k] < out[k] : floor[k] > out[k]) out[k] = floor[k];
  }
  return out;
}

IdentitySet playable_in_some_own_world(const Game& game) {
  const State& s = game.state;
  const auto worlds = open_worlds(game, s.private_base(), s.our_player_index);
  if (worlds.size() <= 1) return s.playable_set;
  IdentitySet out = s.playable_set;
  for (const OpenWorld* w : strike_free(worlds)) out = out.union_with(w->state.playable_set);
  return out;
}

bool read_stable_over_worlds(const Game& prev, Game& game, const ClueAction& action,
                             const std::vector<int>& view) {
  const State& s = game.state;
  if (!s.variant->throw_it_in_a_hole) return false;
  if (view.size() != s.play_stacks.size()) return false;
  // The worlds of the PAIR's own hole cards, and nobody else's: those are what the
  // giver and the receiver cannot name, so they are where the clue is ambiguous TO
  // THEM. A third seat's hole cards are not -- both of them watched those go in.
  // Our own, when we are that third seat, are our private uncertainty, and
  // widening the shared reading with them would write one seat's doubt into
  // `common` (replay 2011397 T3: will-bot69's {g1,b1} turned yagami's clued b1 into
  // {b1,b2} at will-bot69's seat alone, and rule 1 never fired on it).
  const std::vector<int> holders{action.giver, action.target};
  const State frame = s.with_stacks(view);
  // Worlds are replayed on the frame with its BAND: `view` may already carry the
  // floor these same worlds produced (v16.24.0).
  const auto all = open_worlds(
      game, frame.with_band(s.evidence_known_to_both(action.giver, action.target)),
      holders);
  if (all.size() <= 1) return false;
  std::vector<OpenWorld> worlds;
  for (const OpenWorld* w : strike_free(all)) worlds.push_back(*w);
  if (worlds.size() <= 1) return false;

  bool changed = false;
  for (int o : s.hands[action.target]) {
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    if (o < static_cast<int>(prev.meta.size()) &&
        prev.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      continue;  // a standing call, read by the clue that made it
    }
    const Thought& t = game.common.thoughts[o];
    const IdentitySet read = t.inferred;
    // A PLAY call: what the ladder named is playable in the frame it read -- or,
    // when a seat that could see the card made the call in one world only
    // (`tiiah::interpret_clue`), in that world.
    const bool play_call = read.exists([&](Identity i) {
      if (frame.is_playable(i)) return true;
      for (const OpenWorld& w : worlds) {
        if (w.state.is_playable(i)) return true;
      }
      return false;
    });
    if (!play_call) continue;
    // In each world, the playable card of each suit the ladder named.
    IdentitySet widened = IdentitySet::empty();
    std::vector<std::pair<Identity, std::uint64_t>> support;
    for (std::size_t w = 0; w < worlds.size(); ++w) {
      for (Identity i : t.possible) {
        const bool named_suit = read.filter([&i](Identity r) {
                                      return r.suit_index == i.suit_index;
                                    }).non_empty();
        if (!named_suit || !worlds[w].state.is_playable(i)) continue;
        widened = widened.add(i);
        auto it = std::find_if(support.begin(), support.end(),
                               [i](const auto& p) { return p.first == i; });
        if (it == support.end()) {
          support.emplace_back(i, 1ULL << w);
        } else {
          it->second |= (1ULL << w);
        }
      }
    }
    if (widened.is_empty() || widened == read) continue;
    game.with_thought(o, [&widened](const Thought& th) {
      Thought out = th;
      out.inferred = widened;
      return out;
    });
    record_conditional(game, o, worlds, support);
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
    // Replayed on the row with its BAND (v16.24.0): a floor this very enumeration
    // wrote last time is cards the row counts without naming, and a world card
    // that is one of them is absorbed rather than struck (replay 2011327 T36).
    const State base =
        p < static_cast<int>(s.pairwise_evidence.size())
            ? s.pairwise_view(p).with_band(s.pairwise_evidence[p])
            : s.pairwise_view(p);
    // Read AS A ROW (v16.28.0): the cards we settled privately are not replayed
    // -- the row may or may not count them already, and adding them costs the
    // world cap -- but a card waiting on one of them is not a strike either.
    // Replay 2011887 T18: without that, will-bot69's o9 = g2 struck for want of
    // its privately named o6 (the g1), and the row for will-bot67 claimed red 2.
    const auto worlds =
        open_worlds(game, base, {p, me}, 64, -1, /*shared=*/false, /*row=*/true);
    if (worlds.size() <= 1) continue;
    const auto surviving = strike_free(worlds);

    const std::vector<int> floor = world_floor(s, surviving);
    const std::vector<int> before = s.pairwise_play_stacks[p];
    game.with_state([&](State& st) { st = st.with_pairwise_floor(p, floor); });
    if (game.state.pairwise_play_stacks[p] != before) changed = true;
  }
  return changed;
}

namespace {

// §1e rule 6, asked of the SHARED view (v16.23.0).
//
// A player played a card the whole team could name. If it is the next card on the
// shared stacks, the shared view simply takes it. If it lands ABOVE them, the team
// is short of something -- and it can only be short by cards somebody threw in the
// hole without naming them. Never presume a strike: the worlds of every seat's
// superpositions in which the card could not land are refuted, the survivors'
// common floor is one the team HOLDS, and the card itself goes on top.
//
// Every input is shared -- the play is public, the card's identity was common
// knowledge, the superpositions are built from `common`, and the base is the
// shared view -- so every seat reaches the same answer. Rule 6's other shared form,
// `presume_play_lands`, fires only when a partner's play looks dead to OUR PRIVATE
// stacks, so the seat that could see the missing card never ran it and its shared
// view fell behind everybody else's.
//
// Replay 2011327: will-bot67 knew o17 was the r2 at T11, with red on 0 in the
// shared view. The r1 had to be one of will-bot69's two hole cards, o4 {r1,y1} and
// o18 {r1,r2,y1,y2}; strike-free, they are {r1,y1} either way round, so red and
// yellow are both on 1 and the r2 makes red 2. will-bot69 had reached that long
// before; will-bot67's shared view sat on red 0 for the rest of the game.
void known_play_lands_in_common(Game& game, Identity known, int order) {
  const State& s = game.state;
  if (s.common_play_stacks.empty()) return;
  if (playable_on(s, s.common_play_stacks, known)) {
    game.with_state([known](State& st) { st = st.with_common_play(known); });
    return;
  }
  // Worlds are replayed on the shared view with its BAND (v16.24.0).
  const State base = s.common_evidence.empty()
                         ? s.shared_view()
                         : s.shared_view().with_band(s.common_evidence);
  if (base.is_basic_trash(known)) return;  // behind the shared view: nothing to explain

  std::vector<int> everyone;
  for (int p = 0; p < s.num_players; ++p) everyone.push_back(p);
  const auto worlds = open_worlds(game, base, everyone, 64, order, /*shared=*/true);
  std::vector<const OpenWorld*> surviving;
  for (const OpenWorld* w : strike_free(worlds)) {
    if (w->state.is_playable(known)) surviving.push_back(w);
  }
  if (!surviving.empty()) {
    const std::vector<int> floor = world_floor(s, surviving);
    prune_to_worlds(game, worlds, surviving, /*shared=*/true);
    // A FLOOR: the shared view rises, its evidence stays where it was.
    game.with_state([&floor](State& st) { st = st.with_common_floor(floor); });
  }
  // Whether or not a world names the missing cards, the team watched a card it
  // could name go down, and presumes it landed.
  game.with_state([known](State& st) {
    st = st.with_common_at_least(known).with_rows_at_least_common();
  });
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
  const int player = play->player_index_v;
  const auto known = only_one(candidates);
  const auto seen = game.state.deck[order].id();

  // The ROWS (§1.3, v16.23.0). A row takes a play when both seats of the pair can
  // name it, and it takes it as a floor: the pair watched the card land (never
  // presume a strike), and a stack only goes up, so they know it stands at least
  // that high whatever lower card neither of them can name. Before v16.23.0 a row
  // could only take the NEXT card, and a play that arrived above a card the pair
  // had not yet named was dropped for good.
  //
  // Which card they name is the one they WATCHED. The common reading is only our
  // copy of what the player knew, and a copy read on a stale shared view can be
  // wrong where our eyes are not.
  //
  // Replay 2011327. At T11 will-bot67 played o17 knowing it was the r2, while its
  // row for will-bot69 still had red on 0 -- will-bot69's r1 was in the hole,
  // unnamed -- so the r2 never entered that row and red stuck on 1 for the rest of
  // the game. At T19 will-bot69 played o24, a y2, which will-bot67 had read as
  // {y1} on its stale shared view; the rows took the y1, missed the y2, and the
  // y3 at T25 could not land either. At T38 will-bot67 gave a reactive against a
  // row with yellow on 1 that yagami read with yellow on 3, and she struck.
  //
  // Only a play our own belief says LANDED -- `presume_play_lands` has already
  // asked every world of ours -- so a strike is never booked into anybody's view.
  const bool ours = player == game.state.our_player_index;
  const std::optional<Identity> named = seen ? seen : known;
  if (named && game.state.is_playable(*named)) {
    std::vector<int> knowers;
    for (int p = 0; p < game.state.num_players; ++p) {
      if (p == game.state.our_player_index) continue;
      // The player's own row takes it only if the player could name it: they
      // cannot see the card they threw. Our own card we name only through
      // `known`, which is what every seat watching it reads too.
      if (p == player && !known) continue;
      if (ours && !known) continue;
      knowers.push_back(p);
    }
    game.with_state([&](State& st) { st = st.with_pairwise_at_least(*named, knowers); });
  }

  if (known) {
    game.with_meta(order, [k = *known](ConvData& m) {
      m.named_in_hole = IdentitySet::single(k);
    });
    // The player knew what they were playing, so every seat can follow it and
    // the shared view takes it (§1e rule 6's shared-view form, v16.23.0, when it
    // lands above that view). Our own believed view is advanced by the engine,
    // which is handed the same identity.
    known_play_lands_in_common(game, *known, order);
    return;
  }
  if (candidates.is_empty()) return;
  // The player could not name it, so it is a superposition -- for them, and for
  // the shared view. The watchers' rows took it above.
  const int turn = game.state.turn_count;
  game.with_meta(order, [candidates, turn](ConvData& m) {
    m.superposition = candidates;
    m.hole_turn = turn;
  });
}

namespace {

// §1e, the targeting rules as evidence (v16.24.0), SHARED. Every seat watched the
// reactions, every seat holds the same superpositions, and the frame each record
// keeps is the shared view -- so every seat refutes the same worlds. Among what
// survives, "never presume a strike" is the convention (rule 6), asked of the
// shared view over every seat's hole cards jointly.
//
// Only when the targeting rules refuted something: that is the evidence this rule
// adds. Without it the same strike-free question on the shared view would be a
// new shared form of rule 6 in its own right, and the shared view can lag cards a
// seat settled privately -- a world could look struck there that is not.
// The shared view is the MINIMUM across every seat's worlds, as every other view
// is (§1e, v16.24.0): what all seats know of the stacks is the height every world
// of everybody's hole cards reaches. Every input is shared, so every seat writes
// the same floor; replayed with its band, so a floor written last time is absorbed
// rather than read as a strike.
bool advance_common_from_worlds(Game& game) {
  const State& s = game.state;
  if (s.common_play_stacks.empty()) return false;
  std::vector<int> everyone;
  for (int p = 0; p < s.num_players; ++p) everyone.push_back(p);
  const State base = s.common_evidence.empty()
                         ? s.shared_view()
                         : s.shared_view().with_band(s.common_evidence);
  const auto worlds = open_worlds(game, base, everyone, 64, -1, /*shared=*/true);
  if (worlds.size() <= 1) return false;
  const auto surviving = strike_free(worlds);
  const std::vector<int> floor = world_floor(s, surviving);
  const std::vector<int> before = s.common_play_stacks;
  game.with_state([&floor](State& st) {
    st = st.with_common_floor(floor).with_rows_at_least_common();
  });
  // ...and what every surviving world agrees a card WAS, the team knows it was
  // (v16.25.0): never presume a strike, asked of the shared view over every
  // seat's hole cards. The worlds are the SHARED set -- cards a seat settled
  // privately stay in them -- so no world strikes for want of a card only one
  // seat can name. Replay 2011475 T25: after will-bot67's g2 (o23 `{g2,b1}`),
  // (o4, o23) is (g1, g2) or (g1, b1); yagami's o4 was the g1, and not before.
  const bool pruned = prune_to_worlds(game, worlds, surviving, /*shared=*/true);
  return pruned || game.state.common_play_stacks != before;
}

bool prune_infeasible_worlds(Game& game) {
  const State& s = game.state;
  if (game.reaction_records.empty()) return false;
  std::vector<int> everyone;
  for (int p = 0; p < s.num_players; ++p) everyone.push_back(p);
  // Worlds are replayed on the shared view with its BAND (v16.24.0).
  const State base = s.common_evidence.empty()
                         ? s.shared_view()
                         : s.shared_view().with_band(s.common_evidence);
  const auto raw = enumerate_worlds(game, base, everyone, 64, -1, /*shared=*/true);
  if (raw.size() <= 1) return false;
  std::vector<OpenWorld> feasible;
  for (const OpenWorld& w : raw) {
    if (world_feasible(game, w)) feasible.push_back(w);
  }
  if (feasible.empty() || feasible.size() == raw.size()) return false;
  std::vector<const OpenWorld*> surviving;
  for (const OpenWorld* w : strike_free(feasible)) {
    // Point back into `raw`, which is what `prune_to_worlds` narrows against.
    for (const OpenWorld& r : raw) {
      if (r.assignment == w->assignment) surviving.push_back(&r);
    }
  }
  return prune_to_worlds(game, raw, surviving, /*shared=*/true);
}

}  // namespace

void collapse_superpositions(Game& game, const Game& prev, const Action& action) {
  if (!game.state.variant->throw_it_in_a_hole) return;
  if (game.meta.empty()) return;

  const Evidence ev = evidence_from(prev, game, action);
  bool changed = false;

  // The targeting rules first: a world no reaction could have come from is not
  // one any rule below should be weighing.
  if (prune_infeasible_worlds(game)) changed = true;

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
  if (advance_common_from_worlds(game)) changed = true;
  if (advance_rows_from_own_worlds(game)) changed = true;

  // A stack that moved changes what every hand could be holding.
  if (changed) game.elim();
}

}  // namespace hanabi::tiiah
