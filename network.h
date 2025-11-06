#pragma once

#include <string>
#include <vector>

#include "comparator.h"
#include "network.pb.h"
#include "output_type.h"

// Represents a single layer of comparators in a sorting network.
// In a layer, each channel can connect to at most one comparator.
struct Layer {
  // Constructs an empty layer for n channels.
  Layer(int n) : matching(n, -1) {}
  // Creates a Layer from a protocol buffer representation.
  static Layer FromProto(const pb::Layer &layer_proto);
  // Converts the layer to a protocol buffer representation.
  pb::Layer ToProto() const;

  // Returns the number of channels in this layer.
  int n() const { return matching.size(); }
  // Returns a string representation in bracket format, e.g., "[(0,2),(1,3)]".
  std::string ToString() const;
  // Returns true if the layer contains no comparators.
  bool IsEmpty() const;
  bool operator==(const Layer &other) const = default;

  // matching[i] = j means that there is a comparator between i and j.
  // matching[i] = -1 means that the channel i is not matched.
  // Note: If matching[i] = j, then matching[j] = i (bidirectional).
  std::vector<int> matching;
};

// Represents a sorting network: a sequence of layers, each containing
// comparators. A sorting network sorts any input by applying comparators in
// each layer sequentially.
struct Network {
  // Constructs a network with n channels and num_layers empty layers.
  Network(int n, int num_layers) : n(n), layers(num_layers, Layer(n)) {}
  // Creates a Network from a protocol buffer representation.
  static Network FromProto(const pb::Network &network_proto);
  // Converts the network to a protocol buffer representation.
  pb::Network ToProto() const;
  // Returns a string representation of the network.
  // If one_line is true, formats all layers on a single line.
  std::string ToString(bool one_line = false) const;
  // Returns the total number of comparators in the network.
  int Size() const;
  // Returns true if the network is symmetric under channel reflection:
  // reflection maps channel i to channel (n-1-i), and the network structure
  // remains unchanged under this transformation.
  bool IsSymmetric() const;
  // Returns true if there exists an output where channel i has value 1 and
  // channel j has value 0, meaning a comparator (i, j) would be useful.
  // Requires: i < j
  bool HasInverse(int i, int j) const;
  // Adds a comparator to the last layer.
  // Also updates the outputs to reflect the effect of the new comparator.
  void AddComparator(const Comparator &comparator);
  // Adds a new empty layer to the network.
  void AddEmptyLayer();
  bool operator==(const Network &other) const = default;
  // Returns true if the network is a complete sorting network,
  // i.e., it correctly sorts all possible inputs (outputs contain exactly
  // the n+1 sorted binary states).
  bool IsASortingNetwork() const;
  // Permutes the input channels according to the given permutation.
  // The permutation perm[i] = j means channel i is moved to position j.
  // Returns a new network with permuted channels.
  Network PermuteInputChannels(std::vector<int> perm) const;

  int n = 0;
  std::vector<Layer> layers;
  std::vector<OutputType> outputs; // may be empty if not computed
};
