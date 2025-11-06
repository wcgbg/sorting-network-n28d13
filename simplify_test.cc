#include "simplify.h"

#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "comparator.h"
#include "network.h"

// Helper function to create a network from a vector of layers
// Each layer is represented as a vector of comparators (pairs of indices)
Network
CreateNetwork(int n,
              const std::vector<std::vector<std::pair<int, int>>> &layers) {
  Network network(n, 0);
  for (const auto &layer : layers) {
    network.AddEmptyLayer();
    for (const auto &[i, j] : layer) {
      network.AddComparator(Comparator(i, j));
    }
  }
  return network;
}

TEST(SimplifyTest, FourChannelsThreeLayers) {
  // Input network:
  // [(0,2),(1,3)]
  // [(0,1),(2,3)]
  // [(0,3),(1,2)]
  Network input =
      CreateNetwork(4, {{{0, 2}, {1, 3}}, {{0, 1}, {2, 3}}, {{0, 3}, {1, 2}}});

  Network result = Simplify(input);

  // Expected network:
  // [(0,2),(1,3)]
  // [(0,1),(2,3)]
  // [(1,2)]
  Network expected =
      CreateNetwork(4, {{{0, 2}, {1, 3}}, {{0, 1}, {2, 3}}, {{1, 2}}});

  EXPECT_EQ(result.n, expected.n);
  EXPECT_EQ(result.layers.size(), expected.layers.size());
  for (int layer_idx = 0; layer_idx < expected.layers.size(); layer_idx++) {
    EXPECT_EQ(result.layers[layer_idx], expected.layers[layer_idx])
        << "Layer " << layer_idx << " does not match\n"
        << "Expected: " << expected.layers[layer_idx].ToString() << "\n"
        << "Got: " << result.layers[layer_idx].ToString();
  }
}

TEST(SimplifyTest, SixChannelsFiveLayers) {
  // Input network:
  // [(0,5),(1,3),(2,4)]
  // [(0,5),(1,2),(3,4)]
  // [(0,3),(1,4),(2,5)]
  // [(0,1),(2,3),(4,5)]
  // [(0,5),(1,2),(3,4)]
  Network input = CreateNetwork(6, {{{0, 5}, {1, 3}, {2, 4}},
                                    {{0, 5}, {1, 2}, {3, 4}},
                                    {{0, 3}, {1, 4}, {2, 5}},
                                    {{0, 1}, {2, 3}, {4, 5}},
                                    {{0, 5}, {1, 2}, {3, 4}}});

  Network result = Simplify(input);

  // Expected network:
  // [(0,5),(1,3),(2,4)]
  // [(1,2),(3,4)]
  // [(0,3),(2,5)]
  // [(0,1),(2,3),(4,5)]
  // [(1,2),(3,4)]
  Network expected = CreateNetwork(6, {{{0, 5}, {1, 3}, {2, 4}},
                                       {{1, 2}, {3, 4}},
                                       {{0, 3}, {2, 5}},
                                       {{0, 1}, {2, 3}, {4, 5}},
                                       {{1, 2}, {3, 4}}});

  EXPECT_EQ(result.n, expected.n);
  EXPECT_EQ(result.layers.size(), expected.layers.size());
  for (int layer_idx = 0; layer_idx < expected.layers.size(); layer_idx++) {
    EXPECT_EQ(result.layers[layer_idx], expected.layers[layer_idx])
        << "Layer " << layer_idx << " does not match\n"
        << "Expected: " << expected.layers[layer_idx].ToString() << "\n"
        << "Got: " << result.layers[layer_idx].ToString();
  }
}
