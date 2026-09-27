// Port of python-bot/src/hanabi_bot/basics/state.py.
// Original Scala: scala-bot/src/scala_bot/basics/State.scala.
//
// State is the public game-state shared by every observer. Value type;
// `with_*` methods return a new State (copy-on-write). For the endgame
// solver hot path, State will be mutated in place via simulate_action_inplace
// (Phase 5) — that's a deliberate departure from the immutable shape.
#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/variant.h"

namespace hanabi {

struct Thought;

// Hand size by num_players (index = num_players). Indices 0,1 unused.
// Matches scala-bot/.../Game.scala line 14.
inline constexpr std::array<int, 7> kHandSize = {0, 0, 5, 5, 4, 4, 3};

struct State {
  // Pointed-to variant lives in the load_variants() cache; State doesn't own it.
  const Variant* variant = nullptr;
  TableOptions options;

  int num_players = 0;
  std::vector<std::string> names;
  int our_player_index = 0;

  int cards_left = 0;
  int cards_total = 0;

  // What WE believe has been played. In every ordinary variant that is simply
  // what has been played; under Throw It in a Hole it is a belief, because our
  // own plays go into the hole unseen (tiiah/CONVENTION.md 1.1).
  std::vector<int> play_stacks;
  // THROW IT IN A HOLE: the stacks as everyone-knows-everyone-knows them.
  //
  // A play advances these only when its identity was COMMON knowledge at the
  // time — i.e. the player themselves knew what they were playing — or when a
  // superposition later collapses on evidence every seat shares. It therefore
  // lags `play_stacks`, which also advances on the partners' plays that we
  // watched and they did not.
  //
  // It exists because a clue has to mean one thing: any rule that reads the
  // stacks while INTERPRETING must read a view every seat computes identically,
  // or two seats read the same clue two ways. Empty outside TIIAH, where the
  // `shared_*` accessors below fall through to the ordinary ones.
  std::vector<int> common_play_stacks;
  // THROW IT IN A HOLE: row `p` is the stacks WE KNOW SEAT p KNOWS — the plays
  // common to us and them, the "pairwise view" (tiiah/CONVENTION.md 1.3).
  //
  // A hidden play is known to every seat EXCEPT the one who made it, so a play
  // by X counts for the pair (us, p) when X knew it themselves (then everyone
  // does, and `common_play_stacks` has it too) or when X is neither of us. Our
  // own hidden plays are absent by construction: seat p watched them, but we
  // cannot name them, so we cannot put them in. Each row therefore sits between
  // `common_play_stacks` and what seat p actually believes.
  //
  // This is the view a clue is read against: `common_play_stacks` is what
  // EVERYONE knows, which one seat's ignorance holds back for the whole team,
  // while a clue only has to mean one thing to the two seats it is between.
  // Empty outside TIIAH. Row `our_player_index` is unused.
  std::vector<std::vector<int>> pairwise_play_stacks;
  // The same rows WITHOUT the world floors (`tiiah::advance_rows_from_own_worlds`):
  // only the plays the pair can name. What hole-card worlds are replayed on --
  // replaying a world on a row its own floor already lifted strikes the world that
  // produced the floor (v16.24.0, replay 2011327 T36: `{b3,b4}` on a row already
  // at blue 3 left only the b4 world, and the row went to blue 4).
  std::vector<std::vector<int>> pairwise_evidence;
  // `common_play_stacks` without its world floors, for the same reason.
  std::vector<int> common_evidence;
  // ...and `play_stacks` -- our own belief -- without the floor
  // `tiiah::presume_own_plays_land` raises it to. Empty outside TIIAH.
  std::vector<int> play_evidence;
  // Our belief carrying its band: the base our own hole-card worlds replay on.
  State private_base() const;
  // TRANSIENT, for world replay only (`tiiah::open_worlds`): the evidence heights
  // under `play_stacks` when this state is a view with a band of cards known to be
  // down but not tied to a card. Empty everywhere else.
  std::vector<int> band_floor;
  // discard_stacks[suit][rank-1] = orders of cards discarded for that identity,
  // newest first (matching Scala's `order +: list` cons).
  std::vector<std::array<std::vector<int>, 5>> discard_stacks;
  std::vector<int> max_ranks;
  // base_count[ord] = number of physical copies known unavailable (discarded,
  // misplayed, played) for that identity.
  std::vector<int> base_count;

