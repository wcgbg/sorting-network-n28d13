#include "optimize_window_size.h"

#include <algorithm>
#include <numeric>
#include <vector>

#include "glog/logging.h"

#include "isomorphism.h"
#include "math_utils.h"
#include "output_type.h"

std::pair<std::vector<OutputType>, std::vector<int>>
OptimizeWindowSize(int n, const std::vector<OutputType> &outputs,
                   std::mt19937 *gen, bool symmetric) {
  CHECK(!outputs.empty());

  // Greedy local search algorithm to minimize window size by permuting
  // channels. The algorithm tries swapping pairs of channels and keeps
  // improvements.
  if (symmetric) {
    CHECK_EQ(n % 2, 0);
    CHECK(IsSymmetric(n, outputs));
  }
  // Start with a permutation that sorts channels by weight (number of 1s)
  auto [outputs2, perm2] = SortByWeight(n, outputs, gen, symmetric);
  CHECK(outputs2 == PermuteChannels(outputs, perm2));
  if (symmetric) {
    CHECK(IsSymmetric(n, outputs2));
  }

  // Compute initial window size sum
  int sum_window_size = 0;
  WindowSizeStats(n, outputs2, &sum_window_size, nullptr, nullptr);

  // Greedy improvement: try swapping pairs of channels
  while (true) {
    bool found_better = false;
    // Try pairs in random order to avoid getting stuck in local minima
    std::vector<int> perm_i = RandomPermutation(n, gen);
    for (int i : perm_i) {
      std::vector<int> perm_j = RandomPermutation(n, gen);
      for (int j : perm_j) {
        if (i >= j) {
          continue;
        }
        // Create a permutation that swaps channels i and j
        std::vector<int> swap_i_j_perm(n);
        std::iota(swap_i_j_perm.begin(), swap_i_j_perm.end(), 0);
        std::swap(swap_i_j_perm[i], swap_i_j_perm[j]);
        // In symmetric mode, also swap the mirror positions
        if (symmetric && i + j != n - 1) {
          std::swap(swap_i_j_perm[n - 1 - i], swap_i_j_perm[n - 1 - j]);
        }
        // Apply the swap and check if it improves the window size
        std::vector<OutputType> outputs3 =
            PermuteChannels(outputs2, swap_i_j_perm);
        int sum_window_size3 = 0;
        WindowSizeStats(n, outputs3, &sum_window_size3, nullptr, nullptr);
        if (sum_window_size3 < sum_window_size) {
          // Improvement found: update the permutation and outputs
          sum_window_size = sum_window_size3;
          outputs2 = std::move(outputs3);
          // Update the overall permutation to include this swap
          std::vector<int> inv_perm3 = InversePermutation(perm2);
          std::swap(inv_perm3[i], inv_perm3[j]);
          if (symmetric && i + j != n - 1) {
            std::swap(inv_perm3[n - 1 - i], inv_perm3[n - 1 - j]);
          }
          perm2 = InversePermutation(inv_perm3);
          auto expected_outputs2 = PermuteChannels(outputs, perm2);
          DCHECK(outputs2 == expected_outputs2);
          found_better = true;
          break;
        }
      }
      if (found_better) {
        break;
      }
    }
    // If no improvement found, we've reached a local optimum
    if (!found_better) {
      break;
    }
  }
  // Sort outputs for consistency
  std::sort(outputs2.begin(), outputs2.end());
  if (symmetric) {
    CHECK(IsSymmetric(n, outputs2));
  }
  return {outputs2, perm2};
}
