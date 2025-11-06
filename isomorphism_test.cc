#include "isomorphism.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "output_type.h"

namespace {

// Generate a random set of n-bit integers
std::vector<OutputType> GenerateRandomSet(int n, int size, std::mt19937 *gen) {
  CHECK_LT(n, sizeof(OutputType) * 8);
  std::uniform_int_distribution<OutputType> dist(0, (OutputType(1) << n) - 1);
  std::vector<OutputType> result;
  result.reserve(size);

  for (int i = 0; i < size; i++) {
    result.push_back(dist(*gen));
  }

  // Remove duplicates and sort
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());

  return result;
}

// Generate a set that is guaranteed to be isomorphic to a subset of another set
std::vector<OutputType>
GenerateIsomorphicSubset(int n, const std::vector<OutputType> &set_b,
                         int subset_size, std::mt19937 *gen) {
  CHECK_LE(subset_size, set_b.size());

  if (set_b.empty() || subset_size == 0) {
    return {};
  }

  std::uniform_int_distribution<int> perm_dist(0, n - 1);
  std::uniform_int_distribution<int> elem_dist(0, set_b.size() - 1);

  // Generate a random permutation
  std::vector<int> perm(n);
  std::iota(perm.begin(), perm.end(), 0);
  std::shuffle(perm.begin(), perm.end(), *gen);

  std::vector<OutputType> set_b_shuffled = set_b;
  std::shuffle(set_b_shuffled.begin(), set_b_shuffled.end(), *gen);

  std::vector<OutputType> result;
  result.reserve(subset_size);

  for (int i = 0; i < subset_size; i++) {
    // Pick a random element from set_b
    OutputType elem = set_b_shuffled[i];

    // Apply the inverse permutation
    OutputType permuted = 0;
    for (int bit = 0; bit < n; bit++) {
      permuted |= ((elem >> perm[bit]) & OutputType(1)) << bit;
    }

    result.push_back(permuted);
  }

  std::sort(result.begin(), result.end());
  return result;
}

// Test that core implementations agree on the result
void TestCoreImplementationAgreement(int n,
                                     const std::vector<OutputType> &set_a,
                                     const std::vector<OutputType> &set_b,
                                     std::mt19937 *gen) {
  // Run core implementations
  bool neg_precheck_result =
      IsIsomorphicToSubsetNegativePrecheck(n, set_a, set_b);
  bool pos_precheck_result =
      IsIsomorphicToSubsetPositivePrecheck(n, set_a, set_b, 100, gen);
  bool slow_result = internal::IsIsomorphicToSubsetSlow(n, set_a, set_b);
  bool backtracking_result =
      internal::IsIsomorphicToSubsetBacktracking(n, set_a, set_b, false, gen);
  // TODO: add symmetric case

  // Check that slow and backtracking results agree
  EXPECT_EQ(slow_result, backtracking_result)
      << "Slow and backtracking results disagree for n=" << n
      << ", set_a size=" << set_a.size() << ", set_b size=" << set_b.size();

  // Check that precheck returns true when slow function returns true
  if (slow_result) {
    EXPECT_TRUE(neg_precheck_result)
        << "Precheck returned false but slow function returned true for n=" << n
        << ", set_a size=" << set_a.size() << ", set_b size=" << set_b.size();
  } else {
    EXPECT_FALSE(pos_precheck_result)
        << "Precheck returned true but slow function returned false for n=" << n
        << ", set_a size=" << set_a.size() << ", set_b size=" << set_b.size();
  }
}

TEST(IsomorphismTest, BasicCases) {
  // Test case 1: Empty sets
  std::vector<OutputType> empty;
  EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(3, empty, empty));
  EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(3, empty, empty));

  // Test case 2: set_a is empty, set_b is not
  std::vector<OutputType> set_b = {0b001, 0b010, 0b100};
  EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(3, empty, set_b));
  EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(3, empty, set_b));

  // Test case 3: set_a is not empty, set_b is empty
  std::vector<OutputType> set_a = {0b001};
  EXPECT_FALSE(internal::IsIsomorphicToSubsetSlow(3, set_a, empty));
  EXPECT_FALSE(IsIsomorphicToSubsetNegativePrecheck(3, set_a, empty));

  // Test case 4: Identical sets
  EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(3, set_a, set_a));
  EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(3, set_a, set_a));

  // Test case 5: set_a is a subset of set_b
  std::vector<OutputType> set_a_subset = {0b001, 0b010};
  EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(3, set_a_subset, set_b));
  EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(3, set_a_subset, set_b));
}

