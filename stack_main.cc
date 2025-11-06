/*
Stack two small networks into a big one.

This takes two network collections in protobuf format, stacks each pair of
networks from the two collections, and outputs the result.
*/

#include <string>
#include <utility>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "network.h"
#include "network_utils.h"
#include "output_type.h"
#include "stack.h"

DEFINE_bool(symmetric, false, "Whether to stack in symmetric mode.");
DEFINE_int32(n_a, 0, "Input: Number of channels in first network.");
DEFINE_string(input_path_a, "", "Input: Path to first proto file.");
DEFINE_int32(n_b, 0, "Input: Number of channels in second network.");
DEFINE_string(input_path_b, "", "Input: Path to second proto file.");
DEFINE_string(output_path, "", "Output: Path to output proto file.");

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "/tmp";
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  CHECK_EQ(argc, 1);

  CHECK_GT(FLAGS_n_a, 0)
      << "Number of channels in first network must be positive.";
  CHECK_GT(FLAGS_n_b, 0)
      << "Number of channels in second network must be positive.";
  CHECK(!FLAGS_input_path_a.empty()) << "First input file must be specified.";
  CHECK(!FLAGS_input_path_b.empty()) << "Second input file must be specified.";
  CHECK(!FLAGS_output_path.empty()) << "Output file must be specified.";

  if (FLAGS_symmetric) {
    CHECK_EQ(FLAGS_n_a % 2, 0) << "n_a must be even in symmetric mode.";
    CHECK_EQ(FLAGS_n_b % 2, 0) << "n_b must be even in symmetric mode.";
  }

  LOG(INFO) << "Loading networks from: " << FLAGS_input_path_a;
  std::vector<Network> networks_a =
      LoadFromProtoFile(FLAGS_input_path_a, FLAGS_n_a);
  LOG(INFO) << "Loaded " << networks_a.size() << " networks from first file.";

  LOG(INFO) << "Loading networks from: " << FLAGS_input_path_b;
  std::vector<Network> networks_b =
      LoadFromProtoFile(FLAGS_input_path_b, FLAGS_n_b);
  LOG(INFO) << "Loaded " << networks_b.size() << " networks from second file.";

  if (FLAGS_symmetric) {
    CHECK_EQ(FLAGS_n_a % 2, 0);
    CHECK_EQ(FLAGS_n_b % 2, 0);
    for (const Network &network : networks_a) {
      CHECK(network.IsSymmetric()) << FLAGS_input_path_a << " is not symmetric";
    }
    for (const Network &network : networks_b) {
      CHECK(network.IsSymmetric()) << FLAGS_input_path_b << " is not symmetric";
    }
  }

  std::vector<Network> stacked_networks;
  for (const Network &net_a : networks_a) {
    for (const Network &net_b : networks_b) {
      Network stacked = StackNetworks(net_a, net_b, FLAGS_symmetric);
      if (FLAGS_symmetric) {
        CHECK(stacked.IsSymmetric()) << "Stacked network is not symmetric";
        CHECK(IsSymmetric(net_a.n + net_b.n, stacked.outputs))
            << "Stacked network outputs are not symmetric";
      }
      stacked_networks.push_back(std::move(stacked));
    }
  }

  LOG(INFO) << "Created " << stacked_networks.size() << " stacked networks.";
  LOG(INFO) << "Saving to: " << FLAGS_output_path;
  SaveToProtoFile(stacked_networks, FLAGS_output_path);
  LOG(INFO) << "Done.";

  return 0;
}
