//
//  interpreter_test.cc
//  Katara
//
//  Created by Arne Philipeit on 8/14/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/interpreter/interpreter.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/check/check_test_util.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/parse.h"

struct InterpreterTestParams {
  std::string program;
  int64_t expected_exit_code;
};

class InterpreterTest : public testing::TestWithParam<InterpreterTestParams> {};

INSTANTIATE_TEST_SUITE_P(InterpreterTestInstance, InterpreterTest,
                         testing::Values(
                             InterpreterTestParams{
                                 .program =
                                     R"ir(
@0 main() => (i64) {
  {0}
    ret #123:i64
}
)ir",
                                 .expected_exit_code = 123,
                             },
                             InterpreterTestParams{
                                 .program =
                                     R"ir(
@0 main() => (i64) {
  {0}
    jmp {1}
  {1}
    %0:i64 = phi %3{2}, #0{0}
    %1:i64 = phi %4{2}, #0{0}
    %2:b = ilss %0, #10:i64
    jcc %2, {2}, {3}
  {2}
    %3:i64 = iadd %0, #1:i64
    %4:i64 = iadd %0, %1
    jmp {1}
  {3}
    ret %1
}
)ir",
                                 .expected_exit_code = 45,
                             },
                             InterpreterTestParams{
                                 .program =
                                     R"ir(
@0 main() => (i64) {
  {0}
    %0:i64 = call @1, #10:i64
    ret %0
}

@1 fib(%0:i64) => (i64) {
  {0}
    %1:b = ilss %0, #2:i64
    jcc %1, {1}, {2}
  {1}
    ret #1:i64
  {2}
    %2:i64 = isub %0, #1:i64
    %3:i64 = call @1, %2
    %4:i64 = isub %0, #2:i64
    %5:i64 = call @1, %4
    %6:i64 = iadd %3, %5
    ret %6
}

)ir",
                                 .expected_exit_code = 89,
                             }));

TEST_P(InterpreterTest, InterpretsCorrectlyWithoutSanityCheck) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgramOrDie(GetParam().program);
  program->set_entry_func_num(0);

  ir_check::CheckProgramOrDie(program.get());
  ir_interpreter::Interpreter interpreter(program.get(), /*sanitize=*/false);
  interpreter.Run();

  EXPECT_EQ(interpreter.exit_code(), GetParam().expected_exit_code);
}

TEST_P(InterpreterTest, InterpretsCorrectlyWithSanityCheck) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgramOrDie(GetParam().program);
  program->set_entry_func_num(0);

  ir_check::CheckProgramOrDie(program.get());
  ir_interpreter::Interpreter interpreter(program.get(), /*sanitize=*/true);
  interpreter.Run();

  EXPECT_EQ(interpreter.exit_code(), GetParam().expected_exit_code);
}
