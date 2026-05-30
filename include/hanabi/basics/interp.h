// Port of python-bot/src/hanabi_bot/basics/interp.py.
// Original Scala: scala-bot/src/scala_bot/basics/Game.scala lines 30-42.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

namespace hanabi {

enum class ClueInterp : std::uint8_t {
  MISTAKE,
  REACTIVE,
  PLAY,
  SAVE,
  DISCARD,
  LOCK,
  REVEAL,
  FIX,
  STALL,
  DISTRIBUTION,
  USELESS,
};

enum class PlayInterp : std::uint8_t {
  NONE,
  MISTAKE,
  ORDER_CM,
};

enum class DiscardInterp : std::uint8_t {
  NONE,
  MISTAKE,
  SARCASTIC,
  GENTLEMANS_DISCARD,
  EMERGENCY,
  POSITIONAL,
  SACRIFICE,
};

using Interp = std::variant<ClueInterp, PlayInterp, DiscardInterp>;

constexpr std::string_view name(ClueInterp i) {
  switch (i) {
    case ClueInterp::MISTAKE: return "Mistake";
    case ClueInterp::REACTIVE: return "Reactive";
    case ClueInterp::PLAY: return "Play";
    case ClueInterp::SAVE: return "Save";
    case ClueInterp::DISCARD: return "Discard";
    case ClueInterp::LOCK: return "Lock";
    case ClueInterp::REVEAL: return "Reveal";
    case ClueInterp::FIX: return "Fix";
    case ClueInterp::STALL: return "Stall";
    case ClueInterp::DISTRIBUTION: return "Distribution";
    case ClueInterp::USELESS: return "Useless";
  }
  return {};
}

constexpr std::string_view name(PlayInterp i) {
  switch (i) {
    case PlayInterp::NONE: return "None";
    case PlayInterp::MISTAKE: return "Mistake";
    case PlayInterp::ORDER_CM: return "OrderCM";
  }
  return {};
}

constexpr std::string_view name(DiscardInterp i) {
  switch (i) {
    case DiscardInterp::NONE: return "None";
    case DiscardInterp::MISTAKE: return "Mistake";
    case DiscardInterp::SARCASTIC: return "Sarcastic";
    case DiscardInterp::GENTLEMANS_DISCARD: return "GentlemansDiscard";
    case DiscardInterp::EMERGENCY: return "Emergency";
    case DiscardInterp::POSITIONAL: return "Positional";
    case DiscardInterp::SACRIFICE: return "Sacrifice";
  }
  return {};
}

}  // namespace hanabi
