#include "mask_library.h"

#include <bit>
#include <iostream>
#include <mutex>
#include <utility>

#include "glog/logging.h"

#include "output_type.h"

std::map<int, MaskLibrary> MaskLibrary::n_to_instance_;

MaskLibrary::MaskLibrary(int n, bool full_size) : n_(n) {
  LOG(INFO) << "Creating mask library for n=" << n
            << ", full_size=" << full_size;
  CHECK_GT(n, 0);
  CHECK_LT(n, sizeof(OutputType) * 8);
  mask0_.resize(n, BitSet(OutputType(1) << n));
  mask1_.resize(n, BitSet(OutputType(1) << n));
  for (int i = 0; i < n; ++i) {
    for (OutputType x = 0; x < (OutputType(1) << n); x++) {
      if (x & (OutputType(1) << i)) {
        mask1_[i].set(x);
      }
    }
    mask0_[i] = ~mask1_[i];
  }
  mask10_.resize(n);
  for (int i = 0; i < n; ++i) {
    mask10_[i].resize(n);
    for (int j = 0; j < n; ++j) {
      if (full_size || i < j) {
        mask10_[i][j] = mask1_[i] & mask0_[j];
      }
    }
  }

  mask_by_weight_.resize(n + 1, BitSet(OutputType(1) << n));
  for (OutputType x = 0; x < (OutputType(1) << n); x++) {
    mask_by_weight_.at(std::popcount(x)).set(x);
  }
}

const MaskLibrary &MaskLibrary::GetInstance(int n) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);
  auto it = n_to_instance_.find(n);
  if (it == n_to_instance_.end()) {
    it = n_to_instance_.emplace(n, MaskLibrary(n, false)).first;
  }
  CHECK_EQ(it->second.n_, n);
  return it->second;
}
