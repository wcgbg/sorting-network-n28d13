#include "isomorphism.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <format>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#include "glog/logging.h"

#include "math_utils.h"
#include "output_type.h"

namespace {

std::vector<OutputType>
SortByWeight(int n, const std::vector<OutputType> &set,
             const std::vector<uint64_t> &count_one_by_col,
             std::vector<int> *inv_perm) {
  CHECK_NOTNULL(inv_perm);
  std::stable_sort(inv_perm->begin(), inv_perm->end(), [&](int i, int j) {
    return count_one_by_col[i] < count_one_by_col[j];
  });
  std::vector<OutputType> set_perm;
  set_perm.reserve(set.size());
  for (OutputType x : set) {
    OutputType x_perm = 0;
    for (int i = 0; i < n; i++) {
      x_perm |= ((x >> (*inv_perm)[i]) & OutputType(1)) << i;
    }
    set_perm.push_back(x_perm);
  }
  std::sort(set_perm.begin(), set_perm.end());
  return set_perm;
}

bool IsIsomorphicToSubsetBacktrackingRecursive(
    int n, const std::vector<OutputType> &set_a,
    const std::vector<OutputType> &set_b, bool symmetric,
    const std::vector<std::vector<OutputType>> &set_b_pasts, int pos,
    std::vector<int> &perm, std::vector<bool> &used, std::mt19937 *gen) {

  OutputType past_mask = (OutputType(1) << pos) - 1; // bit 0..pos-1
  if (symmetric) {
    past_mask |= past_mask << (n - pos); // bit n-pos .. n-1
  }

  // Check the partial permutation perm[0..pos-1].
  std::vector<OutputType> set_a_past_inv_perm;
  set_a_past_inv_perm.reserve(set_a.size());
  for (OutputType a : set_a) {
    OutputType a_past_inv_perm = 0;
    for (int j = 0; j < pos; j++) {
      a_past_inv_perm |= ((a >> perm[j]) & OutputType(1)) << j;
    }
    if (symmetric) {
      for (int j = n - pos; j < n; j++) {
        a_past_inv_perm |= ((a >> perm[j]) & OutputType(1)) << j;
      }
    }
    set_a_past_inv_perm.push_back(a_past_inv_perm);
  }
  std::sort(set_a_past_inv_perm.begin(), set_a_past_inv_perm.end());
  if (!std::includes(set_b_pasts.at(pos).begin(), set_b_pasts.at(pos).end(),
                     set_a_past_inv_perm.begin(), set_a_past_inv_perm.end())) {
    return false;
  }

  if (pos == n) {
    return true;
  }
  CHECK_LT(pos, n);
  if (symmetric) {
    CHECK_EQ(n % 2, 0);
    if (pos == n / 2) {
      return true;
    }
    CHECK_LT(pos, n - 1 - pos);
  }

  // Try each unused bit position for the current position
  for (int i = 0; i < n; i++) {
    if (used[i]) {
      continue;
    }
    if (symmetric && used[n - 1 - i]) {
      continue;
    }

    perm[pos] = i;
    used[i] = true;
    if (symmetric) {
      perm[n - 1 - pos] = n - 1 - i;
      used[n - 1 - i] = true;
    }

    if (IsIsomorphicToSubsetBacktrackingRecursive(n, set_a, set_b, symmetric,
                                                  set_b_pasts, pos + 1, perm,
                                                  used, gen)) {
      return true;
    }

    used[i] = false;
    if (symmetric) {
      used[n - 1 - i] = false;
    }
  }
  return false;
}

bool IsIsomorphicToSubsetNegativePrecheckByRow(
    const std::array<std::vector<uint8_t>, 2> &count_by_row_a_sorted,
    const std::array<std::vector<uint8_t>, 2> &count_by_row_b_sorted) {
  for (int bit = 0; bit < 2; bit++) {
    CHECK_LE(count_by_row_a_sorted[bit].size(),
             count_by_row_b_sorted[bit].size());
    for (int i = 0; i < count_by_row_a_sorted[bit].size(); i++) {
      if (count_by_row_a_sorted[bit][i] < count_by_row_b_sorted[bit][i]) {
        return false;
      }
    }
  }
  return true;
}

bool IsIsomorphicToSubsetNegativePrecheckByCol(
    int n, const std::array<std::vector<uint64_t>, 2> &count_by_col_a_sorted,
    const std::array<std::vector<uint64_t>, 2> &count_by_col_b_sorted) {
  for (int bit = 0; bit < 2; bit++) {
    CHECK_EQ(count_by_col_a_sorted[bit].size(), n);
    CHECK_EQ(count_by_col_b_sorted[bit].size(), n);
    for (int i = 0; i < n; i++) {
      if (count_by_col_a_sorted[bit][i] > count_by_col_b_sorted[bit][i]) {
        return false;
      }
    }
  }
  return true;
}

} // namespace

