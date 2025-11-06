#include "output_type.h"

#include "glog/logging.h"

#include <algorithm>

std::string ToBinaryString(int n, OutputType x) {
  std::string s;
  s.reserve(n);
  for (int i = 0; i < n; i++) {
    s += ((x >> i) & OutputType(1)) ? '1' : '0';
  }
  return s;
}

void WindowSizeStats(int n, const std::vector<OutputType> &outputs,
                     int *p_sum_window_size, int *p_sum_sqr_window_size,
                     int *p_max_window_size) {
  int sum_window_size = 0;
  int sum_sqr_window_size = 0;
  int max_window_size = 0;
  for (OutputType x : outputs) {
    // Count leading zeros (already sorted low values)
    int num_leading_0s = 0;
    for (int i = 0; i < n; i++) {
      if (!(x & (OutputType(1) << i))) {
        num_leading_0s++;
      } else {
        break;
      }
    }
    // Count trailing ones (already sorted high values)
    int num_trailing_1s = 0;
    for (int i = n - 1; i >= 0; i--) {
      if (x & (OutputType(1) << i)) {
        num_trailing_1s++;
      } else {
        break;
      }
    }
    // Window size is the middle unsorted region
    int window_size = n - num_leading_0s - num_trailing_1s;
    sum_window_size += window_size;
    sum_sqr_window_size += window_size * window_size;
    if (window_size > max_window_size) {
      max_window_size = window_size;
    }
  }
  // Write results to output parameters if provided
  if (p_sum_window_size) {
    *p_sum_window_size = sum_window_size;
  }
  if (p_sum_sqr_window_size) {
    *p_sum_sqr_window_size = sum_sqr_window_size;
  }
  if (p_max_window_size) {
    *p_max_window_size = max_window_size;
  }
}

std::vector<OutputType> PermuteChannels(const std::vector<OutputType> &set,
                                        std::vector<int> perm) {
  int n = perm.size();
  std::vector<OutputType> outputs_perm;
  for (OutputType x : set) {
    OutputType x_perm = 0;
    for (int i = 0; i < n; i++) {
      x_perm |= ((x >> i) & OutputType(1)) << perm[i];
    }
    outputs_perm.push_back(x_perm);
  }
  std::sort(outputs_perm.begin(), outputs_perm.end());
  return outputs_perm;
}

OutputType ReflectAndInvert(int n, OutputType x) {
  CHECK_LT(n, sizeof(OutputType) * 8);
  OutputType x_reflect = 0;
  for (int i = 0; i < n; i++) {
    x_reflect |= ((x >> i) & OutputType(1)) << (n - 1 - i);
  }
  return x_reflect ^ ((OutputType(1) << n) - 1);
}

// Check if a set is symmetric under the permutation (0,n-1), (1,n-2), ...
bool IsSymmetric(int n, const std::vector<OutputType> &set) {
  CHECK(std::is_sorted(set.begin(), set.end()));
  for (OutputType x : set) {
    OutputType rev_inv = ReflectAndInvert(n, x);
    if (!std::binary_search(set.begin(), set.end(), rev_inv)) {
      return false;
    }
  }
  return true;
}

bool HasInverse(const std::vector<OutputType> &outputs, int i, int j) {
  CHECK_LT(i, j);
  for (OutputType x : outputs) {
    auto i_bit = (x >> i) & OutputType(1);
    auto j_bit = (x >> j) & OutputType(1);
    if (i_bit > j_bit) {
      return true;
    }
  }
  return false;
}

std::vector<OutputType> AddComparator(const std::vector<OutputType> &outputs,
                                      int i, int j) {
  // Apply comparator (i, j) to all outputs: if bit i > bit j, swap them.
  // This simulates the effect of adding a comparator to the network.
  std::vector<OutputType> new_outputs;
  new_outputs.reserve(outputs.size());
  for (OutputType x : outputs) {
    auto i_bit = (x >> i) & OutputType(1);
    auto j_bit = (x >> j) & OutputType(1);
    // Swap bits if i > j (i.e., if i has 1 and j has 0)
    if (i_bit > j_bit) {
      // XOR with mask to flip both bits
      x ^= (OutputType(1) << i) ^ (OutputType(1) << j);
    }
    new_outputs.push_back(x);
  }
  // Sort and deduplicate
  std::sort(new_outputs.begin(), new_outputs.end());
  new_outputs.erase(std::unique(new_outputs.begin(), new_outputs.end()),
                    new_outputs.end());
  return new_outputs;
}
