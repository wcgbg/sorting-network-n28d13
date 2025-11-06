#include "cnf_builder.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "boost/iostreams/device/file.hpp"
#include "boost/iostreams/filter/gzip.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "glog/logging.h"

namespace cnf {

auto Clause::operator<=>(const Clause &that) const {
  // First sort the literals, then compare the two vectors.
  std::vector<Literal> literals_sorted = literals;
  std::sort(literals_sorted.begin(), literals_sorted.end());
  std::vector<Literal> that_literals_sorted = that.literals;
  std::sort(that_literals_sorted.begin(), that_literals_sorted.end());
  return literals_sorted <=> that_literals_sorted;
}

bool Clause::operator==(const Clause &that) const {
  return operator<=>(that) == 0;
}

bool Clause::operator!=(const Clause &that) const {
  return operator<=>(that) != 0;
}

bool Clause::Find(int32_t var) const {
  for (const auto &literal : literals) {
    if (literal.Find(var)) {
      return true;
    }
  }
  return false;
}

void Formula::WriteToDimacs(const Variables &vars, std::ostream &out) {
  // Write DIMACS header
  const std::vector<std::string> &var_names = vars.var_names();
  for (int i = 0; i < var_names.size(); i++) {
    out << std::format("c var {} : {}\n", i + 1, var_names[i]);
  }
  out << std::format("p cnf {} {}\n", var_names.size(), clauses_.size());
  // Write clauses
  std::string line;
  line.reserve(1024);
  for (const auto &clause : clauses_) {
    line.clear();
    for (Literal literal : clause.literals) {
      if (literal.i >= 0) {
        line += std::format("{} ", literal.i + 1);
      } else {
        line += std::format("{} ", literal.i);
      }
    }
    line += "0\n"; // End of clause marker
    out << line;
  }
}

void Formula::WriteToDimacs(const std::string &filename,
                            const Variables &vars) {
  if (filename.ends_with(".gz")) {
    // Use Boost Iostreams with gzip compression
    boost::iostreams::filtering_ostream out;
    out.push(boost::iostreams::gzip_compressor());
    out.push(boost::iostreams::file_sink(filename));
    CHECK(out.good()) << "Failed to open gzip file: " << filename;
    WriteToDimacs(vars, out);
  } else {
    // Use regular file output
    constexpr size_t kBufferSize = 1024 * 1024; // 1MB buffer
    std::vector<char> buffer(kBufferSize);
    std::ofstream out(filename);
    CHECK(out.is_open()) << "Failed to open file: " << filename;
    // Set larger buffer for better I/O performance
    out.rdbuf()->pubsetbuf(buffer.data(), kBufferSize);
    WriteToDimacs(vars, out);
  }
}

Formula Formula::operator!() const {
  if (clauses_.empty()) {
    // !True = False
    return Formula::False();
  }

  // not (a or b or c) is
  // not a and not b and not c
  if (clauses_.size() == 1) {
    Formula result;
    for (Literal literal : clauses_[0].literals) {
      result.clauses_.emplace_back();
      result.clauses_.back().literals.push_back(!literal);
    }
    return result;
  }

  // De-morgan's law
  Formula result = Formula::False();
  for (const auto &clause : clauses_) {
    result = result || !Formula(clause);
  }
  return result;
}

void Formula::AndAssign(Formula that) {
  clauses_.insert(clauses_.end(),
                  std::make_move_iterator(that.clauses_.begin()),
                  std::make_move_iterator(that.clauses_.end()));
}

Formula Formula::operator&&(const Formula &that) const {
  Formula result = *this;
  result.AndAssign(that);
  return result;
}

Formula Formula::operator||(const Formula &that) const {
  if (clauses_.empty()) {
    return True();
  }
  if (that.clauses_.empty()) {
    return True();
  }

  // Simple case: combine two single clauses
  if (clauses_.size() == 1 && that.clauses_.size() == 1) {
    Formula result(clauses_.front());
    result.clauses_.front().literals.insert(
        result.clauses_.front().literals.end(),
        that.clauses_.front().literals.begin(),
        that.clauses_.front().literals.end());
    return result;
  }

  Formula result;
  for (const auto &this_clause : clauses_) {
    for (const auto &that_clause : that.clauses_) {
      result.AndAssign(Formula(this_clause) || Formula(that_clause));
    }
  }

  return result;
}

Formula Formula::operator==(const Formula &that) const {
  // A == B is equivalent to (A || !B) && (!A || B)
  Formula not_this = !*this;
  Formula not_that = !that;
  Formula left = *this || not_that;
  Formula right = not_this || that;
  return left && right;
}

Formula Formula::operator!=(const Formula &that) const {
  // A != B is equivalent to (A || B) && (!A || !B)
  Formula not_this = !*this;
  Formula not_that = !that;
  Formula left = *this || that;
  Formula right = not_this || not_that;
  return left && right;
}

// Additional operations needed for the conversion
Formula Formula::Implies(const Formula &that) const {
  // A => B is equivalent to !A || B
  return !*this || that;
}

bool Formula::Find(int32_t var) const {
  for (const auto &clause : clauses_) {
    if (clause.Find(var)) {
      return true;
    }
  }
  return false;
}

std::string Literal::ToString(const Variables &vars) const {
  const std::vector<std::string> &var_names = vars.var_names();

  if (i >= 0) {
    // Positive literal
    return var_names.at(i);
  } else {
    // Negative literal
    int32_t var_idx = ~i;
    return std::format("~{}", var_names.at(var_idx));
  }
}

std::string Clause::ToString(const Variables &vars) const {
  if (literals.empty()) {
    return "false";
  }

  std::string result;
  for (size_t i = 0; i < literals.size(); ++i) {
    if (i > 0) {
      result += " || ";
    }
    result += literals[i].ToString(vars);
  }
  return result;
}

std::string Formula::ToString(const Variables &vars) const {
  if (clauses_.empty()) {
    return "true";
  }

  std::string result;
  for (size_t i = 0; i < clauses_.size(); ++i) {
    if (i > 0) {
      result += " && ";
    }
    result += "(";
    result += clauses_[i].ToString(vars);
    result += ")";
  }
  return result;
}

} // namespace cnf
