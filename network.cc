#include "network.h"

#include <format>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "glog/logging.h"

#include "comparator.h"
#include "network.pb.h"
#include "output_type.h"

bool Layer::IsEmpty() const {
  for (int i = 0; i < matching.size(); i++) {
    if (matching[i] != -1) {
      return false;
    }
  }
  return true;
}

std::string Layer::ToString() const {
  std::string s;
  for (int i = 0; i < matching.size(); i++) {
    if (matching[i] > i) {
      s += std::format("({},{}),", i, matching[i]);
    }
  }
  if (!s.empty()) {
    s.pop_back();
  }
  return s;
}

pb::Layer Layer::ToProto() const {
  pb::Layer layer_proto;
  for (int i = 0; i < matching.size(); i++) {
    layer_proto.add_matching(matching[i]);
  }
  return layer_proto;
}

Layer Layer::FromProto(const pb::Layer &layer_proto) {
  Layer layer(layer_proto.matching_size());
  for (int i = 0; i < layer_proto.matching_size(); i++) {
    layer.matching[i] = layer_proto.matching(i);
  }
  return layer;
}

int Network::Size() const {
  int size = 0;
  for (const auto &layer : layers) {
    for (int i = 0; i < n; i++) {
      int j = layer.matching[i];
      if (j > i) {
        size++;
      }
    }
  }
  return size;
}

std::string Network::ToString(bool one_line) const {
  std::ostringstream oss;
  oss << std::format("# n={}, depth={}, size={}\n", n, layers.size(), Size());
  char separator = one_line ? ',' : '\n';
  for (int i = 0; i < layers.size(); i++) {
    oss << std::format("[{}]{}", layers[i].ToString(), separator);
  }
  std::string s = oss.str();
  if (one_line) {
    s.back() = '\n';
  }
  return s;
}

pb::Network Network::ToProto() const {
  pb::Network network_proto;
  network_proto.set_n(n);
  for (const auto &layer : layers) {
    *network_proto.add_layer() = layer.ToProto();
  }
  for (const auto &output : outputs) {
    network_proto.add_output(output);
  }
  return network_proto;
}

Network Network::FromProto(const pb::Network &network_proto) {
  Network network(network_proto.n(), network_proto.layer_size());
  for (int i = 0; i < network_proto.layer_size(); i++) {
    network.layers[i] = Layer::FromProto(network_proto.layer(i));
  }
  for (int i = 0; i < network_proto.output_size(); i++) {
    network.outputs.push_back(network_proto.output(i));
  }
  return network;
}

bool Network::IsSymmetric() const {
  for (const auto &layer : layers) {
    for (int i = 0; i < n; i++) {
      int reflected_i = n - 1 - i;
      int j = layer.matching[i];
      int reflected_j = j == -1 ? -1 : n - 1 - j;
      if (layer.matching[reflected_i] != reflected_j) {
        return false;
      }
    }
  }
  return true;
}

void Network::AddEmptyLayer() { layers.push_back(Layer(n)); }

void Network::AddComparator(const Comparator &comparator) {
  CHECK(layers.back().matching[comparator.i()] == -1);
  CHECK(layers.back().matching[comparator.j()] == -1);
  layers.back().matching[comparator.i()] = comparator.j();
  layers.back().matching[comparator.j()] = comparator.i();
  outputs = ::AddComparator(outputs, comparator.i(), comparator.j());
}

bool Network::HasInverse(int i, int j) const {
  CHECK_LT(i, j);
  for (OutputType x : outputs) {
    OutputType i_bit = (x >> i) & OutputType(1);
    OutputType j_bit = (x >> j) & OutputType(1);
    if (i_bit > j_bit) {
      return true;
    }
  }
  return false;
}

bool Network::IsASortingNetwork() const {
  if (outputs.size() != n + 1) {
    return false;
  }
  for (int i = 0; i <= n; i++) {
    if (outputs[i] != ((OutputType(1) << i) - OutputType(1)) << (n - i)) {
      return false;
    }
  }
  return true;
}

Network Network::PermuteInputChannels(std::vector<int> perm) const {
  CHECK_EQ(perm.size(), n);
  for (int i = 0; i < n; i++) {
    CHECK_GE(perm[i], 0);
    CHECK_LT(perm[i], n);
  }
  Network new_network(n, layers.size());
  for (int l = 0; l < layers.size(); l++) {
    for (int i = 0; i < n; i++) {
      int j = layers[l].matching[i];
      if (j > i) {
        int new_i = perm[i];
        int new_j = perm[j];
        if (new_i > new_j) {
          std::swap(new_i, new_j);
          std::swap(perm[i], perm[j]);
        }
        CHECK_EQ(new_network.layers[l].matching[new_i], -1);
        CHECK_EQ(new_network.layers[l].matching[new_j], -1);
        new_network.layers[l].matching[new_i] = new_j;
        new_network.layers[l].matching[new_j] = new_i;
      }
    }
  }
  return new_network;
}