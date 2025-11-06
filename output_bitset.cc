#include "output_bitset.h"

#include "glog/logging.h"

#include "output_type.h"

OutputBitset::OutputBitset(int n) : n_(n) {
  CHECK_GT(n, 0);
  CHECK_LT(n, sizeof(OutputType) * 8);
  bitset_.resize(OutputType(1) << n, true);
}

void OutputBitset::AddComparator(int i, int j) {
  // Efficiently apply comparator (i, j) using bitset operations.
  // For outputs where bit i=1 and bit j=0, swap the bits.
  const MaskLibrary &mask_library = MaskLibrary::GetInstance(n_);
  CHECK_LT(i, j);
  // Find all outputs that need swapping (bit i=1, bit j=0)
  BitSet active = bitset_ & mask_library.Mask10(i, j);
  // Calculate the bit distance to swap: move bit from position i to position j
  OutputType delta = (OutputType(1) << j) - (OutputType(1) << i);
  // Remove the active outputs from their current positions and add them
  // at their swapped positions (shifted by delta)
  bitset_ = (bitset_ & ~active) | (active << delta);
}

std::vector<OutputType> OutputBitset::ToSparse() const {
  std::vector<OutputType> outputs;
  for (int i = 0; i < (OutputType(1) << n_); i++) {
    if (bitset_[i]) {
      outputs.push_back(i);
    }
  }
  return outputs;
}