namespace internal {

std::array<std::vector<uint8_t>, 2>
AggregateRows(int n, const std::vector<OutputType> &set, bool sort) {
  std::vector<uint8_t> one_count_by_row(set.size(), 0);
  std::vector<uint8_t> zero_count_by_row(set.size(), 0);
  for (int i = 0; i < set.size(); i++) {
    OutputType x = set[i];
    one_count_by_row[i] = std::popcount(x);
    zero_count_by_row[i] = n - one_count_by_row[i];
  }
  if (sort) {
    std::sort(one_count_by_row.begin(), one_count_by_row.end());
    std::sort(zero_count_by_row.begin(), zero_count_by_row.end());
  }
  return {zero_count_by_row, one_count_by_row};
}

std::array<std::vector<uint64_t>, 2>
AggregateColumns(int n, const std::vector<OutputType> &set, bool sort) {
  std::vector<uint64_t> one_count_by_col(n, 0);
  std::vector<uint64_t> zero_count_by_col(n, 0);
  for (int i = 0; i < n; i++) {
    uint64_t one_count_by_col_i = 0;
    for (OutputType x : set) {
      OutputType bit = (x >> i) & OutputType(1);
      one_count_by_col_i += bit;
    }
    one_count_by_col[i] = one_count_by_col_i;
    zero_count_by_col[i] = set.size() - one_count_by_col_i;
  }
  if (sort) {
    std::sort(one_count_by_col.begin(), one_count_by_col.end());
    std::sort(zero_count_by_col.begin(), zero_count_by_col.end());
  }
  return {zero_count_by_col, one_count_by_col};
}

bool IsIsomorphicToSubsetSlow(int n, const std::vector<OutputType> &set_a,
                              const std::vector<OutputType> &set_b) {
  CHECK(std::is_sorted(set_b.begin(), set_b.end()));
  std::vector<int> perm(n);
  std::iota(perm.begin(), perm.end(), 0);
  do {
    std::vector<OutputType> set_a_perm;
    set_a_perm.reserve(set_a.size());
    for (OutputType a : set_a) {
      OutputType a_perm = 0;
      for (int i = 0; i < n; i++) {
        a_perm |= ((a >> i) & OutputType(1)) << perm[i];
      }
      set_a_perm.push_back(a_perm);
    }
    std::sort(set_a_perm.begin(), set_a_perm.end());
    if (std::includes(set_b.begin(), set_b.end(), set_a_perm.begin(),
                      set_a_perm.end())) {
      return true;
    }
  } while (std::next_permutation(perm.begin(), perm.end()));
  return false;
}

bool IsIsomorphicToSubsetBacktracking(int n,
                                      const std::vector<OutputType> &set_a,
                                      const std::vector<OutputType> &set_b,
                                      bool symmetric, std::mt19937 *gen) {
  if (symmetric) {
    CHECK_EQ(n % 2, 0);
  }

  CHECK(std::is_sorted(set_b.begin(), set_b.end()));
  std::vector<int> perm(n);
  std::vector<bool> used(n, false);

  std::vector<std::vector<OutputType>> set_b_pasts;
  for (int pos = 0; pos <= (symmetric ? n / 2 : n); pos++) {
    // past_mask
    OutputType past_mask = (OutputType(1) << pos) - 1; // bit 0..pos-1
    if (symmetric) {
      past_mask |= past_mask << (n - pos); // bit n-pos .. n-1
    }
    // set_b_past
    std::vector<OutputType> set_b_past;
    set_b_past.reserve(set_b.size());
    for (OutputType b : set_b) {
      set_b_past.push_back(b & past_mask);
    }
    std::sort(set_b_past.begin(), set_b_past.end());
    set_b_pasts.push_back(std::move(set_b_past));
  }

  return IsIsomorphicToSubsetBacktrackingRecursive(
      n, set_a, set_b, symmetric, set_b_pasts, 0, perm, used, gen);
}

bool IsIsomorphicToSubset(
    int n, const std::vector<OutputType> &set_a,
    const std::array<std::vector<uint8_t>, 2> &count_by_row_a_sorted,
    const std::array<std::vector<uint64_t>, 2> &count_by_col_a_sorted,
    const std::vector<OutputType> &set_b,
    const std::array<std::vector<uint8_t>, 2> &count_by_row_b_sorted,
    const std::array<std::vector<uint64_t>, 2> &count_by_col_b_sorted,
    bool symmetric, std::mt19937 *gen) {
  CHECK(std::is_sorted(set_b.begin(), set_b.end()));

  if (!IsIsomorphicToSubsetNegativePrecheck(
          n, set_a, count_by_row_a_sorted, count_by_col_a_sorted, set_b,
          count_by_row_b_sorted, count_by_col_b_sorted)) {
    return false;
  }

  return IsIsomorphicToSubsetBacktracking(n, set_a, set_b, symmetric, gen);
}

// Return true if outputs_collection[i] is redundant.
bool IsRedundant(
    int n, int i,
    const std::vector<std::vector<OutputType>> &outputs_collection,
    const std::vector<std::array<std::vector<uint8_t>, 2>>
        &count_by_row_sorted_collection,
    const std::vector<std::array<std::vector<uint64_t>, 2>>
        &count_by_col_sorted_collection,
    const std::vector<std::vector<OutputType>> &outputs_collection_inv,
    const std::vector<std::array<std::vector<uint8_t>, 2>>
        &count_by_row_inv_sorted_collection,
    const std::vector<std::array<std::vector<uint64_t>, 2>>
        &count_by_col_inv_sorted_collection,
    const std::vector<std::atomic<bool>> &is_redundant_atomic, bool fast,
    bool is_last_pass, bool symmetric, std::mt19937 *gen) {
  auto size_i = outputs_collection[i].size();
  for (int j = 0; j < outputs_collection.size(); j++) {
    if (is_redundant_atomic[j].load()) {
      continue;
    }
    if (j == i) {
      continue;
    }
    auto size_j = outputs_collection[j].size();
    if (size_i < size_j) {
      break;
    }
    if (size_i == size_j && i < j) {
      break;
    }
    // (size_i, i) > (size_j, j)
    if (fast || !is_last_pass) {
      if (!IsIsomorphicToSubsetNegativePrecheckByCol(
              n, count_by_col_sorted_collection[j],
              count_by_col_sorted_collection[i])) {
        continue;
      }
      if (std::includes(
              outputs_collection[i].begin(), outputs_collection[i].end(),
              outputs_collection[j].begin(), outputs_collection[j].end())) {
        return true;
      }
      if (!outputs_collection_inv.empty()) {
        if (!IsIsomorphicToSubsetNegativePrecheckByCol(
                n, count_by_col_sorted_collection[j],
                count_by_col_inv_sorted_collection[i])) {
          continue;
        }
        if (std::includes(outputs_collection_inv[i].begin(),
                          outputs_collection_inv[i].end(),
                          outputs_collection[j].begin(),
                          outputs_collection[j].end())) {
          return true;
        }
      }
    } else {
      // last pass
      if (IsIsomorphicToSubset(
              n, outputs_collection[j], count_by_row_sorted_collection[j],
              count_by_col_sorted_collection[j], outputs_collection[i],
              count_by_row_sorted_collection[i],
              count_by_col_sorted_collection[i], symmetric, gen)) {
        return true;
      }
      CHECK(!outputs_collection_inv.empty());
      if (IsIsomorphicToSubset(
              n, outputs_collection[j], count_by_row_sorted_collection[j],
              count_by_col_sorted_collection[j], outputs_collection_inv[i],
              count_by_row_inv_sorted_collection[i],
              count_by_col_inv_sorted_collection[i], symmetric, gen)) {
        return true;
      }
    }
  }
  return false;
}

} // namespace internal

