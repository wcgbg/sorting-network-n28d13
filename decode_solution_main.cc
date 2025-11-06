#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/iostreams/device/file.hpp"
#include "boost/iostreams/filter/gzip.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "comparator.h"
#include "math_utils.h"
#include "network.h"
#include "network_utils.h"
#include "output_type.h"
#include "simplify.h"

DEFINE_bool(symmetric, false, "");
DEFINE_string(prefix_file, "", "The prefix file.");
DEFINE_string(permuted_prefix_file, "", "The permuted prefix file.");
DEFINE_string(cnf_dir, "", "The cnf directory.");
DEFINE_string(permutation_file, "", "The permutation file.");
DEFINE_string(output_pb_path, "", "The output sorting network protobuf file.");
DEFINE_string(output_bracket_path, "",
              "The output sorting network bracket file.");
DEFINE_bool(simplify, false, "Simplify the network.");

std::unordered_map<int, std::array<int, 3>>
ParseCnfVariables(std::istream &in) {
  std::unordered_map<int, std::array<int, 3>> var_to_comparator;
  std::string line;
  while (std::getline(in, line)) {
    if (line.starts_with("p cnf ")) {
      break;
    }
    // c var 1267 : g_6_3_16
    std::regex regex("c var (\\d+) : g_(\\d+)_(\\d+)_(\\d+)");
    std::smatch match;
    if (std::regex_match(line, match, regex)) {
      int var_idx = std::stoi(match[1]);
      int layer_idx = std::stoi(match[2]);
      int i = std::stoi(match[3]);
      int j = std::stoi(match[4]);
      CHECK_LT(i, j);
      var_to_comparator[var_idx] = {layer_idx, i, j};
    }
  }
  CHECK(!var_to_comparator.empty());
  return var_to_comparator;
}

std::unordered_map<int, std::array<int, 3>>
ParseCnfVariables(const std::string &cnf_file) {
  if (cnf_file.ends_with(".gz")) {
    // Use Boost Iostreams with gzip decompression
    boost::iostreams::filtering_istream in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(boost::iostreams::file_source(cnf_file));
    CHECK(in.good()) << "Failed to open gzip file: " << cnf_file;
    return ParseCnfVariables(in);
  } else {
    // Use regular file input
    std::ifstream in(cnf_file);
    CHECK(in.is_open()) << "Failed to open file: " << cnf_file;
    return ParseCnfVariables(in);
  }
}

std::vector<int> ParseSolution(const std::string &solution_file) {
  std::ifstream ifstream(solution_file);
  std::string line;
  CHECK(std::getline(ifstream, line));
  if (line == "UNSAT") {
    return {};
  }
  CHECK_EQ(line, "SAT");
  int x = 0;
  std::vector<int> solution;
  while (ifstream >> x) {
    solution.push_back(x);
  }
  return solution;
}

