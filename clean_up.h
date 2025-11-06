#pragma once

#include <random>
#include <vector>

#include "network.h"

// Removes redundant networks and keeps only the best ones (minimum number of
// outputs). A network is redundant if its outputs or the negation is isomorphic
// to the superset of another network's outputs.
std::vector<Network> CleanUp(std::vector<Network> networks, bool symmetric,
                             int keep_best_count, std::mt19937 *gen);
