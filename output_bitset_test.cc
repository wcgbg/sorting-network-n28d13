#include "output_bitset.h"

#include <algorithm>
#include <vector>
#include <array>
#include <string>

#include "gtest/gtest.h"

TEST(OutputBitsetTest, SortingNetworkN4) {
  /*
  [(0,2),(1,3)]
  [(0,1),(2,3)]
  [(1,2)]
  */
  int n = 4;
  OutputBitset output_bitset(n);

  // [(0,2),(1,3)]
  output_bitset.AddComparator(0, 2);
  output_bitset.AddComparator(1, 3);

  // [(0,1),(2,3)]
  output_bitset.AddComparator(0, 1);
  output_bitset.AddComparator(2, 3);

  // [(1,2)]
  output_bitset.AddComparator(1, 2);

  // Get the outputs
  std::vector<OutputType> outputs = output_bitset.ToSparse();
  std::sort(outputs.begin(), outputs.end());

  // For a sorting network with n=4, we should have exactly n+1=5 outputs
  EXPECT_EQ(outputs.size(), n + 1);

  std::vector<OutputType> expected_outputs = {0b0000, 0b1000, 0b1100, 0b1110,
                                              0b1111};
  EXPECT_EQ(outputs, expected_outputs);
}

TEST(OutputBitsetTest, SortingNetworkN5) {
  /*
  [(0,3),(1,4)]
  [(0,2),(1,3)]
  [(0,1),(2,4)]
  [(1,2),(3,4)]
  [(2,3)]
  */
  int n = 5;
  OutputBitset output_bitset(n);

  // [(0,3),(1,4)]
  output_bitset.AddComparator(0, 3);
  output_bitset.AddComparator(1, 4);

  // [(0,2),(1,3)]
  output_bitset.AddComparator(0, 2);
  output_bitset.AddComparator(1, 3);

  // [(0,1),(2,4)]
  output_bitset.AddComparator(0, 1);
  output_bitset.AddComparator(2, 4);

  // [(1,2),(3,4)]
  output_bitset.AddComparator(1, 2);
  output_bitset.AddComparator(3, 4);

  // [(2,3)]
  output_bitset.AddComparator(2, 3);

  // Get the outputs
  std::vector<OutputType> outputs = output_bitset.ToSparse();
  std::sort(outputs.begin(), outputs.end());

  // For a sorting network with n=5, we should have exactly n+1=6 outputs
  EXPECT_EQ(outputs.size(), n + 1);

  std::vector<OutputType> expected_outputs = {0b00000, 0b10000, 0b11000,
                                              0b11100, 0b11110, 0b11111};
  EXPECT_EQ(outputs, expected_outputs);
}

TEST(OutputBitsetTest, SortingNetworkN6) {
  /*
  [(0,5),(1,3),(2,4)]
  [(1,2),(3,4)]
  [(0,3),(2,5)]
  [(0,1),(2,3),(4,5)]
  [(1,2),(3,4)]
  */
  std::vector<std::array<int, 2>> comparators = {
      {0, 5}, {1, 3}, {2, 4}, {1, 2}, {3, 4}, {0, 3},
      {2, 5}, {0, 1}, {2, 3}, {4, 5}, {1, 2}, {3, 4}};

  int n = 6;
  OutputBitset output_bitset(n);
  std::vector<OutputType> sparse_outputs;
  for (OutputType x = 0; x < (OutputType(1) << n); x++) {
    sparse_outputs.push_back(x);
  }
  for (const auto &comparator : comparators) {
    output_bitset.AddComparator(comparator[0], comparator[1]);
    sparse_outputs =
        AddComparator(sparse_outputs, comparator[0], comparator[1]);
    EXPECT_EQ(output_bitset.ToSparse(), sparse_outputs);
  }
}
