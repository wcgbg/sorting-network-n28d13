#include "network_utils.h"

#include <algorithm>
#include <atomic>
#include <format>
#include <fstream>
#include <limits>
#include <sstream>
#include <stddef.h>
#include <thread>
#include <utility>
#include <vector>

#include "boost/algorithm/string.hpp"
#include "glog/logging.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"

#include "isomorphism.h"
#include "network.h"
#include "network.pb.h"
#include "output_bitset.h"
#include "output_type.h"

std::vector<OutputType> NetworkOutputs(const Network &network) {
  if (!network.outputs.empty()) {
    return network.outputs;
  }
  int n = network.n;
  OutputBitset output_bitset(n);
  for (const auto &layer : network.layers) {
    for (int i = 0; i < n; i++) {
      int j = layer.matching[i];
      if (j > i) {
        output_bitset.AddComparator(i, j);
      }
    }
  }
  return output_bitset.ToSparse();
}

std::vector<std::vector<OutputType>>
NetworkOutputs(const std::vector<Network> &networks) {
  LOG(INFO) << "Computing outputs for " << networks.size() << " networks";
  std::vector<std::vector<OutputType>> outputs;
  outputs.reserve(networks.size());
  for (const auto &network : networks) {
    outputs.push_back(NetworkOutputs(network));
  }
  return outputs;
}

namespace {
void FillOutputsInParallel(std::vector<Network> &networks,
                           int fill_outputs_if_size_is_smaller_than) {
  if (fill_outputs_if_size_is_smaller_than <= 0) {
    return;
  }
  int num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0) {
    num_threads = 4;
  }
  LOG(INFO) << "Filling " << networks.size() << " outputs in parallel with "
            << num_threads << " threads";
  std::vector<std::thread> threads;
  std::atomic<int> next_network_idx(0);
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([&]() {
      while (true) {
        int network_idx = next_network_idx.fetch_add(1);
        if (network_idx >= networks.size()) {
          break;
        }
        std::string message = std::format("Filling output for network {} / {}",
                                          network_idx, networks.size());
        if (networks.size() < 100) {
          LOG(INFO) << message;
        } else if (networks.size() < 1000) {
          LOG_EVERY_N(INFO, 10) << message;
        } else if (networks.size() < 10000) {
          LOG_EVERY_N(INFO, 100) << message;
        } else if (networks.size() < 100000) {
          LOG_EVERY_N(INFO, 1000) << message;
        } else {
          LOG_EVERY_N(INFO, 10000) << message;
        }
        std::vector<OutputType> outputs = NetworkOutputs(networks[network_idx]);
        if (outputs.size() < fill_outputs_if_size_is_smaller_than) {
          networks[network_idx].outputs = std::move(outputs);
        }
      }
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }
}
} // namespace

