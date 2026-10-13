#include "hanabi/conventions/tiiah/settings.h"

#include <string>

#include "hanabi/basics/variant.h"
#include "hanabi/conventions/tiiah/buckets.h"

namespace hanabi::tiiah {

std::string format_settings(const Variant& variant) {
  if (epoch_shift(variant)) {
    return "Each non-orange suit gets a # based on its stack position starting "
           "with red=0. Epoch = ceiling(turn # / 3). All clues to Cathy are even "
           "reactive. Role inversion on. Rank means Cathy's suit = Bob's suit + "
           "(1 if odd epoch else 2), color means Cathy = Bob - (1 if odd epoch "
           "else 2). Orange tgt = 2 disc";
  }
  return "Each non-orange suit gets a # based on its stack position starting with "
         "red=0. All clues to Cathy are even reactive. Role inversion on. Rank "
         "means Cathy's suit = Bob's suit + 1, while color means Cathy's suit = "
         "Bob's suit - 1. Orange tgt = 2 disc";
}

}  // namespace hanabi::tiiah
