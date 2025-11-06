#pragma once

#include <random>
#include <vector>

#include "network.h"

// Extends a collection of networks by adding comparators to the last layer.
// add_one_comparator: If true, adds exactly one comparator per network;
//                     if false, adds all possible comparators per network;
std::vector<Network> ExtendNetwork(int n, const std::vector<Network> &networks,
                                   bool symmetric, bool add_one_comparator,
                                   int keep_best_count, int jobs,
                                   std::mt19937 *gen);
