cmake_minimum_required(VERSION 3.20)
project(hanabi_bot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

include(cmake/warnings.cmake)
include(cmake/deps.cmake)

# --- Core library: basics, conventions, endgame -------------------------
find_package(OpenSSL REQUIRED)
# Boost 1.90+ provides asio/beast as header-only; use the headers target.
find_package(Boost CONFIG REQUIRED)

add_library(hanabi_core STATIC
  src/basics/action.cpp
  src/basics/card.cpp
  src/basics/convention.cpp
  src/basics/clue_result.cpp
  src/basics/connection.cpp
  src/basics/decide.cpp
  src/basics/eval.cpp
  src/basics/fix.cpp
  src/basics/game.cpp
  src/basics/identity_set.cpp
  src/basics/options.cpp
  src/basics/player.cpp
  src/basics/player_elim.cpp
  src/basics/player_game.cpp
  src/basics/sarcastic.cpp
  src/basics/state.cpp
  src/basics/variant.cpp
  src/conventions/reactor/interpret_clue.cpp
  src/conventions/reactor/interpret_reaction.cpp
  src/conventions/reactor/interpret_reactive.cpp
  src/conventions/reactor/state_eval.cpp
  src/conventions/reactor0/colour_value.cpp
  src/conventions/reactor0/reactive_assignment.cpp
  src/conventions/reactor0/call_invariants.cpp
  src/conventions/reactor0/efficiency.cpp
  src/conventions/reactor0/interpret_clue.cpp
  src/conventions/reactor0/blind_stable.cpp
  src/conventions/reactor0/synesthesia_stable.cpp
  src/conventions/reactor0/decision.cpp
  src/conventions/reactor0/calls.cpp
  src/conventions/reactor0/interpret_reaction.cpp
  src/conventions/reactor0/interpret_reactive.cpp
  src/conventions/reactor0/state_eval.cpp
  src/conventions/tiiah/buckets.cpp
  src/conventions/tiiah/dupes.cpp
  src/conventions/tiiah/interpret_clue.cpp
  src/conventions/tiiah/interpret_reactive.cpp
  src/conventions/tiiah/settings.cpp
  src/conventions/tiiah/superposition.cpp
  src/conventions/variants/brownish.cpp
  src/conventions/variants/hole.cpp
  src/conventions/variants/inverted.cpp
  src/conventions/variants/pinkish.cpp
  src/conventions/variants/predicates.cpp
  src/conventions/variants/reactive_table.cpp
  src/endgame/forced_endgame.cpp
  src/endgame/helper.cpp
  src/endgame/solver.cpp
  src/endgame/winnable.cpp
  src/instrumentation/timer.cpp
  src/logging/decide_trace.cpp
  src/logging/game_logger.cpp
  src/logging/state_snapshot.cpp
  src/net/auth.cpp
  src/net/codec.cpp
  src/net/commands.cpp
  src/net/notes.cpp
  src/net/ws_transport.cpp
  src/settings.cpp
)
target_include_directories(hanabi_core PUBLIC include)
target_include_directories(hanabi_core PRIVATE ${cpp_httplib_SOURCE_DIR})
target_link_libraries(hanabi_core PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog
                                          OpenSSL::SSL OpenSSL::Crypto Boost::headers)
if(WIN32)
  # On macOS/Linux the socket API lives in libc, so no platform libraries are
  # needed. On Windows it lives in Winsock: Boost.Asio/Beast and cpp-httplib
  # both resolve select/WSARecv/WSASend/getaddrinfo/... from ws2_32, Asio's
  # AcceptEx/ConnectEx from mswsock, and OpenSSL's Windows certificate-store
  # loader from crypt32.
  target_link_libraries(hanabi_core PUBLIC ws2_32 mswsock crypt32)
endif()
target_compile_definitions(hanabi_core PUBLIC
  HANABI_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)
hanabi_set_warnings(hanabi_core)

# Self-play harness (self_play_diagnostics/README.md): a simulated server
# driving one bot Game per seat, plus the issue detectors. A library so that
# hanabi_tests can unit-test the wire it builds.
add_library(hanabi_selfplay STATIC
  self_play_diagnostics/sim.cpp
  self_play_diagnostics/diagnostics.cpp
)
target_include_directories(hanabi_selfplay PUBLIC self_play_diagnostics)
target_link_libraries(hanabi_selfplay PUBLIC hanabi_core)
hanabi_set_warnings(hanabi_selfplay)

# --- Tests --------------------------------------------------------------
enable_testing()
include(GoogleTest)

# Tests are split into three executables so a convention-scoped run is
# possible. `gtest_discover_tests` applies PROPERTIES per TARGET (never per
# source file), so the ctest labels below require the split — see CLAUDE.md
# "Running tests" for the invocations. Note ctest's -L takes a REGEX, so an
# unanchored `-L reactor` also matches `reactor0`; the documented commands
# anchor as `-L '^(core|reactor0)$'`.
#
# hanabi_tests / label `core`: convention-NEUTRAL engine tests. These must
# pass for either convention, so every scoped run includes them.
add_executable(hanabi_tests
  tests/test_smoke.cpp
  tests/test_harness.cpp
  tests/test_harness_smoke.cpp
  tests/test_basics/test_identity_set.cpp
  tests/test_basics/test_clue.cpp
  tests/test_basics/test_action.cpp
  tests/test_basics/test_odds_and_evens.cpp
  tests/test_basics/test_blind.cpp
  tests/test_basics/test_synesthesia.cpp
  tests/test_basics/test_alternating_clues.cpp
  tests/test_basics/test_variants.cpp
  tests/test_basics/test_variant_reversed.cpp
  tests/test_basics/test_state.cpp
  tests/test_basics/test_game.cpp
  tests/test_basics/test_chop.cpp
  tests/test_basics/test_snapshot_convention.cpp
  tests/test_basics/test_reactive.cpp
  tests/test_basics/test_stable_ref_discard.cpp
  tests/test_basics/test_sarcastic_needs_a_known_identity.cpp
  tests/test_basics/test_strike_preserves_ctp.cpp
  tests/test_basics/test_snapshot_roundtrip.cpp
  tests/test_endgame/test_forced_endgame.cpp
  tests/test_endgame/test_two_criticals_dead_partner.cpp
  tests/test_endgame/test_partner_needs_two_turns.cpp
  tests/test_endgame/test_helper.cpp
  tests/test_endgame/test_certain_play.cpp
  tests/test_endgame/test_required_play.cpp
  tests/test_endgame/test_timeout_precheck.cpp
  tests/test_endgame/test_orange_chuck.cpp
  tests/test_endgame/test_solver_smoke.cpp
  tests/test_logging/test_game_logger_rename.cpp
  tests/test_logging/test_game_logger_finalize.cpp
  tests/test_logging/test_game_logger_shared_lifetime.cpp
  tests/test_net/test_allplays_scope.cpp
  tests/test_net/test_notes.cpp
  tests/test_net/test_codec.cpp
  tests/test_net/test_commands.cpp
  tests/test_net/test_finish_ongoing_game.cpp
  tests/test_net/test_setall.cpp
  tests/test_net/test_tiiah_commands.cpp
  tests/test_net/test_help_and_announce.cpp
  tests/test_net/test_set_persistence.cpp
  tests/test_net/test_game_lifecycle.cpp
  tests/test_selfplay/test_sim_wire.cpp
  tests/test_selfplay/test_diagnostics.cpp
)
target_include_directories(hanabi_tests PRIVATE tests)
target_link_libraries(hanabi_tests PRIVATE hanabi_core hanabi_selfplay GTest::gtest_main)
hanabi_set_warnings(hanabi_tests)
gtest_discover_tests(hanabi_tests PROPERTIES LABELS "core")

# hanabi_reactor_tests / label `reactor`: the reactor convention's own
# behaviour, plus every replay regression — replays reconstruct under the
# convention recorded in their snapshot, which for all existing fixtures is
# reactor. A reactor0-only change cannot affect these; a shared-engine change
# can, which is why the full suite is still the pre-commit gate.
add_executable(hanabi_reactor_tests
  tests/test_harness.cpp
  tests/test_reactor/test_reactive_table.cpp
  tests/test_reactor/test_ctd_revives.cpp
  tests/test_reactor/test_reactive_inferred_narrow.cpp
  tests/test_reactor/test_reversed.cpp
  tests/test_reactor/test_allplays.cpp
  tests/test_reactor/test_odds_and_evens.cpp
  tests/test_reactor/test_orange.cpp
  tests/test_reactor/test_discard_penalty.cpp
  tests/test_reactor/test_dc_target_retarget.cpp
  tests/test_reactor/test_same_hand_dupe_playables.cpp
  tests/test_reactor/test_bad_reactive_target/test_replay_1916813_clued_trash_over_unclued_trash.cpp
  tests/test_reactor/test_bad_reactive_target/test_replay_1916888_known_trash_targeted_over_unknown.cpp
  tests/test_reactor/test_stacked_plays/test_replay_1916815_play_target_stacked_on_ctp.cpp
  tests/test_reactor/test_endgame/test_replay_1874799.cpp
  tests/test_reactor/test_endgame/test_replay_1875252.cpp
  tests/test_reactor/test_endgame/test_replay_1875304.cpp
  tests/test_reactor/test_endgame/test_replay_1875305.cpp
  tests/test_reactor/test_misc/test_replay_1876746.cpp
  tests/test_reactor/test_misc/test_replay_1882268.cpp
  tests/test_reactor/test_misc/test_replay_1882549.cpp
  tests/test_reactor/test_misc/test_replay_1882573.cpp
  tests/test_reactor/test_misc/test_replay_1883183.cpp
  tests/test_reactor/test_misc/test_replay_1883510.cpp
  tests/test_reactor/test_misc/test_replay_1883535.cpp
  tests/test_reactor/test_misc/test_replay_1883650.cpp
  tests/test_reactor/test_misc/test_replay_1883676.cpp
  tests/test_reactor/test_misc/test_replay_1883728.cpp
  tests/test_reactor/test_misc/test_replay_1884033.cpp
  tests/test_reactor/test_misc/test_replay_1884157.cpp
  tests/test_reactor/test_misc/test_replay_1884192.cpp
  tests/test_reactor/test_misc/test_replay_1884348.cpp
  tests/test_reactor/test_misc/test_replay_1884355.cpp
  tests/test_reactor/test_misc/test_replay_1884410.cpp
  tests/test_reactor/test_misc/test_replay_1884469.cpp
  tests/test_reactor/test_endgame/test_replay_1885375.cpp
  tests/test_reactor/test_endgame/test_replay_1885467.cpp
  tests/test_reactor/test_misc/test_replay_1885527.cpp
  tests/test_reactor/test_misc/test_replay_1885536.cpp
  tests/test_reactor/test_misc/test_replay_1885550.cpp
  tests/test_reactor/test_misc/test_replay_1885749.cpp
  tests/test_reactor/test_endgame/test_replay_1885855.cpp
  tests/test_reactor/test_misc/test_replay_1885899.cpp
  tests/test_reactor/test_misc/test_replay_1888951.cpp
  tests/test_reactor/test_endgame/test_replay_1888970.cpp
  tests/test_reactor/test_misc/test_replay_1889209.cpp
  tests/test_reactor/test_misc/test_replay_1889318.cpp
  tests/test_reactor/test_misc/test_replay_1890204.cpp
  tests/test_reactor/test_misc/test_replay_1892112.cpp
  tests/test_reactor/test_misc/test_replay_1892115.cpp
  tests/test_reactor/test_stacked_plays/test_replay_1892197_reactive_4_targets_next_playable_after_ctp.cpp
  tests/test_reactor/test_misc/test_replay_1892259.cpp
  tests/test_reactor/test_misc/test_replay_1892397.cpp
  tests/test_reactor/test_misc/test_replay_1892428.cpp
  tests/test_reactor/test_endgame/test_replay_1892511.cpp
  tests/test_reactor/test_misc/test_replay_1899044.cpp
  tests/test_reactor/test_endgame/test_replay_1899527.cpp
  tests/test_reactor/test_misc/test_replay_1899552.cpp
  tests/test_reactor/test_misc/test_replay_1899567.cpp
  tests/test_reactor/test_misc/test_replay_1899623.cpp
  tests/test_reactor/test_misc/test_replay_1900437.cpp
  tests/test_reactor/test_bad_reaction/test_replay_1915981_loaded_bob_reacts_to_stable_clue.cpp
  tests/test_reactor/test_receiver_misinterpretation/test_replay_1916791_stale_wc_misses_ctd_to_ctp_promise.cpp
)
target_include_directories(hanabi_reactor_tests PRIVATE tests)
target_link_libraries(hanabi_reactor_tests PRIVATE hanabi_core GTest::gtest_main)
hanabi_set_warnings(hanabi_reactor_tests)
gtest_discover_tests(hanabi_reactor_tests PROPERTIES LABELS "reactor")

# hanabi_reactor0_tests / label `reactor0`: the reactor0 convention's own
# behaviour. A reactor0 bug report's inner loop is
# `ctest -L '^(core|reactor0)$'` — this target plus the neutral one.
add_executable(hanabi_reactor0_tests
  tests/test_harness.cpp
  tests/test_reactor0/test_allplays_parity.cpp
  tests/test_reactor0/test_dead_chuck_call.cpp
  tests/test_reactor0/test_candidate_rollback.cpp
  tests/test_reactor0/test_colour_value.cpp
  tests/test_reactor0/test_button_reveal.cpp
  tests/test_reactor0/test_call_invariants.cpp
  tests/test_reactor0/test_chuck_dupes.cpp
  tests/test_reactor0/test_known_safe_discard.cpp
  tests/test_reactor0/test_no_chuck_at_eight_clues.cpp
  tests/test_reactor0/test_deferred_elim.cpp
  tests/test_reactor0/test_reaction_elim_filter.cpp
  tests/test_reactor0/test_reaction_negative_keeps_inference.cpp
  tests/test_reactor0/test_withdrawn_target_disarms_negative.cpp
  tests/test_reactor0/test_reactive_bluff.cpp
  tests/test_reactor0/test_misc/test_replay_1983205_synesthesia_rank_map_wraps.cpp
  tests/test_reactor0/test_misc/test_replay_1969696_reversed_orange_clue_is_a_chuck.cpp
  tests/test_reactor0/test_misc/test_replay_1967558_dropped_call_keeps_its_inference.cpp
  tests/test_reactor0/test_misc/test_replay_1971788_rank_promise_is_a_parity.cpp
  tests/test_reactor0/test_misc/test_replay_1971882_finesse_elim_needs_the_connector.cpp
  tests/test_reactor0/test_misc/test_replay_1970589_elim_needs_the_alternative.cpp
  tests/test_reactor0/test_misc/test_replay_1972716_spent_reaction_is_not_urgent.cpp
  tests/test_reactor0/test_misc/test_replay_1975197_a_spent_reacter_call_relegates.cpp
  tests/test_reactor0/test_misc/test_replay_1973575_giver_sight_must_not_downgrade.cpp
  tests/test_reactor0/test_misc/test_replay_1973971_reactive_receiver_is_not_the_target.cpp
  tests/test_reactor0/test_misc/test_replay_1973976_known_orange_is_pitchable.cpp
  tests/test_reactor0/test_misc/test_replay_1973974_playable_orange_chop_is_not_saved.cpp
  tests/test_reactor0/test_misc/test_replay_1974046_known_trash_is_chucked.cpp
  tests/test_reactor0/test_misc/test_replay_1974052_known_trash_after_colour_clue.cpp
  tests/test_reactor0/test_misc/test_replay_1974218_sarcastic_needs_a_known_identity.cpp
  tests/test_reactor0/test_misc/test_replay_1974257_inverted_trash_target_is_pitched.cpp
  tests/test_reactor0/test_misc/test_replay_1974512_reacter_ctp_must_not_erase_receiver_ctp.cpp
  tests/test_reactor0/test_misc/test_replay_1977786_eight_clues_needs_a_stable_clue.cpp
  tests/test_reactor0/test_misc/test_replay_1975464_deferred_reaction_survives.cpp
  tests/test_reactor0/test_misc/test_replay_1978081_superseded_call_drops_the_reaction.cpp
  tests/test_reactor0/test_misc/test_replay_1974194_stale_reading_is_not_acted_on.cpp
  tests/test_reactor0/test_misc/test_replay_1973410_blind_clue_names_the_p5.cpp
  tests/test_reactor0/test_misc/test_replay_1973413_blind_clue_names_the_y5.cpp
  tests/test_reactor0/test_misc/test_replay_1974331_a_clued_spare_orange_is_pitchable.cpp
  tests/test_reactor0/test_deferral_keeps_call.cpp
  tests/test_reactor0/test_misc/test_replay_1966687_chuck_known_dupe_not_unknown_chop.cpp
  tests/test_reactor0/test_misc/test_replay_1966696_mixed_rank_direct_play_stamps_ctp.cpp
  tests/test_reactor0/test_misc/test_replay_1966710_reactive_ctp_survives_earlier_negative.cpp
  tests/test_reactor0/test_misc/test_replay_1966745_deferral_keeps_reacter_call.cpp
  tests/test_reactor0/test_misc/test_replay_1967279_colour_reveal_resolves_the_button.cpp
  tests/test_reactor0/test_misc/test_replay_1967287_dead_chuck_call_is_erased.cpp
  tests/test_reactor0/test_misc/test_replay_1967363_no_duplicate_play_clue.cpp
  tests/test_reactor0/test_misc/test_replay_1967376_reacter_ctd_narrows_to_chuckable.cpp
  tests/test_reactor0/test_misc/test_replay_1967416_finesse_is_parity_not_kind.cpp
  tests/test_reactor0/test_misc/test_replay_1967478_no_ctp_on_provably_trash_focus.cpp
  tests/test_reactor0/test_misc/test_replay_1967491_dark_orange_chuck_is_a_valid_reaction.cpp
  tests/test_reactor0/test_dispatch.cpp
  tests/test_reactor0/test_endgame/test_replay_1966675_sole_holder_of_both_red_fours.cpp
  tests/test_reactor0/test_endgame/test_replay_1969779_certain_play_beats_a_gamble.cpp
  tests/test_reactor0/test_endgame/test_replay_1969860_endgame_honours_the_reacter_call.cpp
  tests/test_reactor0/test_endgame/test_replay_1966757_endgame_honours_reacter_call.cpp
  tests/test_reactor0/test_endgame/test_replay_1970943_required_play_with_none_certain.cpp
  tests/test_reactor0/test_endgame/test_replay_1971808_stall_clue_priority.cpp
  tests/test_reactor0/test_endgame/test_replay_1972670_required_play_with_one_card_left.cpp
  tests/test_reactor0/test_endgame/test_replay_1974303_two_criticals_with_a_dead_partner.cpp
  tests/test_reactor0/test_endgame/test_replay_1974119_partner_needs_two_turns.cpp
  tests/test_reactor0/test_endgame/test_replay_1973602_timeout_yields_to_a_finesse.cpp
  tests/test_reactor0/test_endgame/test_replay_2005279_stall_list_takes_reverse_reactive_discard.cpp
  tests/test_reactor0/test_duplicate_play_veto.cpp
  tests/test_reactor0/test_efficiency.cpp
  tests/test_reactor0/test_giver_filters.cpp
  tests/test_reactor0/test_finesse_parity.cpp
  tests/test_reactor0/test_target_parity.cpp
  tests/test_reactor0/test_deferred_reaction.cpp
  tests/test_reactor0/test_blind_stable.cpp
  tests/test_reactor0/test_synesthesia_stable.cpp
  tests/test_reactor0/test_giver_sight_reject.cpp
  tests/test_reactor0/test_orange_chop_and_pitch.cpp
  tests/test_reactor0/test_inverted_trash_target.cpp
  tests/test_reactor0/test_pov_reject.cpp
  tests/test_reactor0/test_provably_trash_focus.cpp
  tests/test_reactor0/test_reactive_assignment.cpp
  tests/test_reactor0/test_reactive_override_effect.cpp
  tests/test_reactor0/test_reactive_dark_chuck.cpp
  tests/test_reactor0/test_react_discard_button.cpp
  tests/test_reactor0/test_reactive_colour.cpp
  tests/test_reactor0/test_reactive_lock.cpp
  tests/test_reactor0/test_dc_target_leftmost.cpp
  tests/test_reactor0/test_reactive_rank.cpp
  tests/test_reactor0/test_reversed_finesse.cpp
  tests/test_reactor0/test_stable_colour.cpp
  tests/test_reactor0/test_stable_colour_baseline.cpp
  tests/test_reactor0/test_stable_rank.cpp
  tests/test_reactor0/test_stable_chuck_drops_criticals.cpp
  tests/test_reactor0/test_odds_and_evens.cpp
  tests/test_reactor0/test_orange.cpp
  tests/test_reactor0/test_reactive_inverted_vet.cpp
  tests/test_reactor0/test_bad_reactive_target/test_replay_1942777_orange_reactive_vet_follows_swap.cpp
  tests/test_reactor0/test_bad_reactive_target/test_replay_1957942_trash_orange_pitch_is_a_valid_reaction.cpp
  tests/test_reactor0/test_bad_reactive_target/test_replay_1966351_wrong_target_from_early_dc_elim.cpp
  tests/test_reactor0/test_bad_reactive_target/test_replay_1974342_reactive_discard_survives_a_dead_chuck.cpp
  tests/test_reactor0/test_bad_reactive_target/test_replay_1973406_finesse_walks_past_a_dead_connector.cpp
  tests/test_reactor0/test_bad_reactive_target/test_replay_2005309_phase_c_rejects_on_giver_only_knowledge.cpp
  tests/test_reactor0/test_stable_rank_omni.cpp
  tests/test_reactor0/test_strike_no_reset.cpp
  tests/test_reactor0/test_urgent_skips_known_trash.cpp
  tests/test_reactor0/test_misc/test_replay_1942525_omni_rank_reads_as_direct_play.cpp
  tests/test_reactor0/test_misc/test_replay_1942458_colour_mode2_walks_dc_targets.cpp
  tests/test_reactor0/test_misc/test_replay_1957905_orange_chuck_must_be_playable.cpp
  tests/test_reactor0/test_misc/test_replay_1957936_orange_chuck_chain_wins_the_endgame.cpp
  tests/test_reactor0/test_misc/test_replay_1957953_trash_orange_is_pitched_not_chucked.cpp
  tests/test_reactor0/test_misc/test_replay_1959065_orange_chuck_call_survives_resolution.cpp
  tests/test_reactor0/test_misc/test_replay_1961419_sieved_orange_is_not_a_discard_candidate.cpp
  tests/test_reactor0/test_misc/test_replay_1966569_chucks_the_orange_not_the_trash_orange.cpp
  tests/test_reactor0/test_misc/test_replay_1966667_chop_is_chucked_not_pitched.cpp
)
target_include_directories(hanabi_reactor0_tests PRIVATE tests)
target_link_libraries(hanabi_reactor0_tests PRIVATE hanabi_core GTest::gtest_main)
hanabi_set_warnings(hanabi_reactor0_tests)
gtest_discover_tests(hanabi_reactor0_tests PROPERTIES LABELS "reactor0")

# --- Decision-making tests -----------------------------------------------
# Behavioural-quality tests. Failures here mean the bot made a suboptimal
# (but not necessarily incorrect) decision. They live under a separate
# executable + ctest label so they can be skipped via `ctest -LE
# decision_making` when only correctness regression matters. Per
# CLAUDE.md, decision-test failures need manual review (either bot fix or
# test change) before being silenced.
add_executable(hanabi_decision_tests
  tests/test_harness.cpp
  tests/test_reactor/test_decision_making/test_low_clue_count.cpp
  tests/test_reactor/test_decision_making/test_high_value_clue.cpp
  tests/test_reactor/test_decision_making/test_replay_1916933_clue_forces_discard_of_queued_playable.cpp
  tests/test_reactor0/test_decision_making/test_clue_tier.cpp
  tests/test_reactor0/test_decision_making/test_clue_shape.cpp
  tests/test_reactor0/test_decision_making/test_reading_holders.cpp
  tests/test_tiiah/test_decision_making/test_clue_reading.cpp
  tests/test_tiiah/test_decision_making/test_ordinary_reactive_reading.cpp
  tests/test_tiiah/test_decision_making/test_refusal_clue.cpp
  tests/test_tiiah/test_decision_making/test_colour_preference.cpp
  tests/test_tiiah/test_decision_making/test_fix_clue_priority.cpp
  tests/test_tiiah/test_decision_making/test_replay_2011327_trash_reveal_over_lock.cpp
  tests/test_tiiah/test_decision_making/test_replay_2013726_fix_dead_call_with_two.cpp
  tests/test_tiiah/test_decision_making/test_own_dupe_filter.cpp
  tests/test_tiiah/test_decision_making/test_replay_2013726_save_playable_chop_despite_own_dupe.cpp
  tests/test_tiiah/test_decision_making/test_replay_2014076_save_bobs_playable_chop_while_occupied.cpp
  tests/test_tiiah/test_decision_making/test_occupied_save_of_bobs_chop.cpp
  tests/test_tiiah/test_decision_making/test_replay_2014538_color_stable_play_over_rank_stall.cpp
  tests/test_tiiah/test_decision_making/test_replay_2014538_stable_play_over_lock_at_one_clue.cpp
  tests/test_tiiah/test_decision_making/test_replay_2014561_stable_play_hierarchy_prefers_purple.cpp
  tests/test_tiiah/test_decision_making/test_stable_play_hierarchy.cpp
  tests/test_tiiah/test_decision_making/test_replay_2014561_named_stable_play_past_own_dupe_veto.cpp
  tests/test_tiiah/test_decision_making/test_replay_2014884_rainbow_one_clued_by_rank.cpp
  tests/test_tiiah/test_decision_making/test_replay_2011327_stall_drawn_card_not_blind_played.cpp
  tests/test_reactor0/test_decision_making/test_trash_reveal_of_a_clued_card.cpp
  tests/test_reactor0/test_decision_making/test_stall_drawn_card.cpp
  tests/test_reactor0/test_decision_making/test_locked_hand_throw.cpp
  tests/test_reactor0/test_decision_making/test_clue_priority.cpp
  tests/test_reactor0/test_decision_making/test_stable_play_before_lock.cpp
  tests/test_reactor0/test_decision_making/test_calls.cpp
  tests/test_reactor0/test_decision_making/test_replay_1966091_lock_must_not_outrank_reaction.cpp
  tests/test_reactor0/test_decision_making/test_replay_1966119_occupied_low_pace_actions_its_call.cpp
  tests/test_reactor0/test_decision_making/test_replay_1966286_call_inference_not_renarrowed_by_stacks.cpp
  tests/test_reactor0/test_decision_making/test_replay_1966633_locked_alice_gives_the_play_reveal.cpp
  tests/test_reactor0/test_decision_making/test_replay_1966653_locked_alice_is_exempt_from_the_tier_gate.cpp
  tests/test_reactor0/test_decision_making/test_replay_1973996_pace_zero_reaches_section_four.cpp
  tests/test_reactor0/test_decision_making/test_replay_1943094_pace_zero_still_plays.cpp
  tests/test_reactor0/test_decision_making/test_replay_1973281_rung38_needs_a_critical_chop.cpp
  tests/test_reactor0/test_decision_making/test_pace_clue_gate.cpp
  tests/test_reactor0/test_decision_making/test_low_pace_clues.cpp
  tests/test_reactor0/test_decision_making/test_section_four_gate.cpp
  tests/test_reactor0/test_decision_making/test_replay_1978799_pace_one_reaches_section_four.cpp
  tests/test_reactor0/test_decision_making/test_replay_1981703_dead_call_is_not_occupied.cpp
  tests/test_reactor0/test_decision_making/test_double_discard_filter.cpp
  tests/test_reactor0/test_decision_making/test_replay_1942330_playable_chop_lifts_clue_tier.cpp
  tests/test_reactor0/test_decision_making/test_replay_1942181_prefers_stable_play_over_double_discard.cpp
  tests/test_reactor0/test_decision_making/test_replay_1981749_reactive_discard_prefers_trash.cpp
  tests/test_reactor0/test_decision_making/test_replay_2005248_duped_playable_chop_can_wait.cpp
)
target_include_directories(hanabi_decision_tests PRIVATE tests)
target_link_libraries(hanabi_decision_tests PRIVATE hanabi_core GTest::gtest_main)
hanabi_set_warnings(hanabi_decision_tests)
gtest_discover_tests(hanabi_decision_tests PROPERTIES LABELS "decision_making")

# hanabi_tiiah_tests / label `tiiah`: the Throw It in a Hole convention, plus
# the engine rules its variants turn on. Those rules key on the VARIANT flag
# rather than on the convention (a 4+ player TIIAH table runs reactor and still
# needs them), so a few of these fixtures deliberately sit under another
# convention.
add_executable(hanabi_tiiah_tests
  tests/test_harness.cpp
  tests/test_tiiah/test_variant_flag.cpp
  tests/test_tiiah/test_convention_predicates.cpp
  tests/test_tiiah/test_buckets.cpp
  tests/test_tiiah/test_engine_rules.cpp
  tests/test_tiiah/test_gate_and_clues.cpp
  tests/test_tiiah/test_superposition.cpp
  tests/test_tiiah/test_pairwise_stacks.cpp
  tests/test_tiiah/test_conditional_reading.cpp
  tests/test_tiiah/test_presumed_landing.cpp
  tests/test_tiiah/test_reverse_reactive.cpp
  tests/test_tiiah/test_role_inversion.cpp
  tests/test_tiiah/test_all_targets_gotten.cpp
  tests/test_tiiah/test_fix_before_reverse_reactive.cpp
  tests/test_tiiah/test_deferred_collapse.cpp
  tests/test_tiiah/test_named_call_on_a_watched_dupe.cpp
  tests/test_tiiah/test_reverse_reactive_confirmation.cpp
  tests/test_tiiah/test_replay_2013726_reacter_rules_out_the_n3.cpp
  tests/test_tiiah/test_ordinary_reactive.cpp
  tests/test_tiiah/test_receiver_bucket.cpp
  tests/test_tiiah/test_replay_2008177_ordinary_reactive_not_read.cpp
  tests/test_tiiah/test_replay_2008217_receiver_misses_the_bucket.cpp
  tests/test_tiiah/test_replay_2008422_receiver_play_in_the_hole_is_not_a_discard.cpp
  tests/test_tiiah/test_replay_2009367_bucket_reading_depends_on_our_hole_card.cpp
  tests/test_tiiah/test_replay_2009367_stable_clue_to_cathy_refuses_the_reactive.cpp
  tests/test_tiiah/test_replay_2010246_illegal_bucket_pairing_not_retargeted.cpp
  tests/test_tiiah/test_replay_2010296_partner_play_is_presumed_to_land.cpp
  tests/test_tiiah/test_replay_2010329_receiver_reads_its_own_worlds.cpp
  tests/test_tiiah/test_replay_2010329_a_withdrawn_call_keeps_its_inference.cpp
  tests/test_tiiah/test_replay_2010329_the_frame_is_the_pair_not_the_team.cpp
  tests/test_tiiah/test_pair_visible_copies.cpp
  tests/test_tiiah/test_own_plays_presumed_to_land.cpp
  tests/test_tiiah/test_row_from_own_worlds.cpp
  tests/test_tiiah/test_replay_2010512_receiver_reads_the_reacter_play.cpp
  tests/test_tiiah/test_reacter_play_read_by_the_receiver.cpp
  tests/test_tiiah/test_fix_clue.cpp
  tests/test_tiiah/test_replay_2011133_rule_six_is_shared.cpp
  tests/test_tiiah/test_replay_2011319_discarded_playable_was_already_played.cpp
  tests/test_tiiah/test_replay_2011327_a_known_play_is_in_every_view.cpp
  tests/test_tiiah/test_known_play_floor.cpp
  tests/test_tiiah/test_replay_2011397_the_frame_is_the_minimum_across_worlds.cpp
  tests/test_tiiah/test_world_feasibility.cpp
  tests/test_tiiah/test_stable_clue_over_worlds.cpp
  tests/test_tiiah/test_replay_2011475_dupes_strikes_and_the_shared_collapse.cpp
  tests/test_tiiah/test_dupes.cpp
  tests/test_tiiah/test_replay_2011830_receiver_bucket_reading_not_the_stamp.cpp
  tests/test_tiiah/test_receiver_reading_beats_the_stamp.cpp
  tests/test_tiiah/test_replay_2011854_refusal_and_the_dupe_strike.cpp
  tests/test_tiiah/test_refusal_and_dupe_strike.cpp
  tests/test_tiiah/test_replay_2011887_undeferred_call_not_dropped.cpp
  tests/test_tiiah/test_replay_2012424_reaction_call_live_in_a_world.cpp
  tests/test_tiiah/test_replay_2013616_reacts_after_the_pair_settles.cpp
  tests/test_tiiah/test_replay_2013645_role_inversion_keeps_the_clue_stable.cpp
  tests/test_tiiah/test_replay_2013645_stable_four_under_role_inversion.cpp
  tests/test_tiiah/test_replay_2013726_endgame_pairing_may_break_the_bucket.cpp
  tests/test_tiiah/test_replay_2013726_reverse_reactive_finesse_on_a_called_play.cpp
  tests/test_tiiah/test_replay_2013963_reverse_reactive_finesse_on_a_settled_call.cpp
  tests/test_tiiah/test_replay_2014076_deferred_collapse_on_a_retouched_card.cpp
  tests/test_tiiah/test_replay_2014402_finesse_read_before_dead_call_check.cpp
  tests/test_tiiah/test_known_play_standing_call.cpp
  tests/test_tiiah/test_replay_2011885_named_card_not_absorbed.cpp
  tests/test_tiiah/test_world_replay_band.cpp
  tests/test_tiiah/test_shared_rule_six.cpp
  tests/test_tiiah/test_bucket_encoding.cpp
  tests/test_tiiah/test_bucket_legality.cpp
  tests/test_tiiah/test_reactions.cpp
  tests/test_tiiah/test_rainbowy.cpp
)
target_include_directories(hanabi_tiiah_tests PRIVATE tests)
target_link_libraries(hanabi_tiiah_tests PRIVATE hanabi_core GTest::gtest_main)
hanabi_set_warnings(hanabi_tiiah_tests)
gtest_discover_tests(hanabi_tiiah_tests PROPERTIES LABELS "tiiah")

# --- Apps ----------------------------------------------------------------
add_executable(hanabi_bot apps/hanabi_bot.cpp)
target_link_libraries(hanabi_bot PRIVATE hanabi_core)
hanabi_set_warnings(hanabi_bot)

# The same bot under another name, so the current source can be built and run
# while build/hanabi_bot.exe is running (and therefore locked on Windows).
# `./build.sh --test` builds it.
add_executable(hanabi_bot_test apps/hanabi_bot.cpp)
target_link_libraries(hanabi_bot_test PRIVATE hanabi_core)
hanabi_set_warnings(hanabi_bot_test)

add_executable(bench_endgame apps/bench_endgame.cpp)
target_link_libraries(bench_endgame PRIVATE hanabi_core)
hanabi_set_warnings(bench_endgame)

# Standalone tool: reconstruct a Game from a per-game JSONL log and either
# re-run take_action with the current build (`--rerun`) or emit a regression
# test scaffold (`--emit-test`). The point of the logging overhaul: turn
# 30-minute "re-simulate from turn 1" loops into 1-second reruns.
add_executable(replay_log apps/replay_log.cpp)
target_link_libraries(replay_log PRIVATE hanabi_core)
hanabi_set_warnings(replay_log)

# Self-play: N seeded games between three bots, with the issue detectors.
add_executable(self_play self_play_diagnostics/self_play.cpp)
target_link_libraries(self_play PRIVATE hanabi_selfplay)
hanabi_set_warnings(self_play)
