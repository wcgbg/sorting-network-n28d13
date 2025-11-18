#pragma once

#include <random>
#include <string>
#include <vector>

#include "network.h"
#include "output_type.h"

std::vector<OutputType> NetworkOutputs(const Network &network);

std::vector<std::vector<OutputType>>
NetworkOutputs(const std::vector<Network> &networks);

// A network in bracket format is like this:
// [(0,2),(1,3)],[(0,1),(2,3)],[(1,2)]
// A bracket file is a file that contains multiple networks in bracket format,
// one network per line.
// The lines starting with '#' are ignored.
std::vector<Network> LoadFromBracketFile(int n, const std::string &filename,
                                         bool fill_outputs = true);
void SaveToBracketFile(const std::vector<Network> &networks,
                       const std::string &filename);

std::vector<Network> LoadFromProtoFile(const std::string &filename, int n = 0);
void SaveToProtoFile(const std::vector<Network> &networks,
                     const std::string &filename);

std::vector<Network> RemoveRedundantNetworks(std::vector<Network> networks,
                                             bool symmetric, bool fast,
                                             std::mt19937 *gen);

std::vector<Network> CreateFirstLayer(int n, bool symmetric);
