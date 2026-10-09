// Throw It in a Hole — superposition (CONVENTION.md §1e).
//
// A card that goes into the hole takes its identity with it, the player who
// played it included. What that player keeps instead is a SUPERPOSITION: the
// set of identities the card could have been. Everyone tracks everyone's, from
// `common`, so the seats agree about what each of them knows.
//
// These entry points are keyed on `Variant::throw_it_in_a_hole` rather than on
// `Game::convention`, because a TIIAH table with 4+ seats is one the convention
// refuses to ACT on while still tracking, and it
// needs the same bookkeeping.
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"

namespace hanabi {
class Game;
}

namespace hanabi::tiiah {

// One of the worlds a seat's outstanding hole plays leave open: an assignment
// of one identity to each of them, and the stacks that assignment produces.
//
// `struck` marks a world in which one of those plays did NOT land. It is still a
// world — a strike is possible — but §1e rule 6 refutes it whenever a strike-free
// world is available, so the callers that apply that rule need to be told which
// is which.
struct OpenWorld {
  std::vector<std::pair<int, Identity>> assignment;
  State state;
  bool struck = false;
  // Per suit, the ranks of the base's BAND (`State::band_floor`) this world's
  // cards have already been absorbed as -- each can be one card only.
  std::vector<unsigned> absorbed;
};

// The worlds the cards `holders` still have in the hole leave open, starting
// from `base` and applying each assignment in PLAY ORDER so a chain lands
// (CONVENTION.md §1e).
//
// Exactly one world — `base` with an empty assignment — when they have nothing in
// the hole, which keeps every reading that calls this unchanged until somebody
// plays a card they cannot name. Also one when the product would exceed `cap`: a
// partial enumeration would read as a conditional set that is missing worlds,
// which is worse than reading it unconditionally.
//
// More than one holder is for the questions asked ABOUT a seat rather than from
// it: what seat `p` can work out rests on `p`'s own hole cards AND on ours, since
// `p` watched ours leave our hand and we cannot name them (§1.3).
//
// `except_order` leaves one card out of the enumeration — for a reader asking what
// THAT card was, whose frame is the other hole cards and not itself. Needed only
// after the fact: at clue time the card in question is still in a hand, so it is
// not in the map to begin with.
//
// `shared` asks for the worlds of the SHARED view (v16.25.0): cards we settled
// privately stay in them with the set every other seat still allows
// (`ConvData::shared_left`), since no other seat followed us.
//
// `row` asks for them as a PAIRWISE ROW reads them (v18.1.0), with `shared`: a
// card we settled privately is left out once `base` already counts the identity we
// settled it as (`ConvData::private_named`), because the row has then counted
// that card, and replaying it would strike it as a duplicate of itself.
std::vector<OpenWorld> open_worlds(const Game& game, const State& base,
                                   const std::vector<int>& holders, int cap = 64,
                                   int except_order = -1, bool shared = false,
                                   bool row = false);
std::vector<OpenWorld> open_worlds(const Game& game, const State& base,
                                   int holder, int cap = 64,
                                   int except_order = -1, bool shared = false);

// §1e rule 6, asked of ONE seat's own plays: the worlds in which none of them
// struck, or all of them when every world has a strike in it — then the strike
// is real and there is nothing to refute.
std::vector<const OpenWorld*> strike_free(const std::vector<OpenWorld>& worlds);

// Narrow every hole card to what `surviving` (a subset of `worlds`) still
// allows, settling the ones left with a single identity -- which moves the shared
// view and the rows when `shared` (§1e). The second half of rule 6, for a caller
// outside this file: the reactive world fallback (CONVENTION.md §1d, v19.3.0).
bool collapse_to_worlds(Game& game, const std::vector<OpenWorld>& worlds,
                        const std::vector<const OpenWorld*>& surviving, bool shared);

// THE ACTIONABLE SUPERPOSITION COLLAPSE RULE (ASCR; CONVENTION.md §1e, v20.6.0,
// the user's ruling). A seat that seems to be called to play a card it cannot play
// first asks whether some world of the hole cards makes it playable. `tiers` are the
// card's candidate identities in priority order (a receiver's bucket + finesse suits
// before the rest); the first tier with an identity that `works` in some world is
// the reading, and `kept` the worlds in which any of its identities works. With
// `require_evidence`, a reading one of whose identities works in EVERY world is not
// one: it does not rest on the hole (v20.8.0; a reading whose identities each need
// some worlds is kept, even when together they cover them all). The caller then
// collapses with `collapse_to_worlds(..., kept, shared)`.
// Every site that reads a play call in the worlds before writing it off shares this.
struct AscrReading {
  IdentitySet reading = IdentitySet::empty();
  std::vector<const OpenWorld*> kept;
  int tier = -1;
};
std::optional<AscrReading> ascr_find(
    const std::vector<const OpenWorld*>& worlds, const std::vector<IdentitySet>& tiers,
    const std::function<bool(const OpenWorld&, Identity)>& works, bool require_evidence);

// Keep the conditional half of a reading, so a later fact can withdraw it: each
// candidate of `support` carries a bitmask over `worlds`, and one every world
// agrees on is unconditional and left out (`ConvData::ConditionalReading`).
void record_conditional(Game& game, int order, const std::vector<OpenWorld>& worlds,
                        const std::vector<std::pair<Identity, std::uint64_t>>& support);

// A world is FEASIBLE when every reactive play clue its cards were in the receiver's
// hand for could have produced the reaction we watched (§1e, v16.24.0). The target
// walk takes a direct playable before a finesse, and each leftmost first; a world
// in which one of our superposed cards would have out-ranked the card the reacter
// actually called cannot be the world we are in. `open_worlds` drops such worlds
// (all of them kept when none survives).
//
// With `named_cards`, the receiver's cards the team has NAMED in the hole count too
// (v20.21.0), as a direct playable refuting a world whose target is a finesse. Only
// `prune_infeasible_worlds` asks for it -- it settles between actions, after every
// reading is complete. `open_worlds` runs while a clue is still being read, when a
// call's single-world reading can look named (replay 2011397 T10).
bool world_feasible(const Game& game, const OpenWorld& world, bool named_cards = false);

// §1e, the STABLE half of "what a card means depends on what you threw away"
// (v16.24.0). The stable ladder read the clue on one frame, `view`; a card it has
// just called to play is re-read in every strike-free world of the pair's hole
// cards, and the reading becomes the union -- the next card of the same suit in
// each world -- with the conditional half recorded. One world: nothing changes.
// Returns whether a reading widened.
bool read_stable_over_worlds(const Game& prev, Game& game, const ClueAction& action,
                             const std::vector<int>& view);

// `base` raised, suit by suit, to the height every strike-free world of `holders`'
// hole cards reaches -- the MINIMUM across those worlds (§1e, v16.24.0). What a
// frame is: the stacks a seat can count on whichever world it lives in.
// `except_order` leaves one hole card out, as for `open_worlds`.
std::vector<int> floor_over_worlds(const Game& game, const std::vector<int>& base,
                                   const std::vector<int>& holders,
                                   const std::vector<int>& band = {},
                                   bool shared = false, int except_order = -1);

// The identities playable in SOME strike-free world of our own hole cards, on our
// belief -- what a call on our own card may be when we cannot name what we threw.
IdentitySet playable_in_some_own_world(const Game& game);

// Does `reading` hold a playable identity in EVERY strike-free world of our own hole
// cards (v20.9.0, the user's ruling)? Then a call on one of our cards with that
// reading is a known play, though no single identity of it plays on our belief:
// whichever world we are in, the card plays (replay 2018766 T23: `{r2,y2}` over a
// hole card that was the r1 or the y1). False with fewer than two worlds.
bool plays_in_every_own_world(const Game& game, const IdentitySet& reading);

// Does `reading` play on the SHARED view, with our own stacks behind it only for
// want of our own unnamed hole cards (v20.11.0, the user's ruling)? Every identity
// must be playable on the shared view and not trash on ours, and every card between
// our stack and it must be a candidate of one of our own hole cards: the shared view
// reads that card as down, and we cannot name it. Replay 2018874 T47: `{b5}` with
// blue on 4 shared and on 3 for us, our hole card o29 `{g4,b4}`. When our view is
// behind for any other reason, it knows better than the shared one.
bool plays_on_shared_view_past_own_hole(const Game& game, const IdentitySet& reading);

// The other half of a conditional reading: an antecedent has narrowed to
// `still`, so every world it contradicts is gone, and so is every candidate that
// had no other world left to stand in (`ConvData::ConditionalReading`).
//
// Called whenever a superposition narrows or settles — from
// `collapse_superpositions` and from its `settle`, which has to do it before it
// clears the set. Returns whether anything moved.
bool refute_worlds(Game& game, int antecedent, const IdentitySet& still);

// §1e rule 5, the REFUSAL (v16.14.0): `gone` is already played, and `giver`
// named it as a play — so their stacks are short by exactly that card, and the
// only plays missing from a seat's own stacks are the ones it threw in the hole.
// Settle the superposition that admits it.
//
// Shared, like rules 1 and 2: every seat watched the refusal and can draw the
// same conclusion, so the shared stacks move with it. Returns whether it settled
// anything.
bool collapse_refused_target(Game& game, int giver, Identity gone);

// The team learns, from a public event, that `gone` is already played (v20.12.0):
// a reacter threw his fixed reaction card, a dead `gone`. If it is not already on
// the shared view, one of the hole cards whose shared set admits it was the copy:
// settle the one, or record the joint fact for the worlds. Returns whether any
// hole card could have been it.
bool team_learns_a_hole_card_was(Game& game, Identity gone);

// §1e rule 6: never presume a partner's play STRUCK.
//
// Our own stacks can be short by exactly what we threw in the hole — a card we
// cannot name is the only thing that makes them short, since every partner's play
// is one we watched. So a partner's play that looks dead to us may be landing on
// a card we played without knowing it.
//
// Asked of the worlds our own hole cards leave open (`open_worlds`): if ANY of
// them lets the play land, every world in which it strikes is refuted. Our
// superpositions are narrowed to what survives and settled when that leaves one
// identity, which advances our believed stacks — so the play then lands.
//
// SHARED as of v16.21.0, so `common_play_stacks` and every pairwise row move with our
// belief. Every seat watched our card go into the hole, every seat holds its candidate
// set (built from `common`), the seat that made the play knows its own reading of it,
// and never-presuming-a-strike is the convention — so the conclusion is reachable from
// every chair. `presume_own_plays_land` below is the form that stays PRIVATE, because
// it judges our own plays against our own belief.
//
// Called from `Game::handle_action` with the RAW action and BEFORE
// `resolve_hidden_action`, whose play-vs-misplay test is what reads the result.
// A no-op outside TIIAH, for our own play (we cannot see it), and when no world
// rescues the card — a partner really can misplay, and then the strike stands.
void presume_play_lands(Game& game, const Action& raw);

// §1e rule 7: a known playable that a partner DISCARDS was already played (v16.22.0).
//
// A partner sees every card in the hole but their own. If they throw a card the
// team had named, and our stacks say it is playable, then our stacks are short —
// and they can only be short by what WE threw in the hole. So every world in which
// the card is still needed is refuted, exactly as rule 6 refutes the worlds in
// which a partner's play strikes. There is no gentleman's discard in TIIAH; this is
// what such a discard means instead. SHARED, for rule 6's reasons: the discard is
// public, the card's identity was common knowledge, the candidate sets come from
// `common`.
//
// Called from `Game::handle_action` with the RAW action, before the dispatch, so
// `interpret_discard` reads the advanced stacks. A no-op outside TIIAH, for our own
// discard, for a card the team could not name, and when no world has the card
// already played — then the partner simply threw a useful card.
void presume_discard_was_played(Game& game, const Action& raw);

// §1e rule 6 again, turned on OUR OWN plays (v16.18.0).
//
// A card we threw in the hole struck or landed and we were not told which. The
// rule is the same one rule 6 applies to a partner's play, so it has to be the
// same answer: presume it landed. Every world in which one of our own hole cards
// failed is refuted, as long as some world has none failing, and a set narrowed
// to one identity settles — which advances our believed stacks.
//
// Called from `collapse_superpositions` rather than from an action hook, because
// it is not about the action: it is about what our own outstanding plays must have
// been, and any evidence that narrows one of them can make it answerable.
bool presume_own_plays_land(Game& game);

// §1.3: a pairwise row takes what holds in EVERY world that survives for the seat
// behind it (v16.18.0).
//
// A row excludes that seat's own hidden plays, because the seat cannot name them.
// But it can still REASON about them: across the worlds they leave open, the ones
// where a play struck are refuted by rule 6, and whatever height is reached in all
// the survivors is one the seat holds. Replay 2010329: yagami's two hole cards are
// each `{g1,b1}`, so `(g1,g1)` and `(b1,b1)` both strike and green and blue are on
// 1 in every survivor.
//
// Enumerated over that seat's hole cards AND ours together, since the seat watched
// ours (see `open_worlds`). Rows only ever advance. Returns whether one moved.
bool advance_rows_from_own_worlds(Game& game);

// CORE RULE 2 AT OUR OWN SEAT (v18.9.0): what we read our own CALLED card as when
// the team's reading of it is one our own eyes rule out -- every copy of it is in
// view. The bucket and finesse reading being empty for us, the card is any
// playable: the identities it could still be that are playable on our stacks,
// less the identity of the receiver's target when we are the reacter (the pair
// would both play it). `possible` when that leaves nothing, or when the card is
// not our own called card, or outside the variant. `Game::elim` asks it where our
// view of a card would otherwise fall back to everything. Human diagnostic
// 2013726 T27 (human_vs_bot_diagnostics/2013726.md): black's reaction card
// read `{n3}` by the bucket, black sees both n3s, and so knows it holds the r4.
IdentitySet own_called_fallback(const Game& game, int order, const IdentitySet& possible);

// Narrow one superposition to `allowed`, and settle it when that leaves a single
// identity (v16.19.0).
//
// SHARED, like rules 1 and 2: the caller's evidence is the convention's own reading
// of a public action, which every seat computes alike, so the shared stacks move with
// it. A set narrowed to nothing is left alone — a reading is a reading, and one
// refuted down to nothing is evidence that something else is wrong.
//
// `tiiah::narrow_reacter_play` is the caller: the receiver of a reactive works out at
// reaction time what the REACTER's blind play must have been, which at that seat is
// the only chance to write it down at all (§1d).
bool narrow_superposition(Game& game, int order, const IdentitySet& allowed);

// The PRIVATE twin (v17.1.0): narrow one of OUR superpositions on evidence only we
// hold, leaving the shared set behind (`ConvData::shared_left`), and settle it
// privately when one identity is left. Used when a clue between two other seats
// can only be a call in some worlds of our own hole cards (§1e rule 4's shape).
bool narrow_own_privately(Game& game, int order, const IdentitySet& allowed);

// §1e rule 3 for a PAIR (v16.18.0). Every copy of `id` is accounted for in the
// discard pile or in a hand belonging to NEITHER us nor `p` — so `p` accounts for
// them exactly as we do, and each of us can see that the other can.
//
// Strictly stronger than the private form `collapse_superpositions` applies to our
// own belief: a copy in either of our own hands is one the other cannot see, so it
// does not count for the pair. That ordering is what keeps a row from ever holding
// something our own belief does not.
bool all_copies_visible_to_pair(const Game& game, int order, Identity id, int p);

// Called from `Game::handle_action` with the RAW wire action, before
// `resolve_hidden_action` has filled in what we could see. A play that reached
// the hole without a common-knowledge identity stamps
// `ConvData::superposition`; one WITH such an identity advances the shared
// stacks instead, since every seat can follow it.
//
// No-op outside TIIAH, and for every action that is not a hidden play.
void note_hidden_action(Game& game, const Action& raw);

// A stable call on another seat that overlaps the CALLED card just played is
// REBASED onto it (CONVENTION.md §1c, v19.2.0): `{p1}` on Cathy, given while
// Bob's own `{p1}` call stood, becomes `{p2}` once Bob plays his. Called from
// `Game::handle_action` before the play is dispatched, while `common` still
// holds the played card's reading and before the call invariants could judge
// the overlapping call dead. No-op outside TIIAH and for a non-play.
void rebase_calls_on_a_played_call(Game& game, const Action& raw);

// Called after an action has been interpreted. Applies §1e's collapsing rules
// to every outstanding superposition:
//
//   SHARED, so they also move the shared stacks --
//     1. another player played a card of that identity;
//     2. a clue called a playable card of that identity to play.
//   Both say the identity was still needed, so the superposed card was not it.
//
//   PRIVATE, so it moves our believed stacks alone --
//     3. every copy is accounted for between the discard pile and the hands we
//        can see.
//
// A set that reaches one identity leaves the map, and the stack it belongs to
// advances: our own believed one whenever the card was ours, and the shared one
// only when the collapse was shared.
//
// `prev` is the pre-action game, which is what makes "this clue newly called
// that card to play" answerable.
void collapse_superpositions(Game& game, const Game& prev, const Action& action);

}  // namespace hanabi::tiiah
