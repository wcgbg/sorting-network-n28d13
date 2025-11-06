#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Represents a binary output of a sorting network.
// For n channels, the i-th bit represents the value of channel i (0 or 1).
// Change it to uint64_t if n >= 32.
using OutputType = uint32_t;

// Converts an OutputType to a binary string representation of length n.
std::string ToBinaryString(int n, OutputType x);

// Computes window size statistics for a set of outputs.
// The window size for an output is the number of channels between the
// leading zeros and trailing ones.
void WindowSizeStats(int n, const std::vector<OutputType> &outputs,
                     int *p_sum_window_size, int *p_sum_sqr_window_size,
                     int *p_max_window_size);

// Permutes the channels in each output according to the given permutation.
// The permutation perm[i] = j means channel i is moved to position j.
std::vector<OutputType> PermuteChannels(const std::vector<OutputType> &set,
                                        std::vector<int> perm);

// Reflects and inverts an output: reflects the bits (0->n-1, 1->n-2, ...)
// and inverts all bits (0->1, 1->0).
OutputType ReflectAndInvert(int n, OutputType x);

// Checks if a set of outputs is symmetric under channel reflection and
// inversion. A set is symmetric if for every output x, ReflectAndInvert(n, x)
// is also in the set.
bool IsSymmetric(int n, const std::vector<OutputType> &set);

// Checks if there exists an output where channel i has value 1 and channel j
// has value 0 for i < j.
bool HasInverse(const std::vector<OutputType> &outputs, int i, int j);

// Applies a comparator (i, j) to outputs.
// For each output, if bit i > bit j, swaps the bits.
// Returns the set of outputs after applying the comparator (deduplicated).
std::vector<OutputType> AddComparator(const std::vector<OutputType> &outputs,
                                      int i, int j);
