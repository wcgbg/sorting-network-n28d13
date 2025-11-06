#include "network_utils.h"

#include <fstream>

#include "gtest/gtest.h"

// LoadFromBracketFile Tests

TEST(LoadPrefixesTest, EmptyFile) {
  // Create a temporary empty file
  std::string filename = "/tmp/test_empty_prefixes.txt";
  std::ofstream file(filename);
  file.close();

  std::vector<Network> networks = LoadFromBracketFile(3, filename);
  EXPECT_EQ(networks.size(), 0);
}

TEST(LoadPrefixesTest, SingleNetwork) {
  std::string filename = "/tmp/test_single_prefix.txt";
  std::ofstream file(filename);
  file << "[(0,1),(2,3)]\n";
  file.close();

  std::vector<Network> networks = LoadFromBracketFile(4, filename, false);
  ASSERT_EQ(networks.size(), 1);
  EXPECT_EQ(networks[0].n, 4);
  EXPECT_EQ(networks[0].layers.size(), 1);
  EXPECT_EQ(networks[0].layers[0].matching[0], 1);
  EXPECT_EQ(networks[0].layers[0].matching[1], 0);
  EXPECT_EQ(networks[0].layers[0].matching[2], 3);
  EXPECT_EQ(networks[0].layers[0].matching[3], 2);
}

TEST(LoadPrefixesTest, MultipleNetworks) {
  std::string filename = "/tmp/test_multiple_prefixes.txt";
  std::ofstream file(filename);
  file << "[(0,1)]\n";
  file << "[(0,2)],[(1,2)]\n";
  file.close();

  std::vector<Network> networks = LoadFromBracketFile(3, filename, false);
  ASSERT_EQ(networks.size(), 2);

  // First network: single comparator (0,1) in layer 0
  EXPECT_EQ(networks[0].layers.size(), 1);
  EXPECT_EQ(networks[0].layers[0].matching[0], 1);
  EXPECT_EQ(networks[0].layers[0].matching[1], 0);

  // Second network: (0,2) in layer 0, (1,2) in layer 1
  EXPECT_EQ(networks[1].layers.size(), 2);
  EXPECT_EQ(networks[1].layers[0].matching[0], 2);
  EXPECT_EQ(networks[1].layers[0].matching[2], 0);
  EXPECT_EQ(networks[1].layers[1].matching[1], 2);
  EXPECT_EQ(networks[1].layers[1].matching[2], 1);
}

TEST(LoadPrefixesTest, SkipEmptyLines) {
  std::string filename = "/tmp/test_empty_lines.txt";
  std::ofstream file(filename);
  file << "\n";
  file << "[(0,1)]\n";
  file << "\n";
  file << "[(1,2)]\n";
  file << "\n";
  file.close();

  std::vector<Network> networks = LoadFromBracketFile(3, filename, false);
  ASSERT_EQ(networks.size(), 2);
}

TEST(LoadPrefixesTest, SkipCommentLines) {
  std::string filename = "/tmp/test_comments.txt";
  std::ofstream file(filename);
  file << "# This is a comment\n";
  file << "[(0,1)]\n";
  file << "# Another comment\n";
  file << "[(1,2)]\n";
  file.close();

  std::vector<Network> networks = LoadFromBracketFile(3, filename, false);
  ASSERT_EQ(networks.size(), 2);
}

TEST(SaveToBracketFileTest, RoundTrip) {
  // Create test networks with same n
  std::vector<Network> original_networks;

  // Network 0: [(0,1),(2,3)]
  Network net0(4, 1);
  net0.layers[0].matching[0] = 1;
  net0.layers[0].matching[1] = 0;
  net0.layers[0].matching[2] = 3;
  net0.layers[0].matching[3] = 2;
  original_networks.push_back(net0);

  // Network 1: [(0,2)],[(1,3)]
  Network net1(4, 2);
  net1.layers[0].matching[0] = 2;
  net1.layers[0].matching[2] = 0;
  net1.layers[1].matching[1] = 3;
  net1.layers[1].matching[3] = 1;
  original_networks.push_back(net1);

  // Save to file
  std::string filename = "/tmp/test_bracket_roundtrip.txt";
  SaveToBracketFile(original_networks, filename);

  // Read the file content to verify format
  std::ifstream check_file(filename);
  std::string line0, line1, line2, line3;
  std::getline(check_file, line0);
  std::getline(check_file, line1);
  std::getline(check_file, line2);
  std::getline(check_file, line3);
  check_file.close();

  EXPECT_TRUE(line0.starts_with('#'));
  EXPECT_EQ(line1, "[(0,1),(2,3)]");
  EXPECT_TRUE(line2.starts_with('#'));
  EXPECT_EQ(line3, "[(0,2)],[(1,3)]");

  // Load back and verify
  std::vector<Network> loaded_networks =
      LoadFromBracketFile(4, filename, false);
  EXPECT_TRUE(loaded_networks == original_networks);
}

// Proto File I/O Tests

