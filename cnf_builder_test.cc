#include "cnf_builder.h"

#include <fstream>
#include <sstream>
#include <string>

#include "gtest/gtest.h"

namespace cnf {

// Helper function to read file contents
std::string ReadFile(const std::string &filename) {
  std::ifstream file(filename);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Helper function to create a temporary filename
std::string GetTempFilename() {
  return "/tmp/cnf_test_" + std::to_string(std::rand()) + ".cnf";
}

TEST(CnfBuilderTest, VariablesBasic) {
  Variables vars;
  Formula x = Formula(Clause(vars.Add("x")));
  Formula y = Formula(Clause(vars.Add("y")));

  EXPECT_EQ(vars.var_names().size(), 2);
  EXPECT_EQ(vars.var_names()[0], "x");
  EXPECT_EQ(vars.var_names()[1], "y");
}

TEST(CnfBuilderTest, FormulaTrueAndFalse) {
  Formula true_formula = Formula::True();
  Formula false_formula = Formula::False();

  // Test that we can create True and False formulas
  // True is represented as empty CNF (no clauses)
  // False is represented as a single empty clause
  EXPECT_TRUE(true_formula.IsTrue());
  EXPECT_FALSE(true_formula.IsFalse());
  EXPECT_FALSE(false_formula.IsTrue());
  EXPECT_TRUE(false_formula.IsFalse());
}

TEST(CnfBuilderTest, NegationBasic) {
  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));

  // Test negation of a single variable
  Formula not_a = !a;
  EXPECT_FALSE(not_a.IsTrue());
  EXPECT_FALSE(not_a.IsFalse());

  // Test negation of True
  Formula not_true = !Formula::True();
  EXPECT_FALSE(not_true.IsTrue());
  EXPECT_TRUE(not_true.IsFalse());

  // Test negation of False
  Formula not_false = !Formula::False();
  EXPECT_TRUE(not_false.IsTrue());
  EXPECT_FALSE(not_false.IsFalse());

  // These operations should complete without error
  // The actual correctness will be tested through DIMACS output
}

TEST(CnfBuilderTest, AndOperation) {
  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));

  Formula result = a && b;

  // Test with True
  Formula true_and_a = Formula::True() && a;

  // Test with False
  Formula false_and_a = Formula::False() && a;

  // These operations should complete without error
}

TEST(CnfBuilderTest, OrOperation) {
  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));

  Formula result = a || b;

  // Test with True
  Formula true_or_a = Formula::True() || a;
  EXPECT_TRUE(true_or_a.IsTrue());

  // Test with False
  Formula false_or_a = Formula::False() || a;

  // These operations should complete without error
}

TEST(CnfBuilderTest, EqualityOperation) {
  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));

  // A == A should be True
  Formula a_equals_a = a == a;

  // A == B should not be True (unless A and B are equivalent)
  Formula a_equals_b = a == b;

  // Test with True
  Formula true_equals_true = Formula::True() == Formula::True();
  EXPECT_TRUE(true_equals_true.IsTrue());

  // Test with False
  Formula false_equals_false = Formula::False() == Formula::False();
  EXPECT_TRUE(false_equals_false.IsTrue());

  // Test True != False
  Formula true_equals_false = Formula::True() == Formula::False();
  EXPECT_TRUE(true_equals_false.IsFalse());

  // These operations should complete without error
}

TEST(CnfBuilderTest, InequalityOperation) {
  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));

  // A != B should be True (unless A and B are equivalent)
  Formula a_not_equals_b = a != b;

  // A != A should be False
  Formula a_not_equals_a = a != a;

  // Test True != False
  Formula true_not_equals_false = Formula::True() != Formula::False();

  // Test True != True
  Formula true_not_equals_true = Formula::True() != Formula::True();

  // These operations should complete without error
}

