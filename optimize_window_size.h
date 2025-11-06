#pragma once

#include <random>
#include <utility>
#include <vector>

#include "output_type.h"

// Optimizes the channel ordering to minimize the sum of window sizes.
// The window size for an output is the number of channels between the
// leading zeros and trailing ones.
// Uses a greedy local search algorithm that tries swapping pairs of channels.
// Args:
//   n: Number of channels
//   outputs: Set of output states to optimize
//   gen: Random number generator for tie-breaking
//   symmetric: If true, ensures the result remains symmetric under reflection
// Returns: A pair of (optimized outputs, permutation) where the permutation
//          maps from original channel positions to optimized positions
std::pair<std::vector<OutputType>, std::vector<int>>
OptimizeWindowSize(int n, const std::vector<OutputType> &outputs,
                   std::mt19937 *gen, bool symmetric = false);