  IdentitySet all_ids;
  IdentitySet playable_set;
  IdentitySet critical_set;
  IdentitySet trash_set;
  // card_count[ord] = total copies in the deck.
  std::vector<int> card_count;

  std::vector<std::vector<int>> hands;   // hands[player] = orders
  std::vector<Card> deck;
  std::vector<int> holders;              // holders[order] = player_index that drew it

  int turn_count = 0;
  int clue_tokens = 8;
  bool half_clue_token = false;
  // The kind of the most recent clue given by ANY player, or nullopt before the
  // first one. Only Alternating Clues reads it, where the server rejects a clue
  // of the same kind as the one before -- see `all_valid_clues`.
  //
  // Set in `Game::on_clue`, which every real and simulated clue passes through,
  // so a hypo and a snapshot replay both rebuild it without being serialised.
  // A play or a discard in between does NOT reset it: the rule is about
  // consecutive CLUES, not consecutive turns.
  std::optional<ClueKind> last_clue_kind;
  int strikes = 0;
  std::optional<int> endgame_turns;
  int next_card_order = 0;

  std::vector<std::vector<Action>> action_list;
  int current_player_index = 0;

  // --- Factory ---
  static State create(std::vector<std::string> names, int our_player_index,
                       const Variant& variant, TableOptions options);

  // --- Mutators (return new State) ---
  State with_discard(Identity id, int order) const;
  State with_play(Identity id) const;
  // Advance the shared view alone (Throw It in a Hole). See state.cpp.
  State with_common_play(Identity id) const;
  // Advance the pairwise rows for every seat in `knowers` (Throw It in a Hole).
  // The caller decides who learned it; see `tiiah::note_hidden_action`.
  State with_pairwise_play(Identity id, const std::vector<int>& knowers) const;
  // RAISE-ONLY forms of the two above (Throw It in a Hole, v16.23.0): the view
  // goes to AT LEAST `id`, and a view already at or past it is left alone. A
  // play both seats of a pair can name puts the stack at least that high for
  // them, whatever lower card neither of them can name (tiiah §1.3).
  State with_common_at_least(Identity id) const;
  State with_pairwise_at_least(Identity id, const std::vector<int>& knowers) const;
  // Every row raised to at least the shared view, suit by suit: what all seats
  // know, every pair knows.
  State with_rows_at_least_common() const;
  // Raise the shared view to a world FLOOR, leaving `common_evidence` where it is.
  State with_common_floor(const std::vector<int>& floor) const;
  // Raise row `p` to a world FLOOR, leaving `pairwise_evidence` where it is.
  State with_pairwise_floor(int p, const std::vector<int>& floor) const;
  // `stacks_known_to_both` on the evidence rows.
  std::vector<int> evidence_known_to_both(int a, int b) const;
  // This state carrying `band_floor = lo`, for replaying hole-card worlds on a view.
  State with_band(const std::vector<int>& lo) const {
    State out = *this;
    out.band_floor = lo;
    return out;
  }
  State try_play(Identity id) const;
  State regain_clue() const;
  // This state with the SHARED stacks in place of our own belief: what every
  // seat can compute alike, which is what a rule deciding what a clue MEANS has
  // to run on (Throw It in a Hole, CONVENTION.md §1.3 and §1e). A superposed
  // play never advanced `common_play_stacks`, so this is also "assuming none of
  // the superposed cards were played" — the two are the same state.
  //
  // `playable_set` and `trash_set` are rebuilt from the swapped stacks; the
  // discard accounting (`base_count`, `discard_stacks`) is the same in both
  // views and is left alone. Returns `*this` outside TIIAH, where the shared
  // vector is empty, so no other variant pays for it.
  State shared_view() const;
  // This state with an arbitrary stack vector in place of our belief, with
  // `playable_set` / `trash_set` rebuilt to match and the discard accounting
  // left alone. `shared_view` and `pairwise_view` are both this with a
  // particular vector; a mismatched length returns `*this`.
  State with_stacks(const std::vector<int>& stacks) const;
  // This state as we know seat `other` sees it — the pairwise view
  // (`pairwise_play_stacks`, tiiah/CONVENTION.md §1.3). What a clue between us
  // and them is read against. Returns `*this` outside TIIAH, and for our own
  // seat, where our belief IS what we know we know.
  State pairwise_view(int other) const;
  // The stacks we know seats `a` and `b` BOTH hold — the view a clue between
  // them is read against (tiiah/CONVENTION.md §1.3). When we are one of the two
  // this is the other's pairwise row, the relation being symmetric; when we are
  // neither we cannot compute it (the plays missing from the shared view are our
  // own, and we cannot name them) and the shared view is returned as the floor.
  // Outside TIIAH it is simply our belief, which every seat shares.
  std::vector<int> stacks_known_to_both(int a, int b) const;

