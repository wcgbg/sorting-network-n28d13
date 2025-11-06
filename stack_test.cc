#include "stack.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "network.h"
#include "network_utils.h"
#include "output_type.h"

// Helper function to create a simple network
Network CreateSimpleNetwork(int n, int num_layers) {
  Network network(n, num_layers);
  // Initialize with all possible outputs
  for (OutputType x = 0; x < (OutputType(1) << n); x++) {
    network.outputs.push_back(x);
  }
  return network;
}

// Test basic non-symmetric stacking with empty networks
TEST(StackTest, NonSymmetricEmptyNetworks) {
  Network net_a = CreateSimpleNetwork(2, 1);
  Network net_b = CreateSimpleNetwork(3, 1);

  Network result = StackNetworks(net_a, net_b, false);

  EXPECT_EQ(result.n, 5);
  EXPECT_EQ(result.layers.size(), 1);

  // Check all layers are empty
  for (const auto &layer : result.layers) {
    EXPECT_TRUE(layer.IsEmpty());
  }

  // Check outputs: should be Cartesian product of 2^2 * 2^3 = 32 outputs
  EXPECT_EQ(result.outputs.size(), 4 * 8);
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test basic symmetric stacking with empty networks
TEST(StackTest, SymmetricEmptyNetworks) {
  Network net_a = CreateSimpleNetwork(4, 1);
  Network net_b = CreateSimpleNetwork(2, 1);

  Network result = StackNetworks(net_a, net_b, true);

  EXPECT_EQ(result.n, 6);
  EXPECT_EQ(result.layers.size(), 1);

  // Check all layers are empty
  for (const auto &layer : result.layers) {
    EXPECT_TRUE(layer.IsEmpty());
  }

  // Check outputs: should be Cartesian product of 2^4 * 2^2 = 64 outputs
  EXPECT_EQ(result.outputs.size(), 16 * 4);
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test non-symmetric stacking with single comparator in each network
TEST(StackTest, NonSymmetricSingleComparators) {
  Network net_a(2, 1);
  net_a.layers[0].matching[0] = 1;
  net_a.layers[0].matching[1] = 0;
  net_a.outputs = NetworkOutputs(net_a);

  Network net_b(2, 1);
  net_b.layers[0].matching[0] = 1;
  net_b.layers[0].matching[1] = 0;
  net_b.outputs = NetworkOutputs(net_b);

  Network result = StackNetworks(net_a, net_b, false);

  EXPECT_EQ(result.n, 4);
  EXPECT_EQ(result.layers.size(), 1);

  // Check comparators: net_a stays (0,1), net_b becomes (2,3)
  EXPECT_EQ(result.layers[0].matching[0], 1);
  EXPECT_EQ(result.layers[0].matching[1], 0);
  EXPECT_EQ(result.layers[0].matching[2], 3);
  EXPECT_EQ(result.layers[0].matching[3], 2);

  EXPECT_EQ(result.outputs.size(), net_a.outputs.size() * net_b.outputs.size());
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test symmetric stacking with comparators
TEST(StackTest, SymmetricWithComparators) {
  Network net_a(4, 1);
  // Add comparators (0,3) and (1,2) - symmetric pattern
  net_a.layers[0].matching[0] = 3;
  net_a.layers[0].matching[1] = 2;
  net_a.layers[0].matching[2] = 1;
  net_a.layers[0].matching[3] = 0;
  net_a.outputs = NetworkOutputs(net_a);

  Network net_b(2, 1);
  // Add comparator (0,1)
  net_b.layers[0].matching[0] = 1;
  net_b.layers[0].matching[1] = 0;
  net_b.outputs = NetworkOutputs(net_b);

  Network result = StackNetworks(net_a, net_b, true);

  EXPECT_EQ(result.n, 6);
  EXPECT_EQ(result.layers.size(), 1);

  // In symmetric mode:
  // net_a: channels 0,1 stay, channels 2,3 shift by +2 -> (0,1,4,5)
  // net_b: channels shift by +2 -> (2,3)
  // So net_a (0,3) becomes (0,5) and (1,2) becomes (1,4)
  EXPECT_EQ(result.layers[0].matching[0], 5);
  EXPECT_EQ(result.layers[0].matching[1], 4);
  EXPECT_EQ(result.layers[0].matching[4], 1);
  EXPECT_EQ(result.layers[0].matching[5], 0);
  // net_b (0,1) becomes (2,3)
  EXPECT_EQ(result.layers[0].matching[2], 3);
  EXPECT_EQ(result.layers[0].matching[3], 2);

  // Check outputs: 2 * 2 = 4 outputs
  EXPECT_EQ(result.outputs.size(), net_a.outputs.size() * net_b.outputs.size());
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test multiple layers
TEST(StackTest, NonSymmetricMultipleLayers) {
  Network net_a(2, 2);
  net_a.layers[0].matching[0] = 1;
  net_a.layers[0].matching[1] = 0;
  net_a.layers[1].matching[0] = 1;
  net_a.layers[1].matching[1] = 0;
  net_a.outputs = NetworkOutputs(net_a);

  Network net_b(2, 1);
  net_b.layers[0].matching[0] = 1;
  net_b.layers[0].matching[1] = 0;
  net_b.outputs = NetworkOutputs(net_b);

  Network result = StackNetworks(net_a, net_b, false);

  EXPECT_EQ(result.n, 4);
  EXPECT_EQ(result.layers.size(), 2);

  // Layer 0: both networks have comparators at layer 0
  EXPECT_EQ(result.layers[0].matching[0], 1);
  EXPECT_EQ(result.layers[0].matching[1], 0);
  EXPECT_EQ(result.layers[0].matching[2], 3);
  EXPECT_EQ(result.layers[0].matching[3], 2);

  // Layer 1: only net_a has comparators
  EXPECT_EQ(result.layers[1].matching[0], 1);
  EXPECT_EQ(result.layers[1].matching[1], 0);
  EXPECT_EQ(result.layers[1].matching[2], -1);
  EXPECT_EQ(result.layers[1].matching[3], -1);

  // Check outputs: 2 * 2 = 4 outputs
  EXPECT_EQ(result.outputs.size(), net_a.outputs.size() * net_b.outputs.size());
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test output merging with different bit patterns
TEST(StackTest, OutputMergingNonSymmetric) {
  Network net_a(2, 0);
  // net_a has outputs with bit 0 set
  net_a.outputs = NetworkOutputs(net_a);

  Network net_b(2, 0);
  // net_b has outputs with bit 1 set
  net_b.outputs = NetworkOutputs(net_b);

  Network result = StackNetworks(net_a, net_b, false);

  EXPECT_EQ(result.n, 4);
  EXPECT_EQ(result.outputs.size(), net_a.outputs.size() * net_b.outputs.size());
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test output merging in symmetric mode
TEST(StackTest, OutputMergingSymmetric) {
  Network net_a(4, 0);
  // Output with bits 0 and 2 set
  net_a.outputs = NetworkOutputs(net_a);

  Network net_b(2, 0);
  // Output with both bits set
  net_b.outputs = NetworkOutputs(net_b);

  Network result = StackNetworks(net_a, net_b, true);

  EXPECT_EQ(result.n, 6);
  EXPECT_EQ(result.outputs.size(), net_a.outputs.size() * net_b.outputs.size());
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test with networks of different sizes
TEST(StackTest, DifferentSizes) {
  Network net_a = CreateSimpleNetwork(3, 1);
  Network net_b = CreateSimpleNetwork(5, 1);

  Network result = StackNetworks(net_a, net_b, false);

  EXPECT_EQ(result.n, 8);
  EXPECT_EQ(result.outputs.size(), 8 * 32); // 2^3 * 2^5
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test edge case: single channel networks
TEST(StackTest, SingleChannelNetworks) {
  Network net_a(1, 1);
  net_a.outputs = NetworkOutputs(net_a);

  Network net_b(1, 1);
  net_b.outputs = NetworkOutputs(net_b);

  Network result = StackNetworks(net_a, net_b, false);

  EXPECT_EQ(result.n, 2);
  EXPECT_EQ(result.outputs.size(), 4);
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test channel permutation correctness in non-symmetric mode
TEST(StackTest, ChannelPermutationNonSymmetric) {
  Network net_a(3, 1);
  net_a.layers[0].matching[1] = 2;
  net_a.layers[0].matching[2] = 1;
  net_a.outputs = NetworkOutputs(net_a);

  Network net_b(2, 1);
  net_b.layers[0].matching[0] = 1;
  net_b.layers[0].matching[1] = 0;
  net_b.outputs = NetworkOutputs(net_b);

  Network result = StackNetworks(net_a, net_b, false);

  EXPECT_EQ(result.n, 5);

  // net_a comparator (1,2) stays (1,2)
  EXPECT_EQ(result.layers[0].matching[1], 2);
  EXPECT_EQ(result.layers[0].matching[2], 1);

  // net_b comparator (0,1) becomes (3,4)
  EXPECT_EQ(result.layers[0].matching[3], 4);
  EXPECT_EQ(result.layers[0].matching[4], 3);

  // Channel 0 from net_a is unmatched
  EXPECT_EQ(result.layers[0].matching[0], -1);

  EXPECT_EQ(result.outputs.size(), net_a.outputs.size() * net_b.outputs.size());
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test channel permutation correctness in symmetric mode
TEST(StackTest, ChannelPermutationSymmetric) {
  Network net_a(4, 1);
  // Comparator (0,1)
  net_a.layers[0].matching[0] = 1;
  net_a.layers[0].matching[1] = 0;
  net_a.outputs = NetworkOutputs(net_a);

  Network net_b(2, 1);
  // Comparator (0,1)
  net_b.layers[0].matching[0] = 1;
  net_b.layers[0].matching[1] = 0;
  net_b.outputs = NetworkOutputs(net_b);

  Network result = StackNetworks(net_a, net_b, true);

  EXPECT_EQ(result.n, 6);

  // In symmetric mode with n_a=4, n_b=2:
  // net_a channels: 0,1 stay; 2,3 shift to 4,5
  // net_a comparator (0,1) stays (0,1)
  EXPECT_EQ(result.layers[0].matching[0], 1);
  EXPECT_EQ(result.layers[0].matching[1], 0);

  // net_b channels: shift to 2,3
  // net_b comparator (0,1) becomes (2,3)
  EXPECT_EQ(result.layers[0].matching[2], 3);
  EXPECT_EQ(result.layers[0].matching[3], 2);

  // Channels 4,5 are unmatched
  EXPECT_EQ(result.layers[0].matching[4], -1);
  EXPECT_EQ(result.layers[0].matching[5], -1);

  EXPECT_EQ(result.outputs.size(), net_a.outputs.size() * net_b.outputs.size());
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test that comparators from different layers don't interfere
TEST(StackTest, LayerSeparation) {
  Network net_a(2, 2);
  net_a.layers[0].matching[0] = 1;
  net_a.layers[0].matching[1] = 0;
  // Layer 1 is empty
  net_a.outputs = NetworkOutputs(net_a);

  Network net_b(2, 2);
  // Layer 0 is empty
  net_b.layers[1].matching[0] = 1;
  net_b.layers[1].matching[1] = 0;
  net_b.outputs = NetworkOutputs(net_b);

  Network result = StackNetworks(net_a, net_b, false);

  EXPECT_EQ(result.layers.size(), 2);

  // Layer 0 should only have net_a's comparator (0,1)
  EXPECT_EQ(result.layers[0].matching[0], 1);
  EXPECT_EQ(result.layers[0].matching[1], 0);
  EXPECT_EQ(result.layers[0].matching[2], -1);
  EXPECT_EQ(result.layers[0].matching[3], -1);

  // Layer 1 should only have net_b's comparator (2,3)
  EXPECT_EQ(result.layers[1].matching[0], -1);
  EXPECT_EQ(result.layers[1].matching[1], -1);
  EXPECT_EQ(result.layers[1].matching[2], 3);
  EXPECT_EQ(result.layers[1].matching[3], 2);

  EXPECT_EQ(result.outputs.size(), net_a.outputs.size() * net_b.outputs.size());
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}

// Test with larger networks to verify scalability
TEST(StackTest, LargerNetworks) {
  Network net_a = CreateSimpleNetwork(6, 2);
  net_a.layers[0].matching[0] = 1;
  net_a.layers[0].matching[1] = 0;
  net_a.layers[0].matching[2] = 3;
  net_a.layers[0].matching[3] = 2;
  net_a.layers[1].matching[0] = 1;
  net_a.layers[1].matching[1] = 0;

  Network net_b = CreateSimpleNetwork(4, 2);
  net_b.layers[0].matching[0] = 1;
  net_b.layers[0].matching[1] = 0;
  net_b.layers[1].matching[0] = 1;
  net_b.layers[1].matching[1] = 0;

  Network result = StackNetworks(net_a, net_b, true);

  EXPECT_EQ(result.n, 10);
  EXPECT_EQ(result.layers.size(), 2);
  EXPECT_EQ(result.outputs.size(), 64 * 16); // 2^6 * 2^4
  EXPECT_EQ(result.outputs, NetworkOutputs(result));
}
