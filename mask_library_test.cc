#include "mask_library.h"

#include <chrono>
#include <iostream>
#include <string>

#include "gtest/gtest.h"

TEST(MaskLibrary, Basic) {
  MaskLibrary mask_library(3);

  BitSet mask0 = mask_library.Mask1(0);
  EXPECT_EQ(mask0.count(), 4);
  EXPECT_TRUE(mask0.test(0b001));
  EXPECT_TRUE(mask0.test(0b011));
  EXPECT_TRUE(mask0.test(0b101));
  EXPECT_TRUE(mask0.test(0b111));

  BitSet mask1 = mask_library.Mask1(1);
  EXPECT_EQ(mask1.count(), 4);
  EXPECT_TRUE(mask1.test(0b010));
  EXPECT_TRUE(mask1.test(0b011));
  EXPECT_TRUE(mask1.test(0b110));
  EXPECT_TRUE(mask1.test(0b111));

  BitSet mask2 = mask_library.Mask1(2);
  EXPECT_EQ(mask2.count(), 4);
  EXPECT_TRUE(mask2.test(0b100));
  EXPECT_TRUE(mask2.test(0b101));
  EXPECT_TRUE(mask2.test(0b110));
  EXPECT_TRUE(mask2.test(0b111));

  BitSet mask01 = mask_library.Mask10(0, 1);
  EXPECT_EQ(mask01.count(), 2);
  EXPECT_TRUE(mask01.test(0b001));
  EXPECT_TRUE(mask01.test(0b101));

  BitSet mask02 = mask_library.Mask10(0, 2);
  EXPECT_EQ(mask02.count(), 2);
  EXPECT_TRUE(mask02.test(0b001));
  EXPECT_TRUE(mask02.test(0b011));

  BitSet mask12 = mask_library.Mask10(1, 2);
  EXPECT_EQ(mask12.count(), 2);
  EXPECT_TRUE(mask12.test(0b010));
  EXPECT_TRUE(mask12.test(0b011));
}

TEST(MaskLibrary, MaskByPopcount) {
  MaskLibrary mask_library(3);
  auto mbpc0 = mask_library.MaskByPopcount(0);
  EXPECT_EQ(mbpc0.count(), 1);
  EXPECT_TRUE(mbpc0.test(0b000));
  auto mbpc1 = mask_library.MaskByPopcount(1);
  EXPECT_TRUE(mbpc1.test(0b001));
  EXPECT_TRUE(mbpc1.test(0b010));
  EXPECT_TRUE(mbpc1.test(0b100));
  EXPECT_EQ(mbpc1.count(), 3);
  auto mbpc2 = mask_library.MaskByPopcount(2);
  EXPECT_EQ(mbpc2.count(), 3);
  EXPECT_TRUE(mbpc2.test(0b011));
  EXPECT_TRUE(mbpc2.test(0b101));
  EXPECT_TRUE(mbpc2.test(0b110));
  auto mbpc3 = mask_library.MaskByPopcount(3);
  EXPECT_EQ(mbpc3.count(), 1);
  EXPECT_TRUE(mbpc3.test(0b111));
}

TEST(MaskLibrary, Time17) {
  auto start = std::chrono::high_resolution_clock::now();
  MaskLibrary mask_library(17);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  std::cout << "MaskLibrary(17) took " << duration.count() << " seconds"
            << std::endl;
}
