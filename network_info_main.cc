#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "network.h"
#include "network_utils.h"

DEFINE_int32(n, 0, "The number of channels.");
DEFINE_string(pb_path, "", "The input file in protobuf format.");
DEFINE_string(bracket_path, "", "The input file in bracket format.");
DEFINE_int32(prefix_depth, std::numeric_limits<int>::max(),
             "Take the prefix of the network up to this depth.");

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "/tmp";
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  CHECK_EQ(argc, 1);

  CHECK(!FLAGS_pb_path.empty() != !FLAGS_bracket_path.empty())
      << "Only one of pb_path or bracket_path must be specified.";

  std::vector<Network> networks;
  if (!FLAGS_pb_path.empty()) {
    networks = LoadFromProtoFile(FLAGS_pb_path, FLAGS_n);
  } else {
    networks = LoadFromBracketFile(FLAGS_n, FLAGS_bracket_path);
  }
  for (int i = 0; i < networks.size(); i++) {
    if (networks[i].layers.size() > FLAGS_prefix_depth) {
      networks[i].layers.erase(networks[i].layers.begin() + FLAGS_prefix_depth,
                               networks[i].layers.end());
      networks[i].outputs.clear();
      networks[i].outputs = NetworkOutputs(networks[i]);
    }
    std::cout << "i=" << i << std::endl;
    std::cout << "Network: " << networks[i].ToString() << std::endl;
    std::cout << "Is symmetric: " << networks[i].IsSymmetric() << std::endl;
    std::cout << "Is sorting network: " << networks[i].IsASortingNetwork()
              << std::endl;
    std::cout << std::endl;
  }

  return 0;
}
