#include "hanabi/conventions/tiiah/settings.h"

#include <string>
#include <vector>

#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/colour_value.h"
#include "hanabi/conventions/tiiah/buckets.h"
#include "hanabi/conventions/variants/predicates.h"

namespace hanabi::tiiah {

namespace {

// One letter per suit, so the bucket table fits: the suit's own abbreviation
// where it has one, else the initial of its name.
std::string short_suit(const Suit& suit) {
  if (suit.abbreviation) return std::string(1, *suit.abbreviation);
  return suit.name.empty() ? "?" : suit.name.substr(0, 1);
}

}  // namespace

std::string format_settings(const Variant& variant) {
  std::string anchors = "{";
  bool first = true;
  for (size_t i = 0; i < variant.clue_colour_names.size(); ++i) {
    if (!first) anchors += ", ";
    first = false;
    anchors += variant.clue_colour_names[i] + "=" +
               std::to_string(reactor0::colour_clue_value(
                   variant, static_cast<int>(i)));
  }
  for (int r : variant.clue_ranks) {
    if (!first) anchors += ", ";
    first = false;
    anchors += std::to_string(r) + "=" +
               std::to_string(hanabi::reactor::variants::rank_reactive_value(
                   variant, r));
  }
  anchors += "}";

  std::string buckets;
  for (const auto& bucket : variant_buckets(variant)) {
    buckets += "[";
    for (size_t i = 0; i < bucket.size(); ++i) {
      if (i) buckets += ",";
      buckets += short_suit(variant.suits[bucket[i]]);
    }
    buckets += "]";
  }

  return "tiiah — a clue to BOB is REACTIVE when Bob has a known play and Cathy "
         "does not (Cathy reacts, Bob receives), else stable as reactor0; every "
         "reactive is EVEN, anchors " +
         anchors + "; buckets " + buckets +
         (six_suit_buckets(variant)
              ? "; EXPERIMENTAL six buckets: with epoch = ceil(turn / 3), rank: the "
                "target sits one bucket UP from the reacter's card on an odd epoch, "
                "two UP on an even one, colour as many DOWN"
              : "; rank: the target sits one bucket UP from the reacter's card, "
                "colour one DOWN") +
         "; inverted suits are in no bucket and are skipped as targets "
         "unless nothing else plays, when the clue is a double chuck";
}

}  // namespace hanabi::tiiah
