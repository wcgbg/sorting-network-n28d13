#pragma once

#include <compare>

#include "glog/logging.h"

// Represents a comparator in a sorting network.
// A comparator compares two channels at positions i and j (where i < j),
class Comparator {
public:
  // Constructs a comparator between channels i and j.
  // Requires: 0 <= i < j
  Comparator(int i, int j) : i_(i), j_(j) {
    CHECK_LE(0, i);
    CHECK_LT(i, j);
  }
  // Returns the first channel index (i < j).
  int i() const { return i_; }
  // Returns the second channel index (i < j).
  int j() const { return j_; }
  // Comparison operator for ordering comparators.
  std::strong_ordering operator<=>(const Comparator &other) const = default;

private:
  // Channel indices: i_ < j_
  int i_ = 0;
  int j_ = 0;
};
