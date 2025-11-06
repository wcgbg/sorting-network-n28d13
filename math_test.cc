#include "math_utils.h"

#include <random>
#include <set>
#include <string>

#include "gtest/gtest.h"

// RandomPermutation Tests

TEST(RandomPermutationTest, Basic) {
  std::mt19937 gen;
  std::vector<int> perm = RandomPermutation(5, &gen);

  EXPECT_EQ(perm.size(), 5);

  // Check that all elements from 0 to 4 are present
  std::set<int> elements(perm.begin(), perm.end());
  EXPECT_EQ(elements.size(), 5);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(elements.count(i), 1);
  }
}

TEST(RandomPermutationTest, SingleElement) {
  std::mt19937 gen;
  std::vector<int> perm = RandomPermutation(1, &gen);

  EXPECT_EQ(perm.size(), 1);
  EXPECT_EQ(perm[0], 0);
}

TEST(RandomPermutationTest, LargeSize) {
  std::mt19937 gen;
  std::vector<int> perm = RandomPermutation(100, &gen);

  EXPECT_EQ(perm.size(), 100);

  // Check that all elements from 0 to 99 are present
  std::set<int> elements(perm.begin(), perm.end());
  EXPECT_EQ(elements.size(), 100);
  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(elements.count(i), 1);
  }
}

// InversePermutation Tests

TEST(InversePermutationTest, Identity) {
  std::vector<int> identity = {0, 1, 2, 3, 4};
  std::vector<int> inv = InversePermutation(identity);

  EXPECT_EQ(inv, identity);
}

TEST(InversePermutationTest, Basic) {
  std::vector<int> perm = {2, 0, 4, 1, 3};
  std::vector<int> inv = InversePermutation(perm);

  // inv[perm[i]] should equal i
  for (int i = 0; i < perm.size(); i++) {
    EXPECT_EQ(inv[perm[i]], i);
  }

  // perm[inv[i]] should equal i
  for (int i = 0; i < inv.size(); i++) {
    EXPECT_EQ(perm[inv[i]], i);
  }
}

TEST(InversePermutationTest, RoundTrip) {
  std::vector<int> perm = {3, 1, 4, 0, 2};
  std::vector<int> inv = InversePermutation(perm);
  std::vector<int> inv_inv = InversePermutation(inv);

  // Inverse of inverse should be original
  EXPECT_EQ(inv_inv, perm);
}

TEST(InversePermutationTest, SingleElement) {
  std::vector<int> perm = {0};
  std::vector<int> inv = InversePermutation(perm);

  EXPECT_EQ(inv.size(), 1);
  EXPECT_EQ(inv[0], 0);
}

TEST(InversePermutationTest, TwoElements) {
  std::vector<int> perm = {1, 0};
  std::vector<int> inv = InversePermutation(perm);

  EXPECT_EQ(inv.size(), 2);
  EXPECT_EQ(inv[0], 1);
  EXPECT_EQ(inv[1], 0);
}

TEST(InversePermutationTest, LargePermutation) {
  std::vector<int> perm = {9, 2, 5, 0, 7, 1, 8, 4, 6, 3};
  std::vector<int> inv = InversePermutation(perm);

  EXPECT_EQ(inv.size(), 10);

  // Verify inverse property
  for (int i = 0; i < perm.size(); i++) {
    EXPECT_EQ(inv[perm[i]], i);
    EXPECT_EQ(perm[inv[i]], i);
  }
}

TEST(InversePermutationTest, WithRandomPermutation) {
  std::mt19937 gen;

  for (int n = 1; n <= 20; n++) {
    std::vector<int> perm = RandomPermutation(n, &gen);
    std::vector<int> inv = InversePermutation(perm);

    // Verify inverse property
    for (int i = 0; i < n; i++) {
      EXPECT_EQ(inv[perm[i]], i) << "Failed for n=" << n << ", i=" << i;
      EXPECT_EQ(perm[inv[i]], i) << "Failed for n=" << n << ", i=" << i;
    }

    // Verify round trip
    std::vector<int> inv_inv = InversePermutation(inv);
    EXPECT_EQ(inv_inv, perm) << "Failed round trip for n=" << n;
  }
}