TEST(ProtoFileIOTest, SaveAndLoadTextFormat) {
  std::string filename = "/tmp/test_network.txt";

  // Create test networks
  std::vector<Network> original_networks;
  Network net1(4, 1);
  net1.layers[0].matching[0] = 1;
  net1.layers[0].matching[1] = 0;
  net1.outputs = {0, 1, 2}; // fake outputs
  original_networks.push_back(net1);

  Network net2(4, 2);
  net2.layers[0].matching[0] = 2;
  net2.layers[0].matching[2] = 0;
  net2.layers[1].matching[1] = 3;
  net2.layers[1].matching[3] = 1;
  net2.outputs = {0, 1}; // fake outputs
  original_networks.push_back(net2);

  // Save to file
  SaveToProtoFile(original_networks, filename);

  // Load from file
  std::vector<Network> loaded_networks = LoadFromProtoFile(filename);

  ASSERT_EQ(loaded_networks.size(), 2);

  // Verify first network
  EXPECT_EQ(loaded_networks[0].n, 4);
  EXPECT_EQ(loaded_networks[0].layers.size(), 1);
  EXPECT_EQ(loaded_networks[0].layers[0].matching[0], 1);
  EXPECT_EQ(loaded_networks[0].layers[0].matching[1], 0);

  // Verify second network
  EXPECT_EQ(loaded_networks[1].n, 4);
  EXPECT_EQ(loaded_networks[1].layers.size(), 2);
  EXPECT_EQ(loaded_networks[1].layers[0].matching[0], 2);
  EXPECT_EQ(loaded_networks[1].layers[1].matching[1], 3);
}

TEST(ProtoFileIOTest, SaveAndLoadBinaryFormat) {
  std::string filename = "/tmp/test_network.pb";

  // Create test network
  std::vector<Network> original_networks;
  Network net(5, 2);
  net.layers[0].matching[0] = 4;
  net.layers[0].matching[4] = 0;
  net.layers[1].matching[1] = 3;
  net.layers[1].matching[3] = 1;
  original_networks.push_back(net);

  // Save to binary file
  SaveToProtoFile(original_networks, filename);

  // Load from binary file
  std::vector<Network> loaded_networks = LoadFromProtoFile(filename);

  ASSERT_EQ(loaded_networks.size(), 1);
  EXPECT_EQ(loaded_networks[0].n, 5);
  EXPECT_EQ(loaded_networks[0].layers.size(), 2);
  EXPECT_EQ(loaded_networks[0].layers[0].matching[0], 4);
  EXPECT_EQ(loaded_networks[0].layers[1].matching[1], 3);
}

TEST(ProtoFileIOTest, LoadWithNFilter) {
  std::string filename = "/tmp/test_network_filter.txt";

  // Create networks with different n values
  std::vector<Network> original_networks;
  Network net1(3, 1);
  net1.layers[0].matching[0] = 1;
  net1.layers[0].matching[1] = 0;
  original_networks.push_back(net1);

  SaveToProtoFile(original_networks, filename);

  // Load with n filter
  std::vector<Network> loaded_networks = LoadFromProtoFile(filename, 3);
  ASSERT_EQ(loaded_networks.size(), 1);
  EXPECT_EQ(loaded_networks[0].n, 3);
}

TEST(CreateFirstLayerTest, Symmetric2) {
  std::vector<Network> networks = CreateFirstLayer(2, true);
  ASSERT_EQ(networks.size(), 1);
  {
    ASSERT_EQ(networks[0].n, 2);
    ASSERT_EQ(networks[0].layers.size(), 1);
    const Layer &layer = networks[0].layers[0];
    EXPECT_EQ(layer.matching[0], 1);
    EXPECT_EQ(layer.matching[1], 0);
  }
}

TEST(CreateFirstLayerTest, Symmetric4) {
  std::vector<Network> networks = CreateFirstLayer(4, true);
  ASSERT_EQ(networks.size(), 2);
  {
    ASSERT_EQ(networks[0].n, 4);
    ASSERT_EQ(networks[0].layers.size(), 1);
    const Layer &layer = networks[0].layers[0];
    EXPECT_EQ(layer.matching[0], 3);
    EXPECT_EQ(layer.matching[1], 2);
    EXPECT_EQ(layer.matching[2], 1);
    EXPECT_EQ(layer.matching[3], 0);
  }
  {
    ASSERT_EQ(networks[1].n, 4);
    ASSERT_EQ(networks[1].layers.size(), 1);
    const Layer &layer = networks[1].layers[0];
    EXPECT_EQ(layer.matching[0], 1);
    EXPECT_EQ(layer.matching[1], 0);
    EXPECT_EQ(layer.matching[2], 3);
    EXPECT_EQ(layer.matching[3], 2);
  }
}

TEST(CreateFirstLayerTest, NonSymmetric3) {
  std::vector<Network> networks = CreateFirstLayer(3, false);
  ASSERT_EQ(networks.size(), 1);
  ASSERT_EQ(networks[0].n, 3);
  ASSERT_EQ(networks[0].layers.size(), 1);
  const Layer &layer = networks[0].layers[0];
  EXPECT_EQ(layer.matching[0], 1);
  EXPECT_EQ(layer.matching[1], 0);
}

TEST(CreateFirstLayerTest, NonSymmetric4) {
  std::vector<Network> networks = CreateFirstLayer(4, false);
  ASSERT_EQ(networks.size(), 1);
  ASSERT_EQ(networks[0].n, 4);
  ASSERT_EQ(networks[0].layers.size(), 1);
  const Layer &layer = networks[0].layers[0];
  EXPECT_EQ(layer.matching[0], 1);
  EXPECT_EQ(layer.matching[1], 0);
  EXPECT_EQ(layer.matching[2], 3);
  EXPECT_EQ(layer.matching[3], 2);
}
