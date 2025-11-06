#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "network.h"
#include "network_utils.h"
#include "optimize_window_size.h"
#include "output_type.h"

DEFINE_int32(n, 0, "The number of channels.");
DEFINE_string(input_path, "", "The input prefixes file.");
DEFINE_string(output_path, "", "The output prefixes file.");
DEFINE_bool(symmetric, false, "Preserve symmetry.");
DEFINE_int32(limit, std::numeric_limits<int>::max(),
             "Limit the number of networks to process.");
DEFINE_bool(verbose, false, "Verbose mode.");

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "/tmp";
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  CHECK_EQ(argc, 1);

  CHECK_GT(FLAGS_n, 0);
  CHECK(!FLAGS_input_path.empty());
  CHECK(!FLAGS_output_path.empty());

  std::mt19937 gen;
  std::vector<Network> networks = LoadFromProtoFile(FLAGS_input_path, FLAGS_n);

  if (networks.size() > FLAGS_limit) {
    LOG(INFO) << "Limiting networks to " << FLAGS_limit;
    networks.erase(networks.begin() + FLAGS_limit, networks.end());
  }

  std::vector<std::vector<int>> permutations;
  for (int network_idx = 0; network_idx < networks.size(); network_idx++) {
    std::cout << "Processing network " << network_idx << "/" << networks.size()
              << '\r' << std::flush;
    Network &network = networks[network_idx];
    if (FLAGS_symmetric) {
      CHECK(IsSymmetric(FLAGS_n, network.outputs));
    }
    auto [new_outputs, perm] =
        OptimizeWindowSize(FLAGS_n, network.outputs, &gen, FLAGS_symmetric);
    if (FLAGS_verbose) {
      std::cout << std::endl;
      std::ostringstream oss;
      for (int i = 0; i < perm.size(); i++) {
        oss << perm[i] << ',';
      }
      LOG(INFO) << "Permutation: " << oss.str();
    }
    permutations.push_back(std::move(perm));
    CHECK_EQ(new_outputs.size(), network.outputs.size());
    network = Network(FLAGS_n, network.layers.size()); // Clear the network
    network.outputs = std::move(new_outputs);
    if (FLAGS_symmetric) {
      CHECK(IsSymmetric(FLAGS_n, network.outputs));
    }
  }
  std::cout << std::endl;

  SaveToProtoFile(networks, FLAGS_output_path);
  // Save the permutations to a file
  std::ofstream ofs(FLAGS_output_path + ".perm");
  for (const auto &perm : permutations) {
    for (int i : perm) {
      ofs << i << ' ';
    }
    ofs << '\n';
  }

  return 0;
}
