#include "stack.h"

#include <algorithm>
#include <vector>

#include "glog/logging.h"

#include "network.h"
#include "output_type.h"

Network StackNetworks(const Network &net_a, const Network &net_b,
                      bool symmetric) {
  int n_a = net_a.n;
  int n_b = net_b.n;

  // Precompute channel mappings
  std::vector<int> perm_a(n_a);
  std::vector<int> perm_b(n_b);

  if (symmetric) {
    CHECK_EQ(n_a % 2, 0);
    CHECK_EQ(n_b % 2, 0);
    // For network_a in symmetric mode:
    // channels < n_a/2 stay the same
    // channels >= n_a/2 shift by +n_b
    for (int i = 0; i < n_a; i++) {
      perm_a[i] = (i < n_a / 2) ? i : i + n_b;
    }
    // For network_b: all channels shift by +n_a/2
    for (int i = 0; i < n_b; i++) {
      perm_b[i] = i + n_a / 2;
    }
  } else {
    // Non-symmetric mode: network_a stays the same
    for (int i = 0; i < n_a; i++) {
      perm_a[i] = i;
    }
    // network_b shifts by +n_a
    for (int i = 0; i < n_b; i++) {
      perm_b[i] = i + n_a;
    }
  }

  // Build the new network
  int n = n_a + n_b;
  Network result(n, std::max(net_a.layers.size(), net_b.layers.size()));

  for (int l = 0; l < result.layers.size(); l++) {
    if (l < net_a.layers.size()) {
      for (int i = 0; i < n_a; i++) {
        int j = net_a.layers[l].matching[i];
        if (j > i) {
          int new_i = perm_a[i];
          int new_j = perm_a[j];
          CHECK_EQ(result.layers[l].matching[new_i], -1);
          CHECK_EQ(result.layers[l].matching[new_j], -1);
          result.layers[l].matching[new_i] = new_j;
          result.layers[l].matching[new_j] = new_i;
        }
      }
    }
    if (l < net_b.layers.size()) {
      for (int i = 0; i < n_b; i++) {
        int j = net_b.layers[l].matching[i];
        if (j > i) {
          int new_i = perm_b[i];
          int new_j = perm_b[j];
          CHECK_EQ(result.layers[l].matching[new_i], -1);
          CHECK_EQ(result.layers[l].matching[new_j], -1);
          result.layers[l].matching[new_i] = new_j;
          result.layers[l].matching[new_j] = new_i;
        }
      }
    }
  }

  // Permute outputs from network_a
  std::vector<OutputType> permuted_outputs_a;
  for (const auto &output_a : net_a.outputs) {
    OutputType permuted = 0;
    for (int i = 0; i < n_a; i++) {
      if ((output_a >> i) & OutputType(1)) {
        permuted |= (OutputType(1) << perm_a[i]);
      }
    }
    permuted_outputs_a.push_back(permuted);
  }

  // Permute outputs from network_b
  std::vector<OutputType> permuted_outputs_b;
  for (const auto &output_b : net_b.outputs) {
    OutputType permuted = 0;
    for (int i = 0; i < n_b; i++) {
      if ((output_b >> i) & OutputType(1)) {
        permuted |= (OutputType(1) << perm_b[i]);
      }
    }
    permuted_outputs_b.push_back(permuted);
  }

  // Merge outputs: Cartesian product
  for (const auto &output_a : permuted_outputs_a) {
    for (const auto &output_b : permuted_outputs_b) {
      CHECK_EQ(output_a & output_b, 0);
      result.outputs.push_back(output_a | output_b);
    }
  }

  std::sort(result.outputs.begin(), result.outputs.end());

  return result;
}
