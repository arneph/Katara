//
//  shared_to_unique_pointer_optimizer_test.cc
//  Katara
//
//  Created by Arne Philipeit on 6/4/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/lang/processors/ir/optimizers/shared_to_unique_pointer_optimizer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/print.h"
#include "src/lang/processors/ir/check/check_test_util.h"
#include "src/lang/processors/ir/serialization/parse.h"

class SharedToUniquePointerOptimizationImpossibleTest : public testing::TestWithParam<std::string> {
};

INSTANTIATE_TEST_SUITE_P(SharedToUniquePointerOptimizationImpossibleTestInstance,
                         SharedToUniquePointerOptimizationImpossibleTest,
                         testing::Values(R"ir(
@0 main() => () {
  {0}
    %0:lunique_ptr<i8> = make_unique #1:i64
    delete_unique %0
    ret
}
)ir",
                                         R"ir(
@0 main() => () {
  {0}
    %0:lshared_ptr<i8, s> = make_shared #1:i64
    %1:lshared_ptr<i8, s> = copy_shared %0, #0:i64
    delete_shared %0
    delete_shared %1
    ret
}
)ir",
                                         R"ir(
 @0 main(%0:lshared_ptr<i8, s>) => () {
   {0}
    delete_shared %0
    ret
 }
)ir",
                                         R"ir(
 @0 main(%0:ptr) => () {
   {0}
     %1:lshared_ptr<i8, s> = make_shared #1:i64
    store %0, %1
    ret
 }
)ir",
                                         R"ir(
@0 creator() => (lshared_ptr<i8, s>) {
  {0}
    %0:lshared_ptr<i8, s> = make_shared #1:i64
    ret %0
}

@1 main() => () {
  {0}
    %0:lshared_ptr<i8, s> = call @0
    delete_shared %0
    ret
}
)ir",
                                         R"ir(
@0 main(%0:b) => () {
  {0}
    %1:lshared_ptr<i8, s> = make_shared #1:i64
    jcc %0, {1}, {2}
  {1}
    %2:lshared_ptr<i8, s> = make_shared #1:i64
    jmp {2}
  {2}
    %3:lshared_ptr<i8, s> = phi %1{0}, %2{1}
    delete_shared %3
    ret
}
)ir"));

TEST_P(SharedToUniquePointerOptimizationImpossibleTest, DoesNotOptimizeProgram) {
  std::unique_ptr<ir::Program> input_program =
      lang::ir_serialization::ParseProgramOrDie(GetParam());
  std::unique_ptr<ir::Program> expected_program =
      lang::ir_serialization::ParseProgramOrDie(GetParam());
  lang::ir_check::CheckProgramOrDie(expected_program.get());

  lang::ir_optimizers::ConvertSharedToUniquePointersInProgram(input_program.get());
  lang::ir_check::CheckProgramOrDie(input_program.get());
  EXPECT_TRUE(ir::IsEqual(input_program.get(), expected_program.get()))
      << "Expected program to stay unoptimized, got:\n"
      << ir_serialization::Print(input_program.get()) << "\nexpected:\n"
      << ir_serialization::Print(expected_program.get());
}

struct PossibleOptimizationTestParams {
  std::string input_program;
  std::string expected_program;
};

class SharedToUniquePointerOptimizationPossibleTest
    : public testing::TestWithParam<PossibleOptimizationTestParams> {};

INSTANTIATE_TEST_SUITE_P(SharedToUniquePointerOptimizationPossibleTestInstance,
                         SharedToUniquePointerOptimizationPossibleTest,
                         testing::Values(
                             PossibleOptimizationTestParams{
                                 .input_program = R"ir(
@0 main() => () {
  {0}
    %0:lshared_ptr<i8, s> = make_shared #1:i64
    delete_shared %0
    ret
}
)ir",
                                 .expected_program = R"ir(
@0 main() => () {
  {0}
    %0:lunique_ptr<i8> = make_unique #1:i64
    delete_unique %0
    ret
}
)ir",
                             },
                             PossibleOptimizationTestParams{
                                 .input_program = R"ir(
@0 main() => () {
  {0}
    %0:lshared_ptr<i8, s> = make_shared #1:i64
    store %0, #123:i8
    delete_shared %0
    ret
}
)ir",
                                 .expected_program = R"ir(
@0 main() => () {
  {0}
    %0:lunique_ptr<i8> = make_unique #1:i64
    store %0, #123:i8
    delete_unique %0
    ret
}
)ir",
                             },
                             PossibleOptimizationTestParams{
                                 .input_program = R"ir(
@0 main() => (i64) {
  {0}
    %0:lshared_ptr<i64, s> = make_shared #1:i64
    store %0, #0:i64
    %1:lshared_ptr<i64, s> = make_shared #1:i64
    store %1, #0:i64
    jmp {1}
  {1}
    %2:i64 = load %1
    %3:b = ilss %2, #10:i64
    jcc %3, {4}, {3}
  {2}
    %4:i64 = load %1
    %5:i64 = iadd %4, #1:i64
    store %1, %5
    jmp {1}
  {3}
    delete_shared %1
    %9:i64 = load %0
    delete_shared %0
    ret %9
  {4}
    %6:i64 = load %1
    %7:i64 = load %0
    %8:i64 = iadd %7, %6
    store %0, %8
    jmp {2}
}
)ir",
                                 .expected_program = R"ir(
  @0 main() => (i64) {
    {0}
      %0:lunique_ptr<i64> = make_unique #1:i64
      store %0, #0:i64
      %1:lunique_ptr<i64> = make_unique #1:i64
      store %1, #0:i64
      jmp {1}
    {1}
      %2:i64 = load %1
      %3:b = ilss %2, #10:i64
      jcc %3, {4}, {3}
    {2}
      %4:i64 = load %1
      %5:i64 = iadd %4, #1:i64
      store %1, %5
      jmp {1}
    {3}
      delete_unique %1
      %9:i64 = load %0
      delete_unique %0
      ret %9
    {4}
      %6:i64 = load %1
      %7:i64 = load %0
      %8:i64 = iadd %7, %6
      store %0, %8
      jmp {2}
  }
    )ir",
                             }));

TEST_P(SharedToUniquePointerOptimizationPossibleTest, OptimizesProgram) {
  std::unique_ptr<ir::Program> optimized_program =
      lang::ir_serialization::ParseProgramOrDie(GetParam().input_program);
  std::unique_ptr<ir::Program> expected_program =
      lang::ir_serialization::ParseProgramOrDie(GetParam().expected_program);
  lang::ir_check::CheckProgramOrDie(optimized_program.get());
  lang::ir_check::CheckProgramOrDie(expected_program.get());

  lang::ir_optimizers::ConvertSharedToUniquePointersInProgram(optimized_program.get());
  lang::ir_check::CheckProgramOrDie(optimized_program.get());
  EXPECT_TRUE(ir::IsEqual(optimized_program.get(), expected_program.get()))
      << "Expected different optimized program, got:\n"
      << ir_serialization::Print(optimized_program.get()) << "\nexpected:\n"
      << ir_serialization::Print(expected_program.get());
}
