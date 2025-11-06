#include <string>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "network.h"
#include "network_utils.h"

DEFINE_int32(n, 0, "The number of channels in the network.");
DEFINE_string(bracket_path, "", "The path to a network in bracket format.");
DEFINE_string(pb_path, "", "The path to a network in protobuf format.");
DEFINE_bool(bracket_to_pb, false, "Convert from bracket to protobuf.");
DEFINE_bool(pb_to_bracket, false, "Convert from protobuf to bracket.");

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "/tmp";
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  CHECK_EQ(argc, 1);

  CHECK_GT(FLAGS_n, 0) << "The number of channels must be positive.";
  CHECK(!FLAGS_bracket_path.empty())
      << "The path to the bracket file must be specified.";
  CHECK(!FLAGS_pb_path.empty())
      << "The path to the protobuf file must be specified.";
  CHECK_NE(FLAGS_bracket_to_pb, FLAGS_pb_to_bracket)
      << "Only one of bracket_to_pb or pb_to_bracket must be specified.";

  if (FLAGS_pb_to_bracket) {
    // Convert from protobuf to bracket
    LOG(INFO) << "Loading networks from protobuf file: " << FLAGS_pb_path;
    std::vector<Network> networks = LoadFromProtoFile(FLAGS_pb_path, FLAGS_n);
    LOG(INFO) << "Loaded " << networks.size() << " networks.";

    LOG(INFO) << "Saving networks to bracket file: " << FLAGS_bracket_path;
    SaveToBracketFile(networks, FLAGS_bracket_path);
    LOG(INFO) << "Conversion complete.";
  } else {
    // Convert from bracket to protobuf
    LOG(INFO) << "Loading networks from bracket file: " << FLAGS_bracket_path;
    std::vector<Network> networks =
        LoadFromBracketFile(FLAGS_n, FLAGS_bracket_path);
    LOG(INFO) << "Loaded " << networks.size() << " networks.";

    LOG(INFO) << "Saving networks to protobuf file: " << FLAGS_pb_path;
    SaveToProtoFile(networks, FLAGS_pb_path);
    LOG(INFO) << "Conversion complete.";
  }
  return 0;
}