std::vector<Network> LoadFromBracketFile(int n, const std::string &filename,
                                         bool fill_outputs) {
  std::ifstream file(filename);
  CHECK(file.is_open()) << std::format("Failed to open file: {}", filename);

  std::vector<Network> networks;
  std::string line;
  int line_number = 0;

  while (std::getline(file, line)) {
    line_number++;

    boost::algorithm::trim(line);
    if (line.empty()) {
      continue; // Empty line after trimming
    }

    // Skip comment lines
    if (line[0] == '#') {
      continue;
    }

    // Parse layers from bracket format: [(0,2),(1,3)],[(0,1),(2,3)],[(1,2)]
    std::vector<Layer> layers;
    size_t pos = 0;

    while (pos < line.size()) {
      // Skip non-bracket characters (commas, spaces)
      while (pos < line.size() && line[pos] != '[') {
        pos++;
      }
      if (pos >= line.size()) {
        break;
      }

      // Found a layer starting with '['
      pos++; // Skip '['
      Layer layer(n);

      // Parse comparators within this layer
      while (pos < line.size() && line[pos] != ']') {
        // Skip whitespace and commas
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == ',')) {
          pos++;
        }

        if (pos >= line.size() || line[pos] == ']') {
          break;
        }

        // Expect '('
        CHECK_EQ(line[pos], '(')
            << std::format("Line {}: Expected '(' at position {} in line: {}",
                           line_number, pos, line);
        pos++;

        // Parse first number
        size_t comma_pos = line.find(',', pos);
        CHECK_NE(comma_pos, std::string::npos)
            << std::format("Line {}: Expected ',' after first number in: {}",
                           line_number, line);
        int i = std::stoi(line.substr(pos, comma_pos - pos));
        pos = comma_pos + 1;

        // Parse second number
        size_t close_paren = line.find(')', pos);
        CHECK_NE(close_paren, std::string::npos)
            << std::format("Line {}: Expected ')' after second number in: {}",
                           line_number, line);
        int j = std::stoi(line.substr(pos, close_paren - pos));
        pos = close_paren + 1;

        // Validate and add comparator to layer
        CHECK_GE(i, 0) << std::format("Line {}: Invalid comparator index: {}",
                                      line_number, i);
        CHECK_GE(j, 0) << std::format("Line {}: Invalid comparator index: {}",
                                      line_number, j);
        CHECK_LT(i, n) << std::format("Line {}: Comparator index {} >= n={}",
                                      line_number, i, n);
        CHECK_LT(j, n) << std::format("Line {}: Comparator index {} >= n={}",
                                      line_number, j, n);
        CHECK_NE(i, j) << std::format(
            "Line {}: Comparator indices must be different: ({},{})",
            line_number, i, j);
        CHECK_EQ(layer.matching[i], -1) << std::format(
            "Line {}: Channel {} already matched in layer", line_number, i);
        CHECK_EQ(layer.matching[j], -1) << std::format(
            "Line {}: Channel {} already matched in layer", line_number, j);

        layer.matching[i] = j;
        layer.matching[j] = i;
      }

      // Skip ']'
      if (pos < line.size() && line[pos] == ']') {
        pos++;
      }

      layers.push_back(std::move(layer));
    }

    if (!layers.empty()) {
      Network network(n, layers.size());
      network.layers = std::move(layers);
      networks.push_back(std::move(network));
    }
  }

  if (fill_outputs) {
    FillOutputsInParallel(networks, std::numeric_limits<int>::max());
  }

  return networks;
}

void SaveToBracketFile(const std::vector<Network> &networks,
                       const std::string &filename) {
  std::ofstream file(filename);
  CHECK(file.is_open()) << std::format("Failed to open file: {}", filename);
  for (const auto &network : networks) {
    file << network.ToString(true);
  }
  file.close();
}

std::vector<Network> LoadFromProtoFile(const std::string &filename, int n) {
  pb::NetworkCollection network_collection_proto;
  if (filename.ends_with(".txt")) {
    // text format
    std::ifstream file(filename);
    CHECK(file.is_open());
    google::protobuf::io::IstreamInputStream zero_copy_stream(&file);
    CHECK(google::protobuf::TextFormat::Parse(&zero_copy_stream,
                                              &network_collection_proto));
  } else {
    // binary format
    std::ifstream file(filename, std::ios::binary);
    CHECK(file.is_open());
    CHECK(network_collection_proto.ParseFromIstream(&file));
  }
  std::vector<Network> networks;
  bool has_outputs = true;
  for (const auto &network_proto : network_collection_proto.network()) {
    if (n == 0) {
      n = network_proto.n();
    } else {
      CHECK_EQ(network_proto.n(), n);
    }
    Network network = Network::FromProto(network_proto);
    if (network.outputs.empty()) {
      has_outputs = false;
    }
    networks.push_back(std::move(network));
  }
  if (!has_outputs) {
    LOG(INFO) << "outputs field is missing. Creating mask library and filling "
                 "outputs in parallel...";
    FillOutputsInParallel(networks, std::numeric_limits<int>::max());
  }
  return networks;
}

