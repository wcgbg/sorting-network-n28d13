#pragma once

#include <random>
#include <utility>
#include <vector>

#include "output_type.h"

// set_a[i] and set_b[i] are n-bit integers.
// It returns true if set_a is isomorphic to a subset of set_b,
// i.e. there exists a permutation perm in S_n such that set(perm(set_a))
// \subseteq set(set_b), where perm(set_a) is [perm(a) for a in set_a], where
// perm(a) permutes the n bits of a.
bool IsIsomorphicToSubset(int n, const std::vector<OutputType> &set_a,
                          const std::vector<OutputType> &set_b, bool symmetric,
                          std::mt19937 *gen);

// It returns false if set_a is not isomorphic to a subset of set_b.
// It returns true if unknown.
bool IsIsomorphicToSubsetNegativePrecheck(int n,
                                          const std::vector<OutputType> &set_a,
                                          const std::vector<OutputType> &set_b);

// It returns true if set_a is isomorphic to a subset of set_b.
// It returns false if unknown.
bool IsIsomorphicToSubsetPositivePrecheck(int n,
                                          const std::vector<OutputType> &set_a,
                                          const std::vector<OutputType> &set_b,
                                          int num_tests, std::mt19937 *gen);

// Returns the sorted set and the permutation that achieves the sort.
std::pair<std::vector<OutputType>, std::vector<int>>
SortByWeight(int n, const std::vector<OutputType> &set,
             std::mt19937 *gen = nullptr, bool symmetric = false);

// It returns a vector of bools, where the i-th element is true if the i-th
// output is redundant.
std::vector<bool> FindRedundantOutputs(
    int n, std::vector<std::vector<OutputType>> outputs_collection, bool fast,
    bool symmetric, std::mt19937 *gen, bool verbose = false);

namespace internal {
// Slow and simple algorithm that checks all permutations of set_a.
bool IsIsomorphicToSubsetSlow(int n, const std::vector<OutputType> &set_a,
                              const std::vector<OutputType> &set_b);
// Backtracking algorithm with pruning.
bool IsIsomorphicToSubsetBacktracking(int n,
                                      const std::vector<OutputType> &set_a,
                                      const std::vector<OutputType> &set_b,
                                      bool symmetric, std::mt19937 *gen);
} // namespace internal