TEST(IsomorphismTest, KnownIsomorphicCases) {
  // Test case: sets that are isomorphic via permutation
  std::vector<OutputType> set_a = {0b001, 0b010, 0b100}; // {1, 2, 4}
  std::vector<OutputType> set_b = {0b001, 0b010,
                                   0b100}; // {1, 2, 4} - same elements, sorted

  EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(3, set_a, set_b));
  EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(3, set_a, set_b));

  // Test case: sets that are isomorphic via bit permutation
  std::vector<OutputType> set_a2 = {0b001, 0b011}; // {1, 3}
  std::vector<OutputType> set_b2 = {0b010, 0b110}; // {2, 6} - bit 0 and 1

  EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(3, set_a2, set_b2));
  EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(3, set_a2, set_b2));
}

TEST(IsomorphismTest, KnownNonIsomorphicCases) {
  // Test case: different popcounts
  std::vector<OutputType> set_a = {0b001, 0b010}; // popcount 1
  std::vector<OutputType> set_b = {0b011, 0b101}; // popcount 2

  EXPECT_FALSE(internal::IsIsomorphicToSubsetSlow(3, set_a, set_b));
  EXPECT_FALSE(IsIsomorphicToSubsetNegativePrecheck(3, set_a, set_b));

  // Test case: set_a larger than set_b
  std::vector<OutputType> set_a_large = {0b001, 0b010, 0b100, 0b111};
  std::vector<OutputType> set_b_small = {0b001, 0b010};

  EXPECT_FALSE(internal::IsIsomorphicToSubsetSlow(3, set_a_large, set_b_small));
  EXPECT_FALSE(
      IsIsomorphicToSubsetNegativePrecheck(3, set_a_large, set_b_small));
}

TEST(IsomorphismTest, RandomSmallCases) {
  std::mt19937 gen(42); // Fixed seed for reproducibility

  for (int n = 3; n <= 6; n++) {
    for (int trial = 0; trial < 20; trial++) {
      // Generate random set_b
      int set_b_size =
          std::uniform_int_distribution<int>(1, std::min(8, 1 << n))(gen);
      std::vector<OutputType> set_b = GenerateRandomSet(n, set_b_size, &gen);

      // Generate random set_a
      int set_a_size = std::uniform_int_distribution<int>(0, set_b_size)(gen);
      std::vector<OutputType> set_a = GenerateRandomSet(n, set_a_size, &gen);

      TestCoreImplementationAgreement(n, set_a, set_b, &gen);
    }
  }
}

TEST(IsomorphismTest, RandomMediumCases) {
  std::mt19937 gen(123); // Fixed seed for reproducibility

  for (int n = 7; n <= 9; n++) {
    for (int trial = 0; trial < 5; trial++) {
      // Generate random set_b
      int set_b_size =
          std::uniform_int_distribution<int>(1, std::min(16, 1 << n))(gen);
      std::vector<OutputType> set_b = GenerateRandomSet(n, set_b_size, &gen);

      // Generate random set_a
      int set_a_size = std::uniform_int_distribution<int>(0, set_b_size)(gen);
      std::vector<OutputType> set_a = GenerateRandomSet(n, set_a_size, &gen);

      TestCoreImplementationAgreement(n, set_a, set_b, &gen);
    }
  }
}

TEST(IsomorphismTest, GuaranteedIsomorphicCases) {
  std::mt19937 gen(456); // Fixed seed for reproducibility

  for (int n = 3; n <= 8; n++) {
    for (int trial = 0; trial < 10; trial++) { // Reduced from 30
      // Generate random set_b
      int set_b_size =
          std::uniform_int_distribution<int>(4, std::min(12, 1 << n))(gen);
      std::vector<OutputType> set_b = GenerateRandomSet(n, set_b_size, &gen);

      // Generate set_a that is guaranteed to be isomorphic to a subset of
      int set_a_size = std::uniform_int_distribution<int>(1, set_b.size())(gen);
      std::vector<OutputType> set_a =
          GenerateIsomorphicSubset(n, set_b, set_a_size, &gen);
      TestCoreImplementationAgreement(n, set_a, set_b, &gen);

      // These should all return true
      EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(n, set_a, set_b))
          << "Guaranteed isomorphic case failed for n=" << n;
      EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(n, set_a, set_b))
          << "Precheck failed on guaranteed isomorphic case for n=" << n;
    }
  }
}

