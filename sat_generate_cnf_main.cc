#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "cnf_builder.h"
#include "network.h"
#include "network_utils.h"
#include "output_type.h"

DEFINE_int32(n, 0, "Number of channels");
DEFINE_int32(depth, 0, "The depth of the network, including the prefix");
DEFINE_string(input_path, "", "The input prefixes file");
DEFINE_string(output_dir, "", "The output directory");
DEFINE_int32(jobs, std::thread::hardware_concurrency(),
             "Number of parallel jobs");
DEFINE_int32(subnet_channels, -1,
             "The number of channels in the subnet (-1 = no limit)");
DEFINE_int32(limit, -1, "The number of networks to generate (-1 = all)");
DEFINE_bool(symmetric, false, "Search symmetric solutions");

using cnf::Clause;
using cnf::Formula;
using cnf::Literal;

namespace {

// a implies (b == (c or d))
Formula AImpliesBeqCOrD(Literal a, Literal b, Literal c, Literal d) {
  return Formula::And(Clause::Or(!a, b, !c), Clause::Or(!a, b, !d),
                      Clause::Or(!a, !b, c, d));
}

// a implies (b == (c and d))
Formula AImpliesBeqCAndD(Literal a, Literal b, Literal c, Literal d) {
  return Formula::And(Clause::Or(!a, !b, c), Clause::Or(!a, !b, d),
                      Clause::Or(!a, b, !c, !d));
}

// Build the formula for the network suffix.
// d: the depth of the network, not including the prefix.
Formula BuildFormula(int n, int d, const Network &network_prefix,
                     int subnet_channels, cnf::Variables &vars,
                     bool symmetric) {
  if (symmetric) {
    CHECK_EQ(n % 2, 0);
  }

  Formula formula = Formula::True();

  Literal invalid_literal = vars.Add(std::format("invalid"));
  Literal true_literal = vars.Add(std::format("true"));
  formula.AndAssign(Clause(true_literal));
  Literal false_literal = vars.Add(std::format("false"));
  formula.AndAssign(Clause(!false_literal));

  // g[k][i][j] means there is a comparator between i and j in the k-th layer.
  // 0 <= i < j < n.
  // 0 <= k < d.
  std::vector<std::vector<std::vector<Literal>>> g;
  for (int k = 0; k < d; ++k) {
    g.emplace_back();
    for (int i = 0; i < n; ++i) {
      g[k].push_back(std::vector<Literal>(n, invalid_literal));
      for (int j = i + 1; j < n; ++j) {
        if (symmetric) {
          int i_sym = n - 1 - i;
          int j_sym = n - 1 - j;
          // compare (i, j) and (j_sym, i_sym)
          if (j_sym < i) {
            g[k][i][j] = g[k][j_sym][i_sym];
            continue;
          }
        }
        g[k][i][j] = vars.Add(std::format("g_{}_{}_{}", k, i, j));
      }
    }
  }

  // In each layer, each channel is used at most once.
  for (int k = 0; k < d; ++k) {
    for (int i = 0; i < n; ++i) {
      for (int j0 = 0; j0 < n; ++j0) {
        if (i == j0)
          continue;
        for (int j1 = j0 + 1; j1 < n; ++j1) {
          if (i == j1)
            continue;
          Literal no_i_j0 = !g[k][std::min(i, j0)][std::max(i, j0)];
          Literal no_i_j1 = !g[k][std::min(i, j1)][std::max(i, j1)];
          // no_i_j0 or no_i_j1
          formula.AndAssign(Clause::Or(no_i_j0, no_i_j1));
        }
      }
    }
  }

  // used[k][i] means the i-th channel is used in the k-th layer.
  std::vector<std::vector<Literal>> used;
  for (int k = 0; k < d; ++k) {
    used.push_back(std::vector<Literal>(n, invalid_literal));
    for (int i = 0; i < n; ++i) {
      if (symmetric) {
        int i_sym = n - 1 - i;
        if (i_sym < i) {
          used[k][i] = used[k][i_sym];
          continue;
        }
      }
      used[k][i] = vars.Add(std::format("used_{}_{}", k, i));
    }
  }

  // Set used[k][i].
  for (int k = 0; k < d; ++k) {
    for (int i = 0; i < n; ++i) {
      Clause clause;
      for (int j = 0; j < n; ++j) {
        if (i < j) {
          clause.literals.push_back(g[k][i][j]);
        }
        if (i > j) {
          clause.literals.push_back(g[k][j][i]);
        }
      }
      formula.AndAssign(Formula(Clause(used[k][i])) == Formula(clause));
    }
  }

  // 0 <= k < d. 0 <= i < j < n.
  // one_down[k][i][j] indicates whether there is a comparator g[k][i][l] for
  // some i<=l<j.
  // one_up[k][i][j] indicates whether there is a comparator g[k][l][j] for some
  // i<l<=j.
  std::vector<std::vector<std::vector<Literal>>> one_down;
  std::vector<std::vector<std::vector<Literal>>> one_up;
  for (int k = 0; k < d; ++k) {
    one_down.push_back(std::vector<std::vector<Literal>>(
        n, std::vector<Literal>(n, invalid_literal)));
    one_up.push_back(std::vector<std::vector<Literal>>(
        n, std::vector<Literal>(n, invalid_literal)));
    for (int i = 0; i < n; ++i) {
      for (int j = i; j < n; ++j) {
        // one_down
        one_down[k][i][j] = vars.Add(std::format("one_down_{}_{}_{}", k, i, j));
        Clause one_down_clause;
        for (int l = i + 1; l <= j; ++l) {
          one_down_clause.literals.push_back(g[k][i][l]);
        }
        formula.AndAssign(Formula(Clause(one_down[k][i][j])) ==
                          Formula(one_down_clause));
        if (symmetric) {
          one_up[k][n - 1 - j][n - 1 - i] = one_down[k][i][j];
          continue;
        }
        // one_up
        one_up[k][i][j] = vars.Add(std::format("one_up_{}_{}_{}", k, i, j));
        Clause one_up_clause;
        for (int l = i; l < j; ++l) {
          one_up_clause.literals.push_back(g[k][l][j]);
        }
        formula.AndAssign(Formula(Clause(one_up[k][i][j])) ==
                          Formula(one_up_clause));
      }
    }
  }

  // Optimization:
  // Non-redundant comparators in the last layer have to be of the form (i,
  // i+1).
  for (int i = 0; i < n; ++i) {
    for (int j = i + 2; j < n; ++j) {
      formula.AndAssign(Clause(!g[d - 1][i][j]));
    }
  }

  // Optimization on the second last layer:
  // No comparator can connect two channels more than 3 channels apart.
  if (d >= 2) {
    for (int i = 0; i < n; ++i) {
      for (int j = i + 4; j < n; ++j) {
        formula.AndAssign(Clause(!g[d - 2][i][j]));
      }
    }
  }

  // Optimization on the last two layers:
  // The existence of a comparator (i, i+3) on the second last layer implies the
  // existence of comparators (i, i+1) and (i+2, i+3) on the last layer.
  if (d >= 2) {
    for (int i = 0; i < n - 3; ++i) {
      formula.AndAssign(
          Clause::Implies(g[d - 2][i][i + 3], g[d - 1][i][i + 1]));
      formula.AndAssign(
          Clause::Implies(g[d - 2][i][i + 3], g[d - 1][i + 2][i + 3]));
    }
  }

  // Optimization on the last two layers:
  // The existence of a comparator (i, i+2) on the second last layer implies the
  // existence of either comparator (i, i+1) or (i+1, i+2) on the last layer.
  if (d >= 2) {
    for (int i = 0; i < n - 2; ++i) {
      formula.AndAssign(Clause::Or(!g[d - 2][i][i + 2], g[d - 1][i][i + 1],
                                   g[d - 1][i + 1][i + 2]));
    }
  }

  // Optimization on the last layer:
  // No adjacent unused channels in the last layer. Add redundant comparators if
  // possible.
  for (int i = 0; i < n - 1; ++i) {
    formula.AndAssign(Clause::Or(used[d - 1][i], used[d - 1][i + 1]));
  }

  // Optimization: SMALL IMPROVEMENT.
  // Based on Lemma 9, i.e. moving a comparator from the last layer to the
  // second last layer.
  if (d >= 2) {
    for (int i = 0; i < n - 2; ++i) {
      formula.AndAssign(Clause::Or(!g[d - 1][i][i + 1], used[d - 1][i + 2],
                                   used[d - 2][i], used[d - 2][i + 1]));
      formula.AndAssign(Clause::Or(!g[d - 1][i + 1][i + 2], used[d - 1][i],
                                   used[d - 2][i + 1], used[d - 2][i + 2]));
    }
  }

  // The network should sort each binary_string.
  for (int m = 0; m < network_prefix.outputs.size(); ++m) {
    std::string binary_string = ToBinaryString(n, network_prefix.outputs[m]);
    int num_0s = std::count(binary_string.begin(), binary_string.end(), '0');
    int num_1s = std::count(binary_string.begin(), binary_string.end(), '1');
    CHECK_EQ(binary_string.length(), n);
    CHECK_EQ(num_0s + num_1s, n);

    int num_leading_0s = 0;
    for (int i = 0; i < n; ++i) {
      if (binary_string[i] == '0') {
        num_leading_0s++;
      } else {
        break;
      }
    }
    int num_trailing_1s = 0;
    for (int i = n - 1; i >= 0; --i) {
      if (binary_string[i] == '1') {
        num_trailing_1s++;
      } else {
        break;
      }
    }
    int channel_begin = num_leading_0s;
    int channel_end = n - num_trailing_1s;

    if (subnet_channels >= 0 && channel_end - channel_begin > subnet_channels) {
      continue;
    }

    // v[k][i] means the value of the i-th channel before k-th layer.
    // 0 <= k <= d. k=0 means the initial state. and k=d means the final
    // state. 0 <= i < n.
    std::vector<std::vector<Literal>> v;
    for (int k = 0; k <= d; ++k) {
      v.emplace_back();
      for (int i = 0; i < n; ++i) {
        if (i < channel_begin) {
          v[k].push_back(false_literal);
        } else if (i < channel_end) {
          v[k].push_back(vars.Add(std::format("v_{}_{}_{}", m, k, i)));
        } else {
          v[k].push_back(true_literal);
        }
      }
    }

    // Input layer: set v[0][i] to binary_string[i].
    for (int i = channel_begin; i < channel_end; ++i) {
      if (binary_string[i] == '1') {
        formula.AndAssign(Clause(v[0][i]));
      } else {
        formula.AndAssign(Clause(!v[0][i]));
      }
    }

    // The network brings v[k] to v[k+1].
    for (int k = 0; k < d; ++k) {
      for (int i = channel_begin; i < channel_end; ++i) {
        // one-up: if v[k][i] is false and no comparators go up from i, then
        // v[k+1][i] is false.
        formula.AndAssign(
            Clause::Or(v[k][i], one_up[k][channel_begin][i], !v[k + 1][i]));
        for (int j = channel_begin; j < i; ++j) {
          // g[k][j][i].Implies(v[k + 1][i] == (v[k][j] || v[k][i]))
          formula.AndAssign(
              AImpliesBeqCOrD(g[k][j][i], v[k + 1][i], v[k][j], v[k][i]));
        }
        // one-down: if v[k][i] is true and no comparators go down from i,
        // then v[k+1][i] is true.
        formula.AndAssign(
            Clause::Or(!v[k][i], one_down[k][i][channel_end - 1], v[k + 1][i]));
        for (int j = i + 1; j < channel_end; ++j) {
          // g[k][i][j].Implies(v[k + 1][i] == (v[k][i] && v[k][j]))
          formula.AndAssign(
              AImpliesBeqCAndD(g[k][i][j], v[k + 1][i], v[k][i], v[k][j]));
        }
      }
    }

    // Output layer: verify it is sorted, i.e. num_0s 0s followed by num_1s 1s.
    for (int i = channel_begin; i < channel_end; ++i) {
      if (i < num_0s) {
        formula.AndAssign(Clause(!v[d][i]));
      } else {
        formula.AndAssign(Clause(v[d][i]));
      }
    }
  }

  // Check the formula doesn't have invalid_literal.
  CHECK(!formula.Find(invalid_literal.Variable()))
      << formula.ToString(vars) << " has invalid_literal.";

  return formula;
}

double GenerateCnf(int n, int depth, int network_prefix_idx,
                   const Network &network_prefix, const std::string &cnf_dir,
                   int subnet_channels, bool symmetric) {
  std::string cnf_file =
      std::format("{}/{:04}.cnf", cnf_dir, network_prefix_idx);
  std::string cnf_gzip_file = cnf_file + ".gz";

  if (std::filesystem::exists(cnf_file) ||
      std::filesystem::exists(cnf_gzip_file)) {
    return 0.0;
  }

  auto start_time = std::chrono::high_resolution_clock::now();
  cnf::Variables vars;
  Formula formula =
      BuildFormula(n, depth, network_prefix, subnet_channels, vars, symmetric);

  std::string tmp_file = cnf_file + ".tmp.gz";
  formula.WriteToDimacs(tmp_file, vars);
  std::filesystem::rename(tmp_file, cnf_gzip_file);

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);
  return duration_ms.count() / 1000.0;
}

} // namespace

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "/tmp";
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  CHECK_EQ(argc, 1);

  CHECK_GT(FLAGS_n, 0);
  CHECK_GT(FLAGS_depth, 0);

  auto start_time = std::chrono::high_resolution_clock::now();

  std::string cnf_dir = FLAGS_output_dir;
  if (cnf_dir.empty()) {
    cnf_dir = std::format("dimacs/n{}.d{}", FLAGS_n, FLAGS_depth);
    if (FLAGS_symmetric) {
      cnf_dir += ".sym";
    }
    if (FLAGS_subnet_channels >= 0) {
      cnf_dir += std::format(".sc{}", FLAGS_subnet_channels);
    }
  }

  if (std::filesystem::exists(cnf_dir)) {
    std::cout << "Directory " << cnf_dir
              << " already exists. Delete or resume? ";
    std::string choice;
    std::cin >> choice;
    if (choice == "delete") {
      std::filesystem::remove_all(cnf_dir);
    } else if (choice != "resume") {
      LOG(FATAL) << "Unknown choice: " << choice;
    }
  }

  std::filesystem::create_directories(cnf_dir);

  std::string pb_file = FLAGS_input_path;
  if (pb_file.empty()) {
    pb_file = std::format("pb/n{}.pb.txt", FLAGS_n);
  }
  std::ifstream in(pb_file);
  if (!in.is_open()) {
    LOG(FATAL) << "Failed to open file: " << pb_file;
  }

  std::vector<Network> network_prefixes = LoadFromProtoFile(pb_file, FLAGS_n);
  std::cout << "Loaded " << network_prefixes.size() << " network prefixes from "
            << pb_file << std::endl;
  CHECK(!network_prefixes.empty());
  int num_layers = network_prefixes.front().layers.size();
  CHECK_GT(num_layers, 0);
  CHECK_LE(num_layers, FLAGS_depth);
  for (const auto &network_prefix : network_prefixes) {
    CHECK_EQ(network_prefix.n, FLAGS_n);
    CHECK_EQ(network_prefix.layers.size(), num_layers);
  }

  std::cout << std::format("Using {} CPU cores for parallel processing",
                           FLAGS_jobs)
            << std::endl;

  int prefix_count = network_prefixes.size();
  if (FLAGS_limit >= 0) {
    prefix_count = std::min(prefix_count, FLAGS_limit);
  }

  // Process network prefixes in parallel
  std::atomic<int> next_prefix_idx(0);

  auto worker_lambda = [&]() {
    while (true) {
      int current_idx = next_prefix_idx.fetch_add(1);
      if (current_idx >= prefix_count) {
        break;
      }

      double build_time =
          GenerateCnf(FLAGS_n, FLAGS_depth - num_layers, current_idx,
                      network_prefixes[current_idx], cnf_dir,
                      FLAGS_subnet_channels, FLAGS_symmetric);

      std::cout << std::format("{}/{}. build_time: {} seconds    \r",
                               current_idx, prefix_count, build_time)
                << std::flush;
    }
  };

  std::vector<std::thread> workers;
  for (int i = 0; i < FLAGS_jobs; ++i) {
    workers.emplace_back(worker_lambda);
  }

  // Wait for all workers to complete
  for (auto &worker : workers) {
    worker.join();
  }
  std::cout << std::endl;
  std::cout << "The results are in " << cnf_dir << std::endl;

  auto end_time = std::chrono::high_resolution_clock::now();
  auto total_duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time);
  std::cout << std::format("Total time: {} seconds",
                           total_duration_ms.count() / 1000.0)
            << std::endl;

  return 0;
}
