#include "math_utils.h"

#include "glog/logging.h"

#include <algorithm>
#include <numeric>

std::vector<int> RandomPermutation(int n, std::mt19937 *gen) {
  CHECK_GT(n, 0);
  std::vector<int> perm(n);
  std::iota(perm.begin(), perm.end(), 0);
  std::shuffle(perm.begin(), perm.end(), *gen);
  return perm;
}

std::vector<int> InversePermutation(const std::vector<int> &perm) {
  std::vector<int> inv_perm(perm.size());
  for (int i = 0; i < perm.size(); i++) {
    inv_perm.at(perm.at(i)) = i;
  }
  return inv_perm;
}