std::pair<std::vector<OutputType>, std::vector<int>>
SortByWeight(int n, const std::vector<OutputType> &set,
             std::mt19937 *nullable_gen, bool symmetric) {
  std::vector<uint64_t> count_one_by_col =
      internal::AggregateColumns(n, set, false)[1];
  std::vector<int> inv_perm(n);
  std::iota(inv_perm.begin(), inv_perm.end(), 0);
  if (nullable_gen) {
    if (symmetric) {
      CHECK_EQ(n % 2, 0);
      std::uniform_int_distribution<int> dist(0, n - 1);
      for (int i = 0; i < n; i++) {
        int j = dist(*nullable_gen);
        std::swap(inv_perm[i], inv_perm[j]);
        if (i + j != n - 1) {
          std::swap(inv_perm[n - 1 - i], inv_perm[n - 1 - j]);
        }
      }
    } else {
      std::shuffle(inv_perm.begin(), inv_perm.end(), *nullable_gen);
    }
  }
  std::vector<OutputType> set_perm =
      SortByWeight(n, set, count_one_by_col, &inv_perm);
  return {set_perm, InversePermutation(inv_perm)};
}

bool IsIsomorphicToSubsetNegativePrecheck(
    int n, const std::vector<OutputType> &set_a,
    const std::array<std::vector<uint8_t>, 2> &count_by_row_a_sorted,
    const std::array<std::vector<uint64_t>, 2> &count_by_col_a_sorted,
    const std::vector<OutputType> &set_b,
    const std::array<std::vector<uint8_t>, 2> &count_by_row_b_sorted,
    const std::array<std::vector<uint64_t>, 2> &count_by_col_b_sorted) {
  if (set_a.size() > set_b.size()) {
    return false;
  }
  if (!IsIsomorphicToSubsetNegativePrecheckByCol(n, count_by_col_a_sorted,
                                                 count_by_col_b_sorted)) {
    return false;
  }
  if (!IsIsomorphicToSubsetNegativePrecheckByRow(count_by_row_a_sorted,
                                                 count_by_row_b_sorted)) {
    return false;
  }
  return true;
}

