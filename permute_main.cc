#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "network.h"
#include "network_utils.h"

DEFINE_int32(n, 0, "The number of channels.");
DEFINE_string(input_network, "", "The input network file path.");
DEFINE_string(permutation, "",
              "The permutation over the n channels, like 2,0,1 for n=3.");
DEFINE_string(output_network, "", "The output network file path.");

std::vector<int> ParsePermutation(std::string permutation) {
  // replace ',' with ' ' in permutation
  std::replace(permutation.begin(), permutation.end(), ',', ' ');
  std::vector<int> perm;
  std::vector<bool> is_used(FLAGS_n, false);
  std::istringstream iss(permutation);
  int x = -1;
  while (iss >> x) {
    CHECK_GE(x, 0);
    CHECK_LT(x, FLAGS_n);
    perm.push_back(x);
    CHECK(!is_used[x]);
    is_used[x] = true;
  }
  CHECK_EQ(perm.size(), FLAGS_n);
  return perm;
}

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "/tmp";
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  CHECK_EQ(argc, 1);

  CHECK_GT(FLAGS_n, 2);

  std::vector<int> permutation = ParsePermutation(FLAGS_permutation);
  CHECK_EQ(permutation.size(), FLAGS_n);

  std::string output_network = FLAGS_output_network;
  if (output_network.empty()) {
    output_network = FLAGS_input_network + ".permuted";
  }
  std::vector<Network> networks =
      LoadFromBracketFile(FLAGS_n, FLAGS_input_network, 0);
  for (Network &network : networks) {
    network = network.PermuteInputChannels(permutation);
  }
  SaveToBracketFile(networks, output_network);

  return 0;
}
