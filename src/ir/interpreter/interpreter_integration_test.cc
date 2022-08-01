//
//  interpreter_integration_test.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 6/17/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "src/common/filesystem/filesystem.h"
#include "src/common/filesystem/test_filesystem.h"
#include "src/ir/interpreter/interpreter.h"
#include "src/lang/processors/ir/builder/ir_builder.h"
#include "src/lang/processors/ir/checker/checker.h"
#include "src/lang/processors/ir/lowerers/shared_pointer_lowerer.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"

namespace {

struct ConstantIntExprTestCase {
  const std::string expr;
  const int expected_value;
};

std::ostream& operator<<(std::ostream& os, const ConstantIntExprTestCase& test_case) {
  return os << "TestCase{\"" << test_case.expr << "\", " << test_case.expected_value << "}";
}

class ConstantIntExprTest : public testing::TestWithParam<ConstantIntExprTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    ConstantIntExprTestInstance, ConstantIntExprTest,
    testing::Values(ConstantIntExprTestCase{"42 + 24", 66}, ConstantIntExprTestCase{"42 - 24", 18},
                    ConstantIntExprTestCase{"42 * 24", 1008}, ConstantIntExprTestCase{"42 / 24", 1},
                    ConstantIntExprTestCase{"42 % 24", 18}, ConstantIntExprTestCase{"42 & 24", 8},
                    ConstantIntExprTestCase{"42 | 24", 58}, ConstantIntExprTestCase{"42 ^ 24", 50},
                    ConstantIntExprTestCase{"42 &^ 24", 34},
                    ConstantIntExprTestCase{"42 << 2", 168},
                    ConstantIntExprTestCase{"42 >> 2", 10}));

TEST_P(ConstantIntExprTest, HandlesBinaryOpsWithConstantOperands) {
  std::string source = R"kat(
package main

func main() int {
  return )kat" + GetParam().expr +
                       R"kat(
}
  )kat";
  common::TestFilesystem filesystem_;
  filesystem_.WriteContentsOfFile("expr.kat", source);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"", /*src_path=*/"");

  // Load main package:
  lang::packages::Package* pkg = pkg_manager.LoadMainPackage("/");
  EXPECT_TRUE(pkg_manager.issue_tracker()->issues().empty());
  EXPECT_TRUE(pkg != nullptr);
  EXPECT_TRUE(pkg->issue_tracker().issues().empty());

  // Generate IR:
  std::unique_ptr<ir::Program> program =
      lang::ir_builder::IRBuilder::TranslateProgram(pkg, pkg_manager.type_info());
  EXPECT_TRUE(program != nullptr);
  ::lang::ir_checker::AssertProgramIsOkay(program.get());

  // Interpret IR:
  ir_interpreter::Interpreter interpreter(program.get(), /*sanitize=*/true);
  interpreter.run();

  EXPECT_EQ(interpreter.exit_code(), GetParam().expected_value);
}

TEST(ConstantIntExprTest, HandlesComparisonWithBinaryOp) {
  std::string source = R"kat(
package main

func main() int {
  if 3 % 2 == 3 {
    return 123
  } else if 3 % 2 == 2 {
    return 234
  } else {
    return 345
  }
}
  )kat";
  common::TestFilesystem filesystem_;
  filesystem_.WriteContentsOfFile("test.kat", source);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"", /*src_path=*/"");

  // Load main package:
  lang::packages::Package* pkg = pkg_manager.LoadMainPackage("/");
  EXPECT_TRUE(pkg_manager.issue_tracker()->issues().empty());
  EXPECT_TRUE(pkg != nullptr);
  EXPECT_TRUE(pkg->issue_tracker().issues().empty());

  // Generate IR:
  std::unique_ptr<ir::Program> program =
      lang::ir_builder::IRBuilder::TranslateProgram(pkg, pkg_manager.type_info());
  EXPECT_TRUE(program != nullptr);
  ::lang::ir_checker::AssertProgramIsOkay(program.get());

  // Interpret IR:
  ir_interpreter::Interpreter interpreter(program.get(), /*sanitize=*/true);
  interpreter.run();

  EXPECT_EQ(interpreter.exit_code(), 345);
}

TEST(LoopTest, SumsIntegersZeroThroughNine) {
  std::string source = R"kat(
package main

func main() int {
        var sum int
        for i := 0; i < 10; i++ {
                sum += i
        }
        return sum
}
  )kat";
  common::TestFilesystem filesystem_;
  filesystem_.WriteContentsOfFile("test.kat", source);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"", /*src_path=*/"");

  // Load main package:
  lang::packages::Package* pkg = pkg_manager.LoadMainPackage("/");
  EXPECT_TRUE(pkg_manager.issue_tracker()->issues().empty());
  EXPECT_TRUE(pkg != nullptr);
  EXPECT_TRUE(pkg->issue_tracker().issues().empty());

  // Generate IR:
  std::unique_ptr<ir::Program> program =
      lang::ir_builder::IRBuilder::TranslateProgram(pkg, pkg_manager.type_info());
  EXPECT_TRUE(program != nullptr);
  ::lang::ir_checker::AssertProgramIsOkay(program.get());

  // Lower IR:
  lang::ir_lowerers::LowerSharedPointersInProgram(program.get());

  // Interpret IR:
  ir_interpreter::Interpreter interpreter(program.get(), /*sanitize=*/true);
  interpreter.run();

  EXPECT_EQ(interpreter.exit_code(), 45);
}

}  // namespace
