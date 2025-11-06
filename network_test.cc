#include "network.h"

#include "gtest/gtest.h"

#include "comparator.h"
#include "network_utils.h"

// Layer Tests

TEST(LayerTest, Construction) {
  Layer layer(4);
  EXPECT_EQ(layer.n(), 4);
  EXPECT_EQ(layer.matching.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(layer.matching[i], -1);
  }
}

TEST(LayerTest, IsEmptyTrue) {
  Layer layer(3);
  EXPECT_TRUE(layer.IsEmpty());
}

TEST(LayerTest, IsEmptyFalse) {
  Layer layer(3);
  layer.matching[0] = 1;
  layer.matching[1] = 0;
  EXPECT_FALSE(layer.IsEmpty());
}

TEST(LayerTest, ToStringEmpty) {
  Layer layer(3);
  EXPECT_EQ(layer.ToString(), "");
}

TEST(LayerTest, ToStringSingleComparator) {
  Layer layer(4);
  layer.matching[0] = 1;
  layer.matching[1] = 0;
  EXPECT_EQ(layer.ToString(), "(0,1)");
}

TEST(LayerTest, ToStringMultipleComparators) {
  Layer layer(6);
  layer.matching[0] = 1;
  layer.matching[1] = 0;
  layer.matching[2] = 3;
  layer.matching[3] = 2;
  layer.matching[4] = 5;
  layer.matching[5] = 4;
  EXPECT_EQ(layer.ToString(), "(0,1),(2,3),(4,5)");
}

TEST(LayerTest, ToStringPartialMatching) {
  Layer layer(4);
  layer.matching[1] = 2;
  layer.matching[2] = 1;
  // 0 and 3 are unmatched
  EXPECT_EQ(layer.ToString(), "(1,2)");
}

TEST(LayerTest, ToProto) {
  Layer layer(3);
  layer.matching[0] = 2;
  layer.matching[1] = -1;
  layer.matching[2] = 0;

  pb::Layer proto = layer.ToProto();
  EXPECT_EQ(proto.matching_size(), 3);
  EXPECT_EQ(proto.matching(0), 2);
  EXPECT_EQ(proto.matching(1), -1);
  EXPECT_EQ(proto.matching(2), 0);
}

TEST(LayerTest, FromProto) {
  pb::Layer proto;
  proto.add_matching(1);
  proto.add_matching(0);
  proto.add_matching(3);
  proto.add_matching(2);

  Layer layer = Layer::FromProto(proto);
  EXPECT_EQ(layer.n(), 4);
  EXPECT_EQ(layer.matching[0], 1);
  EXPECT_EQ(layer.matching[1], 0);
  EXPECT_EQ(layer.matching[2], 3);
  EXPECT_EQ(layer.matching[3], 2);
}

TEST(LayerTest, RoundTripProto) {
  Layer original(5);
  original.matching[0] = 3;
  original.matching[1] = -1;
  original.matching[2] = 4;
  original.matching[3] = 0;
  original.matching[4] = 2;

  pb::Layer proto = original.ToProto();
  Layer recovered = Layer::FromProto(proto);

  EXPECT_EQ(original.n(), recovered.n());
  for (int i = 0; i < original.n(); i++) {
    EXPECT_EQ(original.matching[i], recovered.matching[i]);
  }
  EXPECT_TRUE(original == recovered);
}

// Network Tests

TEST(NetworkTest, Construction) {
  Network network(4, 3);
  EXPECT_EQ(network.n, 4);
  EXPECT_EQ(network.layers.size(), 3);
  for (const auto &layer : network.layers) {
    EXPECT_EQ(layer.n(), 4);
    EXPECT_TRUE(layer.IsEmpty());
  }
}

TEST(NetworkTest, ToStringEmpty) {
  Network network(3, 2);
  EXPECT_EQ(network.ToString(), "# n=3, depth=2, size=0\n[]\n[]\n");
  EXPECT_EQ(network.ToString(true), "# n=3, depth=2, size=0\n[],[]\n");
}

