/*
This program generates prefixes for sorting networks, s.t.
- The prefix is non-redundant, i.e. we cannot remove a comparator from the
prefix and keep its outputs.
- The outputs is minimal under permutation, i.e. we cannot add a comparator to
the prefix to shrink the outputs, even under permutation.

The first layer is (0,1),(2,3),....
*/

#include <limits>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "boost/algorithm/string.hpp"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "extend_network.h"
#include "network.h"
#include "network_utils.h"

DEFINE_int32(n, 0, "The number of channels.");
DEFINE_bool(symmetric, false, "Build symmetric networks.");
DEFINE_int32(input_depth, 1, "The depth of the input prefixes.");
DEFINE_string(input_path, "", "The input prefixes file.");
DEFINE_int32(output_depth, 0, "The depth of the output prefixes.");
DEFINE_string(output_path, "", "The output prefixes file.");
DEFINE_string(
    keep_best_count, "",
    "The number of networks to keep for each depth, separated by commas. "
    "If empty, all networks will be kept.");
DEFINE_int32(jobs, std::thread::hardware_concurrency(),
             "The number of workers to use for parallel processing.");

std::vector<int> ParseKeepBestCount() {
  if (FLAGS_keep_best_count.empty()) {
    return std::vector<int>(FLAGS_output_depth - FLAGS_input_depth,
                            std::numeric_limits<int>::max());
  }
  std::vector<std::string> tokens;
  boost::split(tokens, FLAGS_keep_best_count, boost::is_any_of(","));
  CHECK_EQ(tokens.size(), FLAGS_output_depth - FLAGS_input_depth);
  std::vector<int> keep_best_counts;
  for (const auto &token : tokens) {
    if (token.empty()) {
      keep_best_counts.push_back(std::numeric_limits<int>::max());
    } else {
      keep_best_counts.push_back(std::stoi(token));
    }
    LOG(INFO) << "keep_best_counts.back()=" << keep_best_counts.back()
              << ", token=" << token;
  }
  CHECK_EQ(keep_best_counts.size(), FLAGS_output_depth - FLAGS_input_depth);
  return keep_best_counts;
}

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "/tmp";
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  CHECK_EQ(argc, 1);

  CHECK_GT(FLAGS_n, 0);
  CHECK(!FLAGS_output_path.empty());

  std::vector<Network> networks;
  LOG(INFO) << "Loading networks from " << FLAGS_input_path;
  if (FLAGS_input_depth == 1) {
    CHECK(FLAGS_input_path.empty());
    networks = CreateFirstLayer(FLAGS_n, FLAGS_symmetric);
  } else {
    CHECK(!FLAGS_input_path.empty());
    networks = LoadFromProtoFile(FLAGS_input_path, FLAGS_n);
  }

  for (const auto &network : networks) {
    CHECK_EQ(network.layers.size(), FLAGS_input_depth);
  }
  LOG(INFO) << "Loaded " << networks.size() << " networks";

  std::vector<int> keep_best_counts = ParseKeepBestCount();

  std::mt19937 gen;
  for (int depth = FLAGS_input_depth; depth < FLAGS_output_depth; depth++) {
    LOG(INFO) << "Extending networks from depth " << depth << " to "
              << depth + 1;
    for (auto &network : networks) {
      network.AddEmptyLayer();
    }
    networks = ExtendNetwork(FLAGS_n, networks, FLAGS_symmetric, false,
                             keep_best_counts.at(depth - FLAGS_input_depth),
                             FLAGS_jobs, &gen);
  }

  LOG(INFO) << "Saving " << networks.size() << " networks to "
            << FLAGS_output_path;
  SaveToProtoFile(networks, FLAGS_output_path);

  return 0;
}
