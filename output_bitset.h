#pragma once

#include <vector>

#include "mask_library.h"
#include "output_type.h"

// Efficiently represents a set of outputs of a network using a bitset.
// Starts with all 2^n possible outputs and applies comparators to reduce the
// set. It is more memory-efficient than storing outputs explicitly when the
// set is large.
class OutputBitset {
public:
  // Initializes the bitset with all 2^n possible outputs.
  explicit OutputBitset(int n);
  // Applies a comparator (i, j) to the output set.
  // For each output where bit i > bit j, swaps the bits.
  // Requires: i < j
  void AddComparator(int i, int j);
  // Converts the bitset representation to a sparse vector of OutputType values.
  std::vector<OutputType> ToSparse() const;

private:
  int n_ = 0;     // Number of channels
  BitSet bitset_; // Bitset representing which outputs are present
};