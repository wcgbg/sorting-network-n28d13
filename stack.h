#pragma once

#include "network.h"

// Stack two networks together.
//
// In symmetric mode:
//   - For network_a: channels < net_a.n/2 stay the same, channels >= net_a.n/2
//   shift by +net_b.n
//   - For network_b: all channels shift by +net_a.n/2
//   - Result has net_a.n + net_b.n channels
//
// In non-symmetric mode:
//   - network_a stays in channels [0, net_a.n)
//   - network_b shifts to channels [net_a.n, net_a.n+net_b.n)
//   - Result has net_a.n + net_b.n channels
Network StackNetworks(const Network &net_a, const Network &net_b,
                      bool symmetric);
