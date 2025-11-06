#pragma once

#include <stdint.h>
#include <compare>
#include <string>
#include <vector>
#include <iosfwd>
#include <utility>

namespace cnf {

class Variables;

struct Literal {
  Literal(int32_t var_idx) : i(var_idx) {}
  // The negative of variable i is ~i.
  Literal operator!() const { return Literal(~i); }

  auto operator<=>(const Literal &that) const { return i <=> that.i; }
  bool operator==(const Literal &that) const { return operator<=>(that) == 0; }
  bool operator!=(const Literal &that) const { return operator<=>(that) != 0; }

  int32_t Variable() const { return i >= 0 ? i : ~i; }
  std::string ToString(const Variables &vars) const;
  bool Find(int32_t var) const { return i == var || i == ~var; }

  int32_t i = 0;
};

struct Clause {
  Clause() = default; // false
  Clause(Literal literal) : literals({literal}) {}
  Clause(std::vector<Literal> literals) : literals(std::move(literals)) {}
  static Clause Or(Literal a, Literal b) { return Clause({a, b}); }
  static Clause Or(Literal a, Literal b, Literal c) {
    return Clause({a, b, c});
  }
  static Clause Or(Literal a, Literal b, Literal c, Literal d) {
    return Clause({a, b, c, d});
  }
  static Clause Implies(Literal a, Literal b) { return Clause({!a, b}); }

  auto operator<=>(const Clause &that) const;
  bool operator==(const Clause &that) const;
  bool operator!=(const Clause &that) const;

  std::string ToString(const Variables &vars) const;
  bool Find(int32_t var) const;

  std::vector<Literal> literals;
};

class Formula {
public:
  Formula(Clause clause) { clauses_.push_back(std::move(clause)); }
  Formula(std::vector<Clause> clauses) { clauses_ = std::move(clauses); }
  Formula(const Formula &) = default;
  Formula(Formula &&) = default;
  Formula &operator=(const Formula &) = default;
  Formula &operator=(Formula &&) = default;

  static Formula And(Clause a, Clause b) {
    return Formula({std::move(a), std::move(b)});
  }
  static Formula And(Clause a, Clause b, Clause c) {
    return Formula({std::move(a), std::move(b), std::move(c)});
  }

  static Formula True() {
    // True is represented as an empty CNF (no clauses)
    return {};
  }
  static Formula False() {
    // False is represented as a single empty clause
    return Formula(Clause());
  }

  const std::vector<Clause> &clauses() const { return clauses_; }
  bool IsTrue() const { return clauses_.empty(); }
  bool IsFalse() const {
    return clauses_.size() == 1 && clauses_.front().literals.empty();
  }
  void WriteToDimacs(const std::string &filename, const Variables &vars);
  std::string ToString(const Variables &vars) const;

  void AndAssign(Formula that);
  void AndAssign(Clause clause) { clauses_.push_back(std::move(clause)); }
  // The negation of variable i is ~i.
  Formula operator!() const;
  Formula operator&&(const Formula &that) const;
  Formula operator||(const Formula &that) const;
  Formula operator==(const Formula &that) const;
  Formula operator!=(const Formula &that) const;
  Formula Implies(const Formula &that) const;

  bool Find(int32_t var) const;

private:
  Formula() = default; // True
  void WriteToDimacs(const Variables &vars, std::ostream &out);

  std::vector<Clause> clauses_; // CNF clauses.
};

class Variables {
public:
  Variables() = default;
  Literal Add(const std::string &name) {
    Literal literal(var_names_.size());
    var_names_.push_back(name);
    return literal;
  }
  const std::vector<std::string> &var_names() const { return var_names_; }

private:
  std::vector<std::string> var_names_;
};

} // namespace cnf
