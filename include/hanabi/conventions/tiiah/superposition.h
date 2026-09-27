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
std::vector<OpenWorld> open_worlds(const Game& game, const State& base,
                                   const std::vector<int>& holders, int cap = 64,
                                   int except_order = -1);
std::vector<OpenWorld> open_worlds(const Game& game, const State& base,
                                   int holder, int cap = 64,
                                   int except_order = -1);

// §1e rule 6, asked of ONE seat's own plays: the worlds in which none of them
// struck, or all of them when every world has a strike in it — then the strike
// is real and there is nothing to refute.
std::vector<const OpenWorld*> strike_free(const std::vector<OpenWorld>& worlds);

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