std::vector<std::vector<int>>
ParsePermutationFile(const std::string &permutation_file) {
  std::ifstream ifstream(permutation_file);
  CHECK(ifstream.is_open());
  std::vector<std::vector<int>> permutations;
  std::string line;
  while (std::getline(ifstream, line)) {
    std::vector<int> permutation;
    std::istringstream iss(line);
    int x = -1;
    while (iss >> x) {
      permutation.push_back(x);
    }
    permutations.push_back(permutation);
  }
  return permutations;
}

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "/tmp";
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  CHECK_EQ(argc, 1);

  CHECK(!FLAGS_prefix_file.empty());
  CHECK(!FLAGS_cnf_dir.empty());
  CHECK(!FLAGS_permutation_file.empty());
  CHECK(FLAGS_symmetric);

  std::vector<Network> prefixes = LoadFromProtoFile(FLAGS_prefix_file);
  std::vector<Network> permuted_prefixes;
  if (!FLAGS_permuted_prefix_file.empty()) {
    permuted_prefixes = LoadFromProtoFile(FLAGS_permuted_prefix_file);
    CHECK_EQ(prefixes.size(), permuted_prefixes.size());
  }
  std::vector<std::vector<int>> permutations =
      ParsePermutationFile(FLAGS_permutation_file);
  std::vector<Network> output_networks;

  std::vector<std::filesystem::path> sol_paths;
  for (const auto &entry : std::filesystem::directory_iterator(FLAGS_cnf_dir)) {
    if (entry.path().extension() != ".sol") {
      continue;
    }
    sol_paths.push_back(entry.path());
  }
  std::sort(sol_paths.begin(), sol_paths.end());
  for (const auto &sol_path : sol_paths) {
    LOG(INFO) << "Parsing solution file: " << sol_path.string();
    std::vector<int> solution = ParseSolution(sol_path.string());
    if (solution.empty()) {
      continue;
    }
    int index = std::stoi(sol_path.stem().string());
    Network network = prefixes.at(index);
    int prefix_depth = network.layers.size();
    int n = network.n;
    LOG(INFO) << "n=" << n << ", prefix_depth=" << prefix_depth;
    std::filesystem::path cnf_path = sol_path;
    cnf_path.replace_extension(".cnf");
    if (!std::filesystem::exists(cnf_path)) {
      cnf_path.replace_extension(".cnf.gz");
    }
    LOG(INFO) << "Parsing CNF variables: " << cnf_path.string();
    std::unordered_map<int, std::array<int, 3>> var_to_comparator =
        ParseCnfVariables(cnf_path.string());

    LOG(INFO) << "Building suffix network";
    Network suffix = Network(n, 0);
    for (int x : solution) {
      if (x <= 0) {
        continue;
      }
      auto it = var_to_comparator.find(x);
      if (it == var_to_comparator.end()) {
        continue;
      }
      auto [cnf_layer_idx, i, j] = it->second;
      if (cnf_layer_idx >= suffix.layers.size()) {
        suffix.AddEmptyLayer();
      }
      CHECK_EQ(cnf_layer_idx + 1, suffix.layers.size());
      suffix.AddComparator(Comparator(i, j));
      if (FLAGS_symmetric) {
        if (i + j != n - 1) {
          suffix.AddComparator(Comparator(n - 1 - j, n - 1 - i));
        }
      }
    }

    if (!permuted_prefixes.empty()) {
      LOG(INFO) << "Verifying permuted solution";
      Network permuted_network = permuted_prefixes.at(index);
      for (const Layer &layer : suffix.layers) {
        permuted_network.AddEmptyLayer();
        for (int i = 0; i < n; i++) {
          int j = layer.matching[i];
          if (j > i) {
            permuted_network.AddComparator(Comparator(i, j));
          }
        }
      }
      CHECK(permuted_network.IsASortingNetwork());
    }

    LOG(INFO) << "Permuting input channels";
    suffix =
        suffix.PermuteInputChannels(InversePermutation(permutations.at(index)));
    LOG(INFO) << "Concatenating suffix and prefix";
    network.layers.insert(network.layers.end(), suffix.layers.begin(),
                          suffix.layers.end());
    network.outputs.clear();
    LOG(INFO) << "Verifying";
    network.outputs = NetworkOutputs(network);

    if (FLAGS_symmetric) {
      CHECK(network.IsSymmetric());
      CHECK(IsSymmetric(n, network.outputs));
    }
    if (!network.IsASortingNetwork()) {
      LOG(ERROR) << "Outputs:";
      for (auto output : network.outputs) {
        LOG(ERROR) << ToBinaryString(n, output);
      }
      LOG(FATAL) << "Network is not a sorting network.\n" << network.ToString();
    }

    LOG(INFO) << "Simplifying";
    if (FLAGS_simplify) {
      network = Simplify(std::move(network));
      CHECK(network.IsASortingNetwork());
      if (FLAGS_symmetric) {
        CHECK(network.IsSymmetric());
        CHECK(IsSymmetric(n, network.outputs));
      }
    }
    LOG(INFO) << "Network:\n" << network.ToString();
    output_networks.push_back(std::move(network));
  }

  if (!FLAGS_output_pb_path.empty()) {
    SaveToProtoFile(output_networks, FLAGS_output_pb_path);
  }
  if (!FLAGS_output_bracket_path.empty()) {
    SaveToBracketFile(output_networks, FLAGS_output_bracket_path);
  }

  return 0;
}