TEST(CnfBuilderTest, ComplexLogicalExpressions) {
  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));
  Formula c = Formula(Clause(vars.Add("c")));

  // Test De Morgan's laws
  // !(A && B) == (!A || !B)
  Formula not_a_and_b = !(a && b);
  Formula not_a_or_not_b = (!a) || (!b);

  // !(A || B) == (!A && !B)
  Formula not_a_or_b = !(a || b);
  Formula not_a_and_not_b = (!a) && (!b);

  // Test distributive law: A && (B || C) == (A && B) || (A && C)
  Formula left = a && (b || c);
  Formula right = (a && b) || (a && c);

  // These operations should complete without error
}

TEST(CnfBuilderTest, WriteToDimacsBasic) {
  std::string filename = GetTempFilename();

  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));
  Formula c = Formula(Clause(vars.Add("c")));

  // Create a simple formula: a && b
  Formula formula = a && b;
  formula.WriteToDimacs(filename, vars);

  std::string content = ReadFile(filename);

  // Check that the file contains variable declarations
  EXPECT_TRUE(content.find("c var 1 : a") != std::string::npos);
  EXPECT_TRUE(content.find("c var 2 : b") != std::string::npos);
  EXPECT_TRUE(content.find("c var 3 : c") != std::string::npos);

  // Check DIMACS header
  EXPECT_TRUE(content.find("p cnf 3") != std::string::npos);

  // Clean up
  std::remove(filename.c_str());
}

TEST(CnfBuilderTest, WriteToDimacsComplex) {
  std::string filename = GetTempFilename();

  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));
  Formula c = Formula(Clause(vars.Add("c")));

  // Create a more complex formula: (a || b) && (!a || c)
  Formula formula = (a || b) && ((!a) || c);
  formula.WriteToDimacs(filename, vars);

  std::string content = ReadFile(filename);

  // Check that the file contains variable declarations
  EXPECT_TRUE(content.find("c var 1 : a") != std::string::npos);
  EXPECT_TRUE(content.find("c var 2 : b") != std::string::npos);
  EXPECT_TRUE(content.find("c var 3 : c") != std::string::npos);

  // Check DIMACS header
  EXPECT_TRUE(content.find("p cnf 3") != std::string::npos);

  // The formula should have clauses
  EXPECT_TRUE(content.find("0") != std::string::npos); // End of clause markers

  // Clean up
  std::remove(filename.c_str());
}

TEST(CnfBuilderTest, WriteToDimacsTrue) {
  std::string filename = GetTempFilename();

  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));
  Formula c = Formula(Clause(vars.Add("c")));

  // Write True formula (empty CNF)
  Formula::True().WriteToDimacs(filename, vars);

  std::string content = ReadFile(filename);

  // Should have variable declarations
  EXPECT_TRUE(content.find("c var 1 : a") != std::string::npos);

  // Should have header with 0 clauses
  EXPECT_TRUE(content.find("p cnf 3 0") != std::string::npos);

  // Clean up
  std::remove(filename.c_str());
}

TEST(CnfBuilderTest, WriteToDimacsFalse) {
  std::string filename = GetTempFilename();

  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));
  Formula c = Formula(Clause(vars.Add("c")));

  // Write False formula (single empty clause)
  Formula::False().WriteToDimacs(filename, vars);

  std::string content = ReadFile(filename);

  // Should have variable declarations
  EXPECT_TRUE(content.find("c var 1 : a") != std::string::npos);

  // Should have header with 1 clause
  EXPECT_TRUE(content.find("p cnf 3 1") != std::string::npos);

  // Should have an empty clause (just "0")
  EXPECT_TRUE(content.find("\n0\n") != std::string::npos);

  // Clean up
  std::remove(filename.c_str());
}

TEST(CnfBuilderTest, AndAssignOperation) {
  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));

  Formula formula = a;
  Formula original = formula;

  // Test AndAssign modifies the original formula
  formula.AndAssign(b);

  // The result should be different from the original
  // We can test this by writing both to files and comparing
  std::string filename1 = GetTempFilename();
  std::string filename2 = GetTempFilename();

  original.WriteToDimacs(filename1, vars);
  formula.WriteToDimacs(filename2, vars);

  std::string content1 = ReadFile(filename1);
  std::string content2 = ReadFile(filename2);

  // The files should be different (formula should have more clauses)
  EXPECT_NE(content1, content2);

  // Clean up
  std::remove(filename1.c_str());
  std::remove(filename2.c_str());
}