void SaveToProtoFile(const std::vector<Network> &networks,
                     const std::string &filename) {
  pb::NetworkCollection network_collection_proto;
  for (const auto &network : networks) {
    *network_collection_proto.add_network() = network.ToProto();
  }
  if (filename.ends_with(".txt")) {
    // text format
    std::ofstream file(filename);
    CHECK(file.is_open());
    google::protobuf::io::OstreamOutputStream output_stream(&file);
    CHECK(google::protobuf::TextFormat::Print(network_collection_proto,
                                              &output_stream));
  } else if (filename.ends_with(".pb")) {
    // binary format
    std::ofstream file(filename, std::ios::binary);
    CHECK(file.is_open());
    CHECK(network_collection_proto.SerializeToOstream(&file));
  } else {
    LOG(FATAL) << "Unsupported file extension: " << filename;
  }
}

std::vector<Network> RemoveRedundantNetworks(std::vector<Network> networks,
                                             bool symmetric, bool fast,
                                             std::mt19937 *gen) {
  if (networks.size() <= 1) {
    return networks;
  }
  int n = networks.front().n;
  std::vector<std::vector<OutputType>> outputs = NetworkOutputs(networks);
  std::vector<bool> is_redundant =
      FindRedundantOutputs(n, outputs, fast, symmetric, gen);
  CHECK_EQ(is_redundant.size(), networks.size());
  std::vector<Network> non_redundant_networks;
  for (int i = 0; i < networks.size(); i++) {
    if (!is_redundant[i]) {
      non_redundant_networks.push_back(std::move(networks[i]));
    }
  }
  std::sort(non_redundant_networks.begin(), non_redundant_networks.end(),
            [](const Network &a, const Network &b) {
              return a.outputs.size() < b.outputs.size();
            });
  return non_redundant_networks;
}

void CheckRedundancy(const std::vector<Network> &networks, bool symmetric,
                     std::mt19937 *gen) {
  if (networks.size() <= 1) {
    return;
  }
  int n = networks.front().n;
  std::vector<std::vector<OutputType>> outputs_collection =
      NetworkOutputs(networks);
  std::vector<bool> is_redundant =
      FindRedundantOutputs(n, outputs_collection, false, symmetric, gen);
  int redundant_count =
      std::count(is_redundant.begin(), is_redundant.end(), true);
  if (redundant_count == 0) {
    return;
  }
  std::ostringstream oss;
  for (int i = 0; i < networks.size(); i++) {
    if (is_redundant[i]) {
      oss << i << std::endl;
    }
  }
  LOG(FATAL) << "Found redundant prefixes! " << oss.str();
}

std::vector<Network> CreateFirstLayer(int n, bool symmetric) {
  if (symmetric) {
    CHECK_EQ(n % 2, 0);
    std::vector<Network> networks;
    for (int k = 0; k <= n / 2 / 2; k++) {
      Layer layer(n);
      for (int i = 0; i < k; i++) {
        layer.matching[i * 2] = i * 2 + 1;
        layer.matching[i * 2 + 1] = i * 2;
        layer.matching[n - 1 - i * 2] = n - 1 - (i * 2 + 1);
        layer.matching[n - 1 - (i * 2 + 1)] = n - 1 - i * 2;
      }
      for (int i = k * 2; i < n / 2; i++) {
        layer.matching[i] = n - 1 - i;
        layer.matching[n - 1 - i] = i;
      }
      Network network(n, 1);
      network.layers[0] = std::move(layer);
      network.outputs = NetworkOutputs(network);
      networks.push_back(std::move(network));
    }
    return networks;
  } else {
    Layer layer(n);
    for (int i = 0; i + 1 < n; i += 2) {
      layer.matching[i] = i + 1;
      layer.matching[i + 1] = i;
    }
    Network network(n, 1);
    network.layers[0] = std::move(layer);
    network.outputs = NetworkOutputs(network);
    return {network};
  }
}
