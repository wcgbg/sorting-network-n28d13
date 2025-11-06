#include "simplify.h"

#include <vector>

#include "comparator.h"
#include "network_utils.h"

Network Simplify(Network network) {
  // Removes redundant comparators by rebuilding the network layer by layer,
  // only keeping comparators that actually change the output set.
  if (network.layers.empty()) {
    return network;
  }

  int n = network.n;
  Network new_network(n, 0);
  // The first layer is never redundant (no previous comparators to compare
  // against).
  new_network.layers.push_back(network.layers[0]);
  new_network.outputs = NetworkOutputs(new_network);

  // Process each subsequent layer
  for (int d = 1; d < network.layers.size(); d++) {
    new_network.AddEmptyLayer();
    // Only add comparators that are not redundant (i.e., that change outputs)
    for (int i = 0; i < n; i++) {
      int j = network.layers[d].matching[i];
      if (j > i) {
        // Check if this comparator would actually change the output set
        if (new_network.HasInverse(i, j)) {
          new_network.AddComparator(Comparator(i, j));
        }
        // Otherwise, skip this redundant comparator
      }
    }
  }

  return new_network;
}