  // --- Pure helpers ---
  bool ended() const;
  int score() const;
  int max_score() const;
  int rem_score() const { return max_score() - score(); }
  int pace() const { return score() + cards_left + num_players - max_score(); }

  // The same two questions asked of the SHARED view (`common_play_stacks`), for
  // the rules that decide what a clue MEANS. Identical to the pair above in
  // every variant but Throw It in a Hole, so a call site can simply prefer them
  // without a variant test.
  int shared_score() const;
  int shared_pace() const {
    return shared_score() + cards_left + num_players - max_score();
  }

  int last_player_index(int player_index) const {
    return (player_index + num_players - 1) % num_players;
  }
  int next_player_index(int player_index) const {
    return (player_index + 1) % num_players;
  }

  // Reversed-direction semantics: a reversed suit's stack starts at 6
  // (sentinel meaning "nothing played, next playable = 5") and decreases
  // as cards play 5 → 4 → 3 → 2 → 1. `max_ranks[suit]` for reversed
  // stores the *lowest* still-achievable rank (rises as low-rank
  // criticals are discarded).
  bool is_basic_trash(Identity id) const {
    const auto& st = variant->suits[id.suit_index].suit_type;
    if (st.reversed) {
      return id.rank >= play_stacks[id.suit_index] || id.rank < max_ranks[id.suit_index];
    }
    return id.rank <= play_stacks[id.suit_index] || id.rank > max_ranks[id.suit_index];
  }
  bool is_useful(Identity id) const {
    const auto& st = variant->suits[id.suit_index].suit_type;
    if (st.reversed) {
      return id.rank < play_stacks[id.suit_index] && id.rank >= max_ranks[id.suit_index];
    }
    return id.rank > play_stacks[id.suit_index] && id.rank <= max_ranks[id.suit_index];
  }
  int playable_away(Identity id) const {
    const auto& st = variant->suits[id.suit_index].suit_type;
    if (st.reversed) {
      return (play_stacks[id.suit_index] - 1) - id.rank;
    }
    return id.rank - (play_stacks[id.suit_index] + 1);
  }
  bool is_playable(Identity id) const { return playable_away(id) == 0; }
  bool is_critical(Identity id) const;

  // Per-suit "cards played so far" — for both normal (stack 0 → 5
  // counts directly) and reversed (stack 6 → 1, count = 6 − stack).
  int played_count(int suit_index) const {
    const auto& st = variant->suits[suit_index].suit_type;
    if (st.reversed) return 6 - play_stacks[suit_index];
    return play_stacks[suit_index];
  }
  // Per-suit "max cards reachable" given current discards.
  int max_played(int suit_index) const {
    const auto& st = variant->suits[suit_index].suit_type;
    if (st.reversed) return 6 - max_ranks[suit_index];
    return max_ranks[suit_index];
  }

  const std::vector<int>& our_hand() const { return hands[our_player_index]; }
  bool can_clue() const { return clue_tokens > 0; }
  int holder_of(int order) const;
  bool in_starting_hand(int order) const {
    return order < num_players * kHandSize[num_players];
  }
  int multiplicity(IdentitySet ids) const;

  bool has_consistent_infs(const Thought& thought) const;

  std::vector<int> clue_touched(const std::vector<int>& orders,
                                 ClueKind kind, int value) const;
  std::vector<Clue> all_colour_clues(int target) const;
  std::vector<Clue> all_valid_clues(int target) const;

  // Substring match on any suit name. The Python version uses re.Pattern;
  // for now we expose simple-substring search (sufficient for current call
  // sites in basics/conventions; revisit if a real regex is required).
  bool includes_variant(std::string_view needle) const;

  Identity expand_short(std::string_view short_) const;
  std::string log_id(std::optional<Identity> id) const;
  std::string log_id_by_order(int order) const;
};

}  // namespace hanabi
