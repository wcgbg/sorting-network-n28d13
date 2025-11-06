#pragma once

#include <random>
#include <vector>

// Generates a random permutation of [0, 1, ..., n-1].
std::vector<int> RandomPermutation(int n, std::mt19937 *gen);

// Computes the inverse permutation.
std::vector<int> InversePermutation(const std::vector<int> &perm);
