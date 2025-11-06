#include <limits>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "extend_network.h"
#include "network.h"
#include "network_utils.h"

DEFINE_bool(symmetric, false, "Build symmetric networks.");
DEFINE_string(input_path, "", "Path to the input file.");
DEFINE_string(output_path, "", "Path to the output file.");
DEFINE_int32(keep_best_count, std::numeric_limits<int>::max(),
             "Number of networks to keep after adding each comparator. "
             "Default is to keep all networks.");
DEFINE_int32(jobs, std::thread::hardware_concurrency(),
             "The number of workers to use for parallel processing.");

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "/tmp";
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  CHECK_EQ(argc, 1);

  CHECK(!FLAGS_output_path.empty());
  CHECK(!FLAGS_input_path.empty());

  std::vector<Network> networks = LoadFromProtoFile(FLAGS_input_path);
  CHECK(!networks.empty());

  std::mt19937 gen;
  int n = networks[0].n;
  int num_layers = networks[0].layers.size();
  LOG(INFO) << "Add one layer on " << num_layers << " layers";

  for (auto &network : networks) {
    if (FLAGS_symmetric) {
      CHECK(network.IsSymmetric());
    }
    CHECK_EQ(network.n, n);
    CHECK_EQ(network.layers.size(), num_layers);
    network.AddEmptyLayer();
  }
  for (int num_comps = 0; num_comps < n / 2; ++num_comps) {
    LOG(INFO) << "Add one comparator on " << num_comps << " comparators";
    networks = ExtendNetwork(n, networks, FLAGS_symmetric, true,
                             FLAGS_keep_best_count, FLAGS_jobs, &gen);
    LOG(INFO) << "After cleanup: networks.size()=" << networks.size();
  }

  LOG(INFO) << "Saving " << networks.size() << " networks to "
            << FLAGS_output_path;
  SaveToProtoFile(networks, FLAGS_output_path);

  return 0;
}
