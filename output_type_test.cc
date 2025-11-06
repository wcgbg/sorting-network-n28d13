#include "output_type.h"

#include <algorithm>

#include "gtest/gtest.h"

TEST(ReflectAndInvert, SingleBit) {
  // Test with n=1
  EXPECT_EQ(ReflectAndInvert(1, 0b0), 0b1); // 0 -> 1 -> 1
  EXPECT_EQ(ReflectAndInvert(1, 0b1), 0b0); // 1 -> 0 -> 0
}

TEST(ReflectAndInvert, TwoBits) {
  // Test with n=2
  EXPECT_EQ(ReflectAndInvert(2, 0b00), 0b11); // 00 -> 11 -> 11
  EXPECT_EQ(ReflectAndInvert(2, 0b01), 0b01); // 01 -> 10 -> 01
  EXPECT_EQ(ReflectAndInvert(2, 0b10), 0b10); // 10 -> 01 -> 10
  EXPECT_EQ(ReflectAndInvert(2, 0b11), 0b00); // 11 -> 00 -> 00
}

TEST(ReflectAndInvert, ThreeBits) {
  // Test with n=3
  EXPECT_EQ(ReflectAndInvert(3, 0b000), 0b111); // 000 -> 111 -> 111
  EXPECT_EQ(ReflectAndInvert(3, 0b001), 0b011); // 001 -> 110 -> 011
  EXPECT_EQ(ReflectAndInvert(3, 0b010), 0b101); // 010 -> 101 -> 101
  EXPECT_EQ(ReflectAndInvert(3, 0b011), 0b001); // 011 -> 100 -> 001
  EXPECT_EQ(ReflectAndInvert(3, 0b100), 0b110); // 100 -> 011 -> 110
  EXPECT_EQ(ReflectAndInvert(3, 0b101), 0b010); // 101 -> 010 -> 010
  EXPECT_EQ(ReflectAndInvert(3, 0b110), 0b100); // 110 -> 001 -> 100
  EXPECT_EQ(ReflectAndInvert(3, 0b111), 0b000); // 111 -> 000 -> 000
}

TEST(ReflectAndInvert, Involution) {
  // ReflectAndInvert should be its own inverse:
  // ReflectAndInvert(ReflectAndInvert(x)) == x
  for (int n = 1; n <= 8; n++) {
    for (OutputType x = 0; x < (OutputType(1) << n); x++) {
      EXPECT_EQ(ReflectAndInvert(n, ReflectAndInvert(n, x)), x)
          << "ReflectAndInvert is not involutive for n=" << n << ", x=" << x;
    }
  }
}

TEST(ToBinaryString, Basic) {
  EXPECT_EQ(ToBinaryString(1, 0b0), "0");
  EXPECT_EQ(ToBinaryString(1, 0b1), "1");
  EXPECT_EQ(ToBinaryString(2, 0b00), "00");
  EXPECT_EQ(ToBinaryString(2, 0b01), "10");
  EXPECT_EQ(ToBinaryString(2, 0b10), "01");
  EXPECT_EQ(ToBinaryString(2, 0b11), "11");
  EXPECT_EQ(ToBinaryString(3, 0b101), "101");
  EXPECT_EQ(ToBinaryString(4, 0b1010), "0101");
}

TEST(WindowSizeStats, Basic) {
  int sum_window_size, sum_sqr_window_size, max_window_size;

  // Test with n=3
  std::vector<OutputType> outputs = {0b000, 0b100, 0b110, 0b111};
  WindowSizeStats(3, outputs, &sum_window_size, &sum_sqr_window_size,
                  &max_window_size);
  // 000: leading 0s=3, trailing 1s=0, window=0
  // 100: leading 0s=2, trailing 1s=1, window=0
  // 110: leading 0s=1, trailing 1s=2, window=0
  // 111: leading 0s=0, trailing 1s=3, window=0
  EXPECT_EQ(sum_window_size, 0);
  EXPECT_EQ(sum_sqr_window_size, 0);
  EXPECT_EQ(max_window_size, 0);

  // Test with some values that have non-zero windows
  outputs = {0b010, 0b001};
  WindowSizeStats(3, outputs, &sum_window_size, &sum_sqr_window_size,
                  &max_window_size);
  // 010: leading 0s=1, trailing 1s=0, window=2
  // 001: leading 0s=0, trailing 1s=0, window=3
  EXPECT_EQ(sum_window_size, 5);
  EXPECT_EQ(sum_sqr_window_size, 13); // 2*2 + 3*3
  EXPECT_EQ(max_window_size, 3);
}

TEST(WindowSizeStats, NullPointers) {
  // Test that null pointers don't cause crashes
  std::vector<OutputType> outputs = {0b010, 0b101};
  WindowSizeStats(3, outputs, nullptr, nullptr, nullptr);
  WindowSizeStats(3, outputs, nullptr, nullptr, nullptr);
}

TEST(PermuteChannels, Basic) {
  std::vector<OutputType> set = {0b001, 0b111};
  std::vector<int> perm = {1, 2, 0};
  std::vector<OutputType> result = PermuteChannels(set, perm);
  std::sort(result.begin(), result.end());
  std::vector<OutputType> expected = {0b010, 0b111};
  EXPECT_EQ(result, expected);
}

TEST(IsSymmetric, EmptySet) {
  std::vector<OutputType> set = {};
  EXPECT_TRUE(IsSymmetric(3, set));
}

TEST(IsSymmetricAndAddComparator, N3) {
  /*
  [(0,2)]
  [(0,1)]
  [(1,2)]
  */
  int n = 3;
  std::vector<OutputType> set;
  for (OutputType x = 0; x < (OutputType(1) << n); x++) {
    set.push_back(x);
  }
  EXPECT_TRUE(IsSymmetric(n, set));
  set = AddComparator(set, 0, 2);
  EXPECT_TRUE(IsSymmetric(n, set));
  set = AddComparator(set, 0, 1);
  EXPECT_FALSE(IsSymmetric(n, set));
  set = AddComparator(set, 1, 2);
  EXPECT_TRUE(IsSymmetric(n, set));
  // It is a sorting network.
  EXPECT_EQ(set.size(), n + 1);
}

TEST(IsSymmetricAndAddComparator, N4) {
  /*
  [(0,2),(1,3)]
  [(0,1),(2,3)]
  [(1,2)]
  */
  int n = 4;
  std::vector<OutputType> set;
  for (OutputType x = 0; x < (OutputType(1) << n); x++) {
    set.push_back(x);
  }
  EXPECT_TRUE(IsSymmetric(n, set));
  set = AddComparator(set, 0, 2);
  EXPECT_FALSE(IsSymmetric(n, set));
  set = AddComparator(set, 1, 3);
  EXPECT_TRUE(IsSymmetric(n, set));
  set = AddComparator(set, 0, 1);
  EXPECT_FALSE(IsSymmetric(n, set));
  set = AddComparator(set, 2, 3);
  EXPECT_TRUE(IsSymmetric(n, set));
  set = AddComparator(set, 1, 2);
  EXPECT_TRUE(IsSymmetric(n, set));
  // It is a sorting network.
  EXPECT_EQ(set.size(), n + 1);
}

TEST(HasInverse, Basic) {
  // HasInverse checks if there's any output where bit i > bit j (i.e., i=1,
  // j=0)
  std::vector<OutputType> outputs = {0b001};
  EXPECT_TRUE(HasInverse(outputs, 0, 1));

  // Test with a set that doesn't have any inversions
  outputs = {0b000, 0b110, 0b010, 0b011};
  EXPECT_FALSE(HasInverse(outputs, 0, 1));
}