TEST(NetworkTest, ToStringSingleLayer) {
  Network network(4, 1);
  network.layers[0].matching[0] = 1;
  network.layers[0].matching[1] = 0;
  network.layers[0].matching[2] = 3;
  network.layers[0].matching[3] = 2;
  EXPECT_EQ(network.ToString(), "# n=4, depth=1, size=2\n[(0,1),(2,3)]\n");
  EXPECT_EQ(network.ToString(true), "# n=4, depth=1, size=2\n[(0,1),(2,3)]\n");
}

TEST(NetworkTest, ToStringMultipleLayers) {
  Network network(4, 2);
  network.layers[0].matching[0] = 2;
  network.layers[0].matching[1] = 3;
  network.layers[0].matching[2] = 0;
  network.layers[0].matching[3] = 1;
  network.layers[1].matching[0] = 1;
  network.layers[1].matching[1] = 0;
  EXPECT_EQ(network.ToString(),
            "# n=4, depth=2, size=3\n[(0,2),(1,3)]\n[(0,1)]\n");
  EXPECT_EQ(network.ToString(true),
            "# n=4, depth=2, size=3\n[(0,2),(1,3)],[(0,1)]\n");
}

TEST(NetworkTest, ToProto) {
  Network network(3, 2);
  network.layers[0].matching[0] = 1;
  network.layers[0].matching[1] = 0;
  network.layers[1].matching[1] = 2;
  network.layers[1].matching[2] = 1;
  network.outputs = {0, 1, 2, 3};

  pb::Network proto = network.ToProto();
  EXPECT_EQ(proto.n(), 3);
  EXPECT_EQ(proto.layer_size(), 2);
  EXPECT_EQ(proto.layer(0).matching_size(), 3);
  EXPECT_EQ(proto.layer(1).matching_size(), 3);
  EXPECT_EQ(proto.output_size(), 4);
  EXPECT_EQ(proto.output(0), 0);
  EXPECT_EQ(proto.output(3), 3);
}

TEST(NetworkTest, FromProto) {
  pb::Network proto;
  proto.set_n(4);

  pb::Layer *layer0 = proto.add_layer();
  layer0->add_matching(1);
  layer0->add_matching(0);
  layer0->add_matching(-1);
  layer0->add_matching(-1);

  pb::Layer *layer1 = proto.add_layer();
  layer1->add_matching(-1);
  layer1->add_matching(-1);
  layer1->add_matching(3);
  layer1->add_matching(2);

  Network network = Network::FromProto(proto);
  EXPECT_EQ(network.n, 4);
  EXPECT_EQ(network.layers.size(), 2);
  EXPECT_EQ(network.layers[0].matching[0], 1);
  EXPECT_EQ(network.layers[0].matching[1], 0);
  EXPECT_EQ(network.layers[0].matching[2], -1);
  EXPECT_EQ(network.layers[1].matching[2], 3);
  EXPECT_EQ(network.layers[1].matching[3], 2);
}

TEST(NetworkTest, RoundTripProto) {
  Network original(5, 3);
  original.layers[0].matching[0] = 1;
  original.layers[0].matching[1] = 0;
  original.layers[1].matching[2] = 3;
  original.layers[1].matching[3] = 2;
  original.layers[2].matching[1] = 4;
  original.layers[2].matching[4] = 1;
  original.outputs = {0, 1, 2, 3, 4, 5};

  pb::Network proto = original.ToProto();
  Network recovered = Network::FromProto(proto);

  EXPECT_EQ(original.n, recovered.n);
  EXPECT_EQ(original.layers.size(), recovered.layers.size());
  for (int i = 0; i < original.layers.size(); i++) {
    for (int j = 0; j < original.n; j++) {
      EXPECT_EQ(original.layers[i].matching[j],
                recovered.layers[i].matching[j]);
    }
  }
  EXPECT_TRUE(original == recovered);
}

