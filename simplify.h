#pragma once

#include "network.h"

// Removes redundant comparators from a network.
// A comparator is redundant if its first input is always less than the second
// input. This preserves the network's functionality while potentially reducing
// its size.
Network Simplify(Network network);
