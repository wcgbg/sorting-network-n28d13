#include "extend_network.h"

#include <atomic>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "glog/logging.h"

#include "clean_up.h"
#include "comparator.h"
#include "output_type.h"

namespace {
void AddComparator(const Network &network, bool symmetric,
                   const std::vector<std::vector<bool>> &has_inverse, int i0,
                   int remaining_dfs_depth,
                   std::vector<Network> *extended_networks) {
  CHECK_NOTNULL(extended_networks);

  int n = network.n;
  const Layer &extended_layer = network.layers.back();

  if (symmetric) {
    CHECK(network.IsSymmetric());
  }

  extended_networks->push_back(network);

  if (remaining_dfs_depth == 0) {
    return;
  }

  for (int i = i0; i < n; i++) {
    if (extended_layer.matching[i] != -1) {
      continue;
    }
    if (symmetric && extended_layer.matching[n - 1 - i] != -1) {
      continue;
    }
    for (int j = i + 1; j < n; j++) {
      if (extended_layer.matching[j] != -1) {
        continue;
      }
      if (symmetric && n - 1 - j < i0) {
        continue;
      }
      if (symmetric && extended_layer.matching[n - 1 - j] != -1) {
        continue;
      }
      if (!has_inverse[i][j]) {
        continue;
      }
      if (symmetric && !has_inverse[n - 1 - j][n - 1 - i]) {
        continue;
      }
      Network new_network = network;
      new_network.AddComparator(Comparator(i, j));
      if (symmetric) {
        if (i + j != n - 1) {
          new_network.AddComparator(Comparator(n - 1 - j, n - 1 - i));
        }
      }
      std::vector<std::vector<bool>> new_has_inverse = has_inverse;
      for (int k = 0; k < n; k++) {
        if (k < i) {
          new_has_inverse[k][i] = new_network.HasInverse(k, i);
          if (symmetric) {
            new_has_inverse[n - 1 - i][n - 1 - k] = new_has_inverse[k][i];
          }
        }
        if (k > i) {
          new_has_inverse[i][k] = new_network.HasInverse(i, k);
          if (symmetric) {
            new_has_inverse[n - 1 - k][n - 1 - i] = new_has_inverse[i][k];
          }
        }
        if (k < j) {
          new_has_inverse[k][j] = new_network.HasInverse(k, j);
          if (symmetric) {
            new_has_inverse[n - 1 - j][n - 1 - k] = new_has_inverse[k][j];
          }
        }
        if (k > j) {
          new_has_inverse[j][k] = new_network.HasInverse(j, k);
          if (symmetric) {
            new_has_inverse[n - 1 - k][n - 1 - j] = new_has_inverse[j][k];
          }
        }
      }
      AddComparator(new_network, symmetric, new_has_inverse, i + 1,
                    remaining_dfs_depth - 1, extended_networks);
    }
  }
}

void ProcessPrefixWorker(const Network &network, int n, bool symmetric,
                         bool add_one_comparator,
                         std::vector<Network> *extended_networks,
                         std::mutex *output_mutex) {
  if (symmetric) {
    CHECK_EQ(n % 2, 0);
    CHECK(network.IsSymmetric());
    CHECK(IsSymmetric(n, network.outputs));
  }

  CHECK(!network.outputs.empty());
  // network.layers.push_back(Layer(n));

  std::vector<std::vector<bool>> has_inverse(n, std::vector<bool>(n));
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      has_inverse[i][j] = network.HasInverse(i, j);
      if (symmetric) {
        int mirror_i = n - 1 - i;
        int mirror_j = n - 1 - j;
        if (mirror_j < i) {
          CHECK_EQ(has_inverse[i][j], has_inverse[mirror_j][mirror_i])
              << "i=" << i << ", j=" << j << ", mirror_j=" << mirror_j
              << ", mirror_i=" << mirror_i;
        }
      }
    }
  }
  std::vector<Network> local_extended_networks;
  int remaining_dfs_depth =
      add_one_comparator ? 1 : std::numeric_limits<int>::max();
  AddComparator(network, symmetric, has_inverse, 0, remaining_dfs_depth,
                &local_extended_networks);
  std::lock_guard<std::mutex> lock(*output_mutex);
  for (const auto &extended_network : local_extended_networks) {
    extended_networks->push_back(std::move(extended_network));
  }
}

} // namespace

std::vector<Network> ExtendNetwork(int n, const std::vector<Network> &networks,
                                   bool symmetric, bool add_one_comparator,
                                   int keep_best_count, int jobs,
                                   std::mt19937 *gen) {
  // Parallel processing setup
  std::vector<std::thread> workers;
  std::vector<Network> extended_networks;
  std::mutex output_mutex;
  std::atomic<int> next_network_idx(0);

  LOG(INFO) << "Processing " << networks.size() << " networks using " << jobs
            << " workers";

  // Create worker workers
  for (int w = 0; w < jobs; w++) {
    workers.emplace_back([&]() {
      while (true) {
        int network_idx = next_network_idx.fetch_add(1);
        if (network_idx >= networks.size()) {
          break;
        }
        ProcessPrefixWorker(networks[network_idx], n, symmetric,
                            add_one_comparator, &extended_networks,
                            &output_mutex);
      }
    });
  }

  // Wait for all workers to complete
  for (auto &worker : workers) {
    worker.join();
  }
  LOG(INFO) << "Extended " << extended_networks.size() << " networks";

  extended_networks =
      CleanUp(std::move(extended_networks), symmetric, keep_best_count, gen);

  return extended_networks;
}
