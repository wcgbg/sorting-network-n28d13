#include "clean_up.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "glog/logging.h"

#include "network_utils.h"

std::vector<Network> CleanUp(std::vector<Network> networks, bool symmetric,
                             int keep_best_count, std::mt19937 *gen) {
  if (networks.empty()) {
    return networks;
  }
  CHECK_GT(keep_best_count, 0);
  if (keep_best_count >= networks.size()) {
    networks =
        RemoveRedundantNetworks(std::move(networks), symmetric, false, gen);
    return networks;
  }
  networks = RemoveRedundantNetworks(std::move(networks), symmetric, true, gen);
  // Only keep good networks. It reduces the computational cost of the following
  // RemoveRedundantNetworks call.
  constexpr double kPreFilterFactor = 2.0;
  int filtered_num_networks =
      static_cast<int>(std::ceil(keep_best_count * kPreFilterFactor));
  while (true) {
    bool is_filtered = false;
    std::vector<Network> filtered_networks;
    if (networks.size() > filtered_num_networks) {
      filtered_networks = std::vector<Network>(
          networks.begin(), networks.begin() + filtered_num_networks);
      is_filtered = true;
    } else {
      filtered_networks = networks;
    }
    filtered_networks = RemoveRedundantNetworks(std::move(filtered_networks),
                                                symmetric, false, gen);
    CHECK(std::is_sorted(filtered_networks.begin(), filtered_networks.end(),
                         [](const Network &a, const Network &b) {
                           return a.outputs.size() < b.outputs.size();
                         }));
    if (!is_filtered ||
        (filtered_networks.size() > keep_best_count &&
         filtered_networks.back().outputs.size() >
             filtered_networks.at(keep_best_count - 1).outputs.size())) {
      int best_count_threshold =
          filtered_networks
              .at(std::min<int>(keep_best_count, filtered_networks.size()) - 1)
              .outputs.size();
      while (filtered_networks.back().outputs.size() > best_count_threshold) {
        filtered_networks.pop_back();
      }
      return filtered_networks;
    }
    // If we filtered too aggressively, increase the filter size.
    // Factor of 1.5 provides gradual expansion without being too aggressive.
    constexpr double kRedundantFactor = 1.5;
    filtered_num_networks = static_cast<int>(
        std::ceil(kRedundantFactor * filtered_num_networks *
                  std::max<int>(keep_best_count, filtered_networks.size()) /
                  filtered_networks.size()));
    LOG(INFO) << "Increasing filtered_num_networks to " << filtered_num_networks
              << " and retrying";
  }
}