TEST(NetworkTest, IsSymmetricTrue) {
  Network network(4, 2);
  // Symmetric pattern: (0,3), (1,2) in layer 0
  network.layers[0].matching[0] = 3;
  network.layers[0].matching[1] = 2;
  network.layers[0].matching[2] = 1;
  network.layers[0].matching[3] = 0;
  // Symmetric pattern: (0,1), (2,3) in layer 1
  network.layers[1].matching[0] = 1;
  network.layers[1].matching[1] = 0;
  network.layers[1].matching[2] = 3;
  network.layers[1].matching[3] = 2;

  EXPECT_TRUE(network.IsSymmetric());
}

TEST(NetworkTest, IsSymmetricFalse) {
  Network network(4, 1);
  // Asymmetric pattern: (0,1)
  network.layers[0].matching[0] = 1;
  network.layers[0].matching[1] = 0;

  EXPECT_FALSE(network.IsSymmetric());
}

TEST(NetworkTest, IsSymmetricEmpty) {
  Network network(4, 2);
  EXPECT_TRUE(network.IsSymmetric());
}

TEST(NetworkTest, IsSymmetricSingleChannel) {
  Network network(1, 1);
  EXPECT_TRUE(network.IsSymmetric());
}

TEST(NetworkTest, IsSymmetricWithUnmatchedChannels) {
  Network network(6, 1);
  // Symmetric: (1,4), (2,3) - channels 0 and 5 unmatched (which is symmetric)
  network.layers[0].matching[1] = 4;
  network.layers[0].matching[2] = 3;
  network.layers[0].matching[3] = 2;
  network.layers[0].matching[4] = 1;

  EXPECT_TRUE(network.IsSymmetric());
}

TEST(NetworkTest, IsASortingNetwork) {
  int n = 4;
  Network network(n, 0);
  for (OutputType x = 0; x < (OutputType(1) << n); x++) {
    network.outputs.push_back(x);
  }
  EXPECT_FALSE(network.IsASortingNetwork());
  network.AddEmptyLayer();
  network.AddComparator(Comparator(0, 2));
  network.AddComparator(Comparator(1, 3));
  EXPECT_FALSE(network.IsASortingNetwork());
  network.AddEmptyLayer();
  network.AddComparator(Comparator(0, 1));
  network.AddComparator(Comparator(2, 3));
  EXPECT_FALSE(network.IsASortingNetwork());
  network.AddEmptyLayer();
  network.AddComparator(Comparator(1, 2));
  EXPECT_TRUE(network.IsASortingNetwork());
}

TEST(NetworkTest, PermuteInputChannels3) {
  // (0,2), (0,1), (1,2)
  Network network(3, 3);
  network.layers[0].matching[0] = 2;
  network.layers[0].matching[2] = 0;
  network.layers[1].matching[0] = 1;
  network.layers[1].matching[1] = 0;
  network.layers[2].matching[1] = 2;
  network.layers[2].matching[2] = 1;
  network.outputs = NetworkOutputs(network);
  EXPECT_TRUE(network.IsASortingNetwork());
  // Test all permutations
  {
    Network permuted = network.PermuteInputChannels({0, 1, 2});
    EXPECT_EQ(network.outputs, NetworkOutputs(permuted));
  }
  {
    Network permuted = network.PermuteInputChannels({0, 2, 1});
    EXPECT_EQ(network.outputs, NetworkOutputs(permuted));
  }
  {
    Network permuted = network.PermuteInputChannels({1, 0, 2});
    EXPECT_EQ(network.outputs, NetworkOutputs(permuted));
  }
  {
    Network permuted = network.PermuteInputChannels({1, 2, 0});
    EXPECT_EQ(network.outputs, NetworkOutputs(permuted));
  }
  {
    Network permuted = network.PermuteInputChannels({2, 0, 1});
    EXPECT_EQ(network.outputs, NetworkOutputs(permuted));
  }
  {
    Network permuted = network.PermuteInputChannels({2, 1, 0});
    EXPECT_EQ(network.outputs, NetworkOutputs(permuted));
  }
}