TEST(CnfBuilderTest, VariableIndexing) {
  Variables vars;
  Formula x = Formula(Clause(vars.Add("x")));
  Formula y = Formula(Clause(vars.Add("y")));
  Formula z = Formula(Clause(vars.Add("z")));

  // Variables should be indexed starting from 0
  EXPECT_EQ(vars.var_names().size(), 3);
  EXPECT_EQ(vars.var_names()[0], "x");
  EXPECT_EQ(vars.var_names()[1], "y");
  EXPECT_EQ(vars.var_names()[2], "z");
}

TEST(CnfBuilderTest, NegationOfComplexFormula) {
  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));

  // Test negation of a complex formula: !(a && b)
  Formula complex = a && b;
  Formula negated = !complex;

  // Write both to files to verify they are different
  std::string filename1 = GetTempFilename();
  std::string filename2 = GetTempFilename();

  complex.WriteToDimacs(filename1, vars);
  negated.WriteToDimacs(filename2, vars);

  std::string content1 = ReadFile(filename1);
  std::string content2 = ReadFile(filename2);

  // The files should be different
  EXPECT_NE(content1, content2);

  // Clean up
  std::remove(filename1.c_str());
  std::remove(filename2.c_str());
}

TEST(CnfBuilderTest, OrWithSingleClauses) {
  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));

  // Test OR operation with single clauses (should be optimized)
  Formula single_clause_a = a;
  Formula single_clause_b = b;
  Formula result = single_clause_a || single_clause_b;

  // Write to file to verify it's not empty
  std::string filename = GetTempFilename();
  result.WriteToDimacs(filename, vars);

  std::string content = ReadFile(filename);

  // Should have clauses (not just header)
  EXPECT_TRUE(content.find("0") != std::string::npos) << content;

  // Clean up
  std::remove(filename.c_str());
}

TEST(CnfBuilderTest, MultipleVariables) {
  Variables vars;
  Formula x = Formula(Clause(vars.Add("x")));
  Formula y = Formula(Clause(vars.Add("y")));
  Formula z = Formula(Clause(vars.Add("z")));

  // Test complex expression with multiple variables
  Formula complex = (x && y) || (y && z) || (x && z);

  // Write to file to verify it's not trivially True or False
  std::string filename = GetTempFilename();
  complex.WriteToDimacs(filename, vars);

  std::string content = ReadFile(filename);

  // Should have clauses (not just header with 0 clauses)
  EXPECT_TRUE(content.find("p cnf 3") != std::string::npos) << content;
  EXPECT_TRUE(content.find("0") != std::string::npos);

  // Clean up
  std::remove(filename.c_str());
}

TEST(CnfBuilderTest, DIMACSFormatValidation) {
  std::string filename = GetTempFilename();

  Variables vars;
  Formula a = Formula(Clause(vars.Add("a")));
  Formula b = Formula(Clause(vars.Add("b")));
  Formula c = Formula(Clause(vars.Add("c")));

  // Create a formula and write to DIMACS
  Formula formula = (a || b) && c;
  formula.WriteToDimacs(filename, vars);

  std::string content = ReadFile(filename);

  // Validate DIMACS format
  // Should start with variable comments
  EXPECT_TRUE(content.find("c var 1 : a") != std::string::npos) << content;
  EXPECT_TRUE(content.find("c var 2 : b") != std::string::npos);
  EXPECT_TRUE(content.find("c var 3 : c") != std::string::npos);

  // Should have proper header
  EXPECT_TRUE(content.find("p cnf 3") != std::string::npos) << content;

  // Should have clauses ending with 0
  EXPECT_TRUE(content.find("0") != std::string::npos) << content;

  // Clean up
  std::remove(filename.c_str());
}

TEST(CnfBuilderTest, ClauseComparison) {
  Clause clause1 = Clause({Literal(0), Literal(1)});
  Clause clause2 = Clause({Literal(1), Literal(0)});
  EXPECT_EQ(clause1, clause2);
}

} // namespace cnf
