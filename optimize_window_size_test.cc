#include "optimize_window_size.h"

#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "output_type.h"

void TestOptimizeWindowSize(int n, const std::vector<OutputType> &outputs,
                            bool symmetric, std::mt19937 *gen) {
  if (symmetric) {
    CHECK(IsSymmetric(n, outputs));
  }
  auto [optimized_outputs, perm] =
      OptimizeWindowSize(n, outputs, gen, symmetric);

  // Check that permutation is valid (contains all indices 0 to n-1)
  EXPECT_EQ(perm.size(), n);
  std::vector<int> sorted_perm = perm;
  std::sort(sorted_perm.begin(), sorted_perm.end());
  for (int i = 0; i < n; i++) {
    EXPECT_EQ(sorted_perm[i], i);
  }

  // Check that outputs are sorted
  EXPECT_TRUE(
      std::is_sorted(optimized_outputs.begin(), optimized_outputs.end()));

  // Check that permutation correctly transforms input to output
  std::vector<OutputType> permuted_outputs = PermuteChannels(outputs, perm);
  std::sort(permuted_outputs.begin(), permuted_outputs.end());
  EXPECT_EQ(optimized_outputs, permuted_outputs);

  if (symmetric) {
    CHECK(IsSymmetric(n, optimized_outputs));
  }

  // Calculate initial window size
  int initial_sum_window_size = 0;
  WindowSizeStats(n, outputs, &initial_sum_window_size, nullptr, nullptr);
  // Calculate optimized window size
  int optimized_sum_window_size = 0;
  WindowSizeStats(n, optimized_outputs, &optimized_sum_window_size, nullptr,
                  nullptr);

  // The optimized version should have equal or better (lower) window size
  EXPECT_LE(optimized_sum_window_size, initial_sum_window_size);
}

TEST(IsomorphismTest, OptimizeWindowSize) {
  std::mt19937 gen(42);
  {
    int n = 2;
    std::vector<OutputType> outputs = {0b00, 0b11};
    TestOptimizeWindowSize(n, outputs, false, &gen);
    TestOptimizeWindowSize(n, outputs, true, &gen);
  }
  {
    int n = 2;
    std::vector<OutputType> outputs = {0b01};
    TestOptimizeWindowSize(n, outputs, false, &gen);
    TestOptimizeWindowSize(n, outputs, true, &gen);
  }
  {
    int n = 2;
    std::vector<OutputType> outputs = {0b00, 0b10, 0b11};
    TestOptimizeWindowSize(n, outputs, false, &gen);
    TestOptimizeWindowSize(n, outputs, true, &gen);
  }
  {
    int n = 4;
    std::vector<OutputType> outputs = {0b0010, 0b0100, 0b1011, 0b1101};
    TestOptimizeWindowSize(n, outputs, false, &gen);
    TestOptimizeWindowSize(n, outputs, true, &gen);
  }
  {
    int n = 4;
    std::vector<OutputType> outputs = {0b0010, 0b0011, 0b0100,
                                       0b0101, 0b1011, 0b1101};
    TestOptimizeWindowSize(n, outputs, false, &gen);
    TestOptimizeWindowSize(n, outputs, true, &gen);
  }
}
