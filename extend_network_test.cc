#include "extend_network.h"

#include <limits>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "network_utils.h"

void TestTwoLayers(int n, bool symmetric, int expected_networks_count) {
  std::mt19937 gen;
  std::vector<Network> networks = CreateFirstLayer(n, symmetric);
  int keep_best_count = std::numeric_limits<int>::max();
  int jobs = std::thread::hardware_concurrency();
  CHECK_GT(jobs, 0);
  for (Network &network : networks) {
    network.AddEmptyLayer();
  }
  networks =
      ExtendNetwork(n, networks, symmetric, false, keep_best_count, jobs, &gen);

  EXPECT_EQ(networks.size(), expected_networks_count)
      << "n=" << n << ", symmetric=" << symmetric;
}

TEST(ExtendPrefixFull, TwoLayers) {
  TestTwoLayers(3, false, 1);

  TestTwoLayers(4, false, 2);
  TestTwoLayers(4, true, 2);

  TestTwoLayers(5, false, 4);

  TestTwoLayers(6, false, 5);
  TestTwoLayers(6, true, 4);

  TestTwoLayers(7, false, 8);

  TestTwoLayers(8, false, 12);
  TestTwoLayers(8, true, 12);

  TestTwoLayers(9, false, 22);
}