std::vector<bool>
FindRedundantOutputs(int n,
                     std::vector<std::vector<OutputType>> outputs_collection,
                     bool fast, bool symmetric, std::mt19937 *gen) {
  CHECK(std::is_sorted(
      outputs_collection.begin(), outputs_collection.end(),
      [](const auto &a, const auto &b) { return a.size() < b.size(); }));

  if (symmetric) {
    // Symmetry check is expensive for large n, so we only verify it for n < 16.
    // For larger n, we assume the caller has ensured symmetry.
    if (n < 16) {
      // It is slow when n is large.
      // If we really want to check the symmetry for large n, we can parallelize
      // the check.
      LOG(INFO)
          << "FindRedundantOutputs: Checking if all outputs are symmetric";
      for (const auto &outputs : outputs_collection) {
        CHECK(IsSymmetric(n, outputs));
      }
    }
  }

  LOG(INFO) << "Computing count by row and column";
  std::vector<std::array<std::vector<uint8_t>, 2>>
      count_by_row_sorted_collection;
  std::vector<std::array<std::vector<uint8_t>, 2>>
      count_by_row_inv_sorted_collection;
  std::vector<std::array<std::vector<uint64_t>, 2>>
      count_by_col_sorted_collection;
  std::vector<std::array<std::vector<uint64_t>, 2>>
      count_by_col_inv_sorted_collection;
  {
    // Compute count by row and column in parallel
    count_by_row_sorted_collection.resize(outputs_collection.size());
    count_by_row_inv_sorted_collection.resize(outputs_collection.size());
    count_by_col_sorted_collection.resize(outputs_collection.size());
    count_by_col_inv_sorted_collection.resize(outputs_collection.size());
    std::atomic<int> next_i(0);
    int num_threads = std::thread::hardware_concurrency();
    CHECK_GT(num_threads, 0);
    auto worker = [&]() {
      while (true) {
        int i = next_i.fetch_add(1);
        if (i >= outputs_collection.size()) {
          break;
        }
        const auto &outputs = outputs_collection[i];
        count_by_row_sorted_collection[i] =
            internal::AggregateRows(n, outputs, true);
        count_by_row_inv_sorted_collection[i] = {
            count_by_row_sorted_collection[i][1],
            count_by_row_sorted_collection[i][0]};
        count_by_col_sorted_collection[i] =
            internal::AggregateColumns(n, outputs, true);
        count_by_col_inv_sorted_collection[i] = {
            count_by_col_sorted_collection[i][1],
            count_by_col_sorted_collection[i][0]};
      }
    };
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; t++) {
      threads.emplace_back(worker);
    }
    for (auto &thread : threads) {
      thread.join();
    }
  }

  std::vector<std::atomic<bool>> is_redundant_atomic(outputs_collection.size());
  int num_passes = 6;
  if (fast) {
    num_passes = 2;
  }
  int num_threads = std::thread::hardware_concurrency();
  CHECK_GT(num_threads, 0);
  for (int pass = 0; pass < num_passes; pass++) {
    std::cout << "Pass " << pass << ". Count: "
              << std::count(is_redundant_atomic.begin(),
                            is_redundant_atomic.end(), false)
              << std::endl;
    std::vector<std::vector<OutputType>> outputs_collection_inv;
    if (!fast) {
      if (pass + 2 >= num_passes) { // last two passes
        outputs_collection_inv = outputs_collection;
      }
    }
    // SortByWeight in parallel
    {
      std::atomic<int> next_i(0);
      auto worker = [&]() {
        while (true) {
          int i = next_i.fetch_add(1);
          if (i >= outputs_collection.size()) {
            break;
          }
          if (is_redundant_atomic[i].load()) {
            continue;
          }
          outputs_collection[i] =
              SortByWeight(n, outputs_collection[i], gen, symmetric).first;
          if (outputs_collection_inv.empty()) {
            continue;
          }
          for (OutputType &x : outputs_collection_inv[i]) {
            x ^= (OutputType(1) << n) - 1;
          }
          outputs_collection_inv[i] =
              SortByWeight(n, outputs_collection_inv[i], gen, symmetric).first;
        }
      };
      std::vector<std::thread> threads;
      for (int t = 0; t < num_threads; t++) {
        threads.emplace_back(worker);
      }
      for (auto &thread : threads) {
        thread.join();
      }
    }
    // Check redundancy in parallel
    {
      std::atomic<int> next_i(0);
      auto worker = [&]() {
        while (true) {
          int i = next_i.fetch_add(1);
          if (i >= outputs_collection.size()) {
            break;
          }
          if (i % 64 == 0 || i + 1 == outputs_collection.size() ||
              (!fast && pass + 1 == num_passes)) {
            std::cout << std::format("Progress: {}/{} \r", i,
                                     outputs_collection.size())
                      << std::flush;
          }
          if (is_redundant_atomic[i].load()) {
            continue;
          }
          is_redundant_atomic[i].store(internal::IsRedundant(
              n, i, outputs_collection, count_by_row_sorted_collection,
              count_by_col_sorted_collection, outputs_collection_inv,
              count_by_row_inv_sorted_collection,
              count_by_col_inv_sorted_collection, is_redundant_atomic, fast,
              pass + 1 == num_passes, symmetric, gen));
        }
      };

      std::vector<std::thread> threads;
      for (int t = 0; t < num_threads; t++) {
        threads.emplace_back(worker);
      }
      for (auto &thread : threads) {
        thread.join();
      }
    }
    std::cout << '\n';
  }
  std::vector<bool> is_redundant(outputs_collection.size());
  for (int i = 0; i < outputs_collection.size(); i++) {
    is_redundant[i] = is_redundant_atomic[i].load();
  }
  std::cout << "Non-redundant count: "
            << std::count(is_redundant.begin(), is_redundant.end(), false)
            << std::endl;
  return is_redundant;
}
