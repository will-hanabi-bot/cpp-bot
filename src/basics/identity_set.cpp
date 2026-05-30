#include "hanabi/basics/identity_set.h"

#include <sstream>

namespace hanabi {

std::string IdentitySet::to_string() const {
  std::ostringstream os;
  os << "IdentitySet({";
  bool first = true;
  for (Identity id : *this) {
    if (!first) os << ", ";
    first = false;
    os << "Identity(" << static_cast<int>(id.suit_index) << ", "
       << static_cast<int>(id.rank) << ")";
  }
  os << "})";
  return os.str();
}

}  // namespace hanabi