TEST(IsomorphismTest, PerformanceComparison) {
  std::mt19937 gen(789);
  int n = 9;

  // Generate a moderately sized test case
  std::vector<OutputType> set_b = GenerateRandomSet(n, 20, &gen);
  std::vector<OutputType> set_a = GenerateRandomSet(n, 15, &gen);

  // Time the slow function
  auto start = std::chrono::high_resolution_clock::now();
  bool slow_result = internal::IsIsomorphicToSubsetSlow(n, set_a, set_b);
  auto end = std::chrono::high_resolution_clock::now();
  auto slow_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Time the backtracking function
  start = std::chrono::high_resolution_clock::now();
  bool backtracking_result =
      internal::IsIsomorphicToSubsetBacktracking(n, set_a, set_b, false, &gen);
  end = std::chrono::high_resolution_clock::now();
  auto backtracking_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Results should agree (except for bipartite which has bugs)
  EXPECT_EQ(slow_result, backtracking_result);

  // Print performance info
  std::cout << "Performance comparison for n=" << n
            << ", set_a size=" << set_a.size()
            << ", set_b size=" << set_b.size() << ":\n";
  std::cout << "Slow function: " << slow_duration.count() << " μs\n";
  std::cout << "Backtracking: " << backtracking_duration.count() << " μs\n";
  std::cout << "Slow result: " << slow_result << "\n";
  std::cout << "Backtracking result: " << backtracking_result << "\n";
}

TEST(IsomorphismTest, EdgeCases) {
  // Test with maximum values
  std::vector<OutputType> set_a = {0b11111111}; // All bits set for n=8
  std::vector<OutputType> set_b = {0b00000000, 0b11111111}; // Sorted

  EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(8, set_a, set_b));
  EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(8, set_a, set_b));

  // Test with single elements
  std::vector<OutputType> single_a = {0b001};
  std::vector<OutputType> single_b = {0b010};

  EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(3, single_a, single_b));
  EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(3, single_a, single_b));

  // Test with all possible values for small n
  std::vector<OutputType> all_values;
  for (int i = 0; i < (1 << 3); i++) {
    all_values.push_back(i);
  }

  EXPECT_TRUE(internal::IsIsomorphicToSubsetSlow(3, all_values, all_values));
  EXPECT_TRUE(IsIsomorphicToSubsetNegativePrecheck(3, all_values, all_values));
}

TEST(IsomorphismTest, SortByWeight) {
  {
    int n = 4;
    std::vector<OutputType> set = {0b0100, 0b0101, 0b1101};
    auto [set_sorted, perm] = SortByWeight(n, set);
    EXPECT_EQ(set_sorted, (std::vector<OutputType>{0b1000, 0b1100, 0b1110}));
    EXPECT_EQ(perm, (std::vector<int>{2, 0, 3, 1}));
    EXPECT_TRUE(set_sorted == PermuteChannels(set, perm));
  }
  {
    int n = 4;
    std::vector<OutputType> set = {0b1000, 0b1001, 0b1101};
    auto [set_sorted, perm] = SortByWeight(n, set);
    EXPECT_EQ(set_sorted, (std::vector<OutputType>{0b1000, 0b1100, 0b1110}));
    EXPECT_EQ(perm, (std::vector<int>{2, 0, 1, 3}));
    EXPECT_TRUE(set_sorted == PermuteChannels(set, perm));
  }
  {
    std::mt19937 gen;
    for (int n = 3; n <= 8; n++) {
      for (int trial = 0; trial < 1000; trial++) {
        std::vector<OutputType> set =
            GenerateRandomSet(n, OutputType(1) << (n - 1), &gen);
        auto [set_sorted, perm] = SortByWeight(n, set);
        ASSERT_TRUE(set_sorted == PermuteChannels(set, perm))
            << "SortByWeight failed for n=" << n << ", trial=" << trial;
      }
    }
  }
}

} // namespace
