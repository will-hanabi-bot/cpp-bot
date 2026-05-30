// Port of python-bot/src/hanabi_bot/basics/connection.py.
// Original Scala: scala-bot/src/scala_bot/basics/Connection.scala.
//
// The state-dependent helper WaitingConnection::get_next_index is implemented
// in src/basics/connection.cpp once State is available (Phase 2).
#pragma once

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"

namespace hanabi {

struct State;

enum class FinesseKind : std::uint8_t {
  TRUE_K,           // "True" — kind=true; named TRUE_K to avoid the macro.
  HIDDEN,
  CERTAIN,
  POSSIBLY_BLUFF,
  BLUFF,
};

struct KnownConn {
  int reacting;
  int order;
  Identity id;

  std::vector<Identity> ids() const { return {id}; }
  static constexpr const char* kind = "known";
  bool hidden() const { return false; }
  bool is_bluff() const { return false; }
  bool is_possibly_bluff() const { return false; }

  auto operator<=>(const KnownConn&) const = default;
};

struct PlayableConn {
  int reacting;
  int order;
  Identity id;
  std::vector<int> linked;
  bool hidden_flag = false;
  // Orders of connecting cards this card is being layered in front of (Layered Finesse).
  std::optional<std::vector<int>> inserting_into;

  std::vector<Identity> ids() const { return {id}; }
  static constexpr const char* kind = "playable";
  bool hidden() const { return hidden_flag; }
  bool is_bluff() const { return false; }
  bool is_possibly_bluff() const { return false; }

  bool operator==(const PlayableConn&) const = default;
};

struct PromptConn {
  int reacting;
  int order;
  Identity id;
  bool hidden_flag = false;

  std::vector<Identity> ids() const { return {id}; }
  static constexpr const char* kind = "prompt";
  bool hidden() const { return hidden_flag; }
  bool is_bluff() const { return false; }
  bool is_possibly_bluff() const { return false; }

  auto operator<=>(const PromptConn&) const = default;
};

struct FinesseConn {
  int reacting;
  int order;
  std::vector<Identity> ids_vec;
  FinesseKind f_kind;
  bool ambiguous_passback = false;

  const std::vector<Identity>& ids() const { return ids_vec; }
  const char* kind() const {
    if (f_kind == FinesseKind::POSSIBLY_BLUFF) return "bluff?";
    if (f_kind == FinesseKind::BLUFF) return "bluff";
    return "finesse";
  }
  bool hidden() const { return f_kind == FinesseKind::HIDDEN; }
  bool possibly_bluff() const { return f_kind == FinesseKind::POSSIBLY_BLUFF; }
  bool bluff() const { return f_kind == FinesseKind::BLUFF; }
  bool certain() const { return f_kind == FinesseKind::CERTAIN; }
  bool is_bluff() const { return f_kind == FinesseKind::BLUFF; }
  bool is_possibly_bluff() const {
    return f_kind == FinesseKind::POSSIBLY_BLUFF || f_kind == FinesseKind::BLUFF;
  }

  bool operator==(const FinesseConn&) const = default;
};

struct PositionalConn {
  int reacting;
  int order;
  std::vector<Identity> ids_vec;
  // If our involvement is ambiguous, (target_order, playable_possibilities).
  std::optional<std::pair<int, std::vector<Identity>>> ambiguous_own;

  const std::vector<Identity>& ids() const { return ids_vec; }
  static constexpr const char* kind = "positional";
  bool hidden() const { return false; }
  bool is_bluff() const { return false; }
  bool is_possibly_bluff() const { return false; }

  bool operator==(const PositionalConn&) const = default;
};

using Connection =
    std::variant<KnownConn, PlayableConn, PromptConn, FinesseConn, PositionalConn>;

// Polymorphic accessors over Connection variants.
inline int reacting(const Connection& c) {
  return std::visit([](const auto& v) { return v.reacting; }, c);
}
inline int order(const Connection& c) {
  return std::visit([](const auto& v) { return v.order; }, c);
}
inline std::vector<Identity> ids(const Connection& c) {
  return std::visit([](const auto& v) -> std::vector<Identity> { return v.ids(); }, c);
}
inline bool hidden(const Connection& c) {
  return std::visit([](const auto& v) { return v.hidden(); }, c);
}
inline bool is_bluff(const Connection& c) {
  return std::visit([](const auto& v) { return v.is_bluff(); }, c);
}
inline bool is_possibly_bluff(const Connection& c) {
  return std::visit([](const auto& v) { return v.is_possibly_bluff(); }, c);
}

struct FocusPossibility {
  Identity id;
  std::vector<Connection> connections;
  Interp interp;
  bool symmetric = false;
  bool ambiguous = false;
  bool save = false;
  bool complicated = false;

  bool is_bluff() const {
    return !connections.empty() && hanabi::is_bluff(connections.front());
  }
};

struct WaitingConnection {
  std::vector<Connection> connections;
  int giver;
  int target;
  int turn;
  int focus;
  Identity inference;
  bool ambiguous_passback = false;
  bool self_passback = false;
  bool symmetric = false;
  bool ambiguous_self = false;

  const Connection& curr_conn() const { return connections.front(); }

  // Implemented in src/basics/connection.cpp (depends on State, lands Phase 2).
  std::optional<int> get_next_index(const State& state) const;
};

}  // namespace hanabi
