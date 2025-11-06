#pragma once

#include <map>
#include <vector>

#include "boost/dynamic_bitset.hpp"
#include "glog/logging.h"
#include "gtest/gtest_prod.h"

using BitSet = boost::dynamic_bitset<>;

// Singleton class providing precomputed bit masks for efficient set operations.
// For n channels, provides masks for common patterns used in sorting network
// algorithms.
class MaskLibrary {
public:
  // Gets or creates the singleton instance for n channels.
  static const MaskLibrary &GetInstance(int n);

  // Returns the number of channels.
  int n() const { return n_; }

  // Returns the set of n-bit binary numbers where bit i is 0.
  const BitSet &Mask0(int i) const { return mask0_.at(i); }
  // Returns the set of n-bit binary numbers where bit i is 1.
  const BitSet &Mask1(int i) const { return mask1_.at(i); }
  // Returns the set of n-bit binary numbers where bit i is 1 and bit j is 0.
  const BitSet &Mask10(int i, int j) const {
    const BitSet &mask = mask10_.at(i).at(j);
    CHECK_NE(mask.size(), 0);
    return mask;
  }
  // Returns the set of n-bit binary numbers with exactly 'popcount' bits set
  // to 1 (0 <= popcount <= n).
  const BitSet &MaskByPopcount(int popcount) const {
    return mask_by_weight_.at(popcount);
  }

private:
  // Constructs a mask library for n channels.
  // If full_size is true, computes all mask10_ entries (including i >= j).
  MaskLibrary(int n, bool full_size = false);

  int n_ = 0;
  // Precomputed masks: mask0_[i] = {x | bit i of x is 0}
  std::vector<BitSet> mask0_;
  // Precomputed masks: mask1_[i] = {x | bit i of x is 1}
  std::vector<BitSet> mask1_;
  // Precomputed masks: mask10_[i][j] = {x | bit i of x is 1 and bit j of x is
  // 0}
  std::vector<std::vector<BitSet>> mask10_;
  // Precomputed masks: mask_by_weight_[w] = {x | popcount(x) == w}
  std::vector<BitSet> mask_by_weight_;

  // Singleton cache: maps n to the corresponding MaskLibrary instance.
  static std::map<int, MaskLibrary> n_to_instance_;

  FRIEND_TEST(MaskLibrary, Basic);
  FRIEND_TEST(MaskLibrary, MaskByPopcount);
  FRIEND_TEST(MaskLibrary, Time17);
};
