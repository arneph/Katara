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
#include "src/ir/serialization/parse.h"
#include "src/lang/processors/ir/checker/checker.h"
#include "src/lang/processors/ir/serialization/func_parser.h"

TEST(SharedToUniquePointerOptimizerTest, OptimizesSmallProgram) {
  std::unique_ptr<ir::Program> optimized_program =
      ir_serialization::ParseProgram<lang::ir_serialization::FuncParser>(R"ir(
@0 main() => (i64) {
  {0}
    %0:lshared_ptr<i64, s> = make_shared
    store %0, #0:i64
    %1:lshared_ptr<i64, s> = make_shared
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
)ir");
  std::unique_ptr<ir::Program> expected_program =
      ir_serialization::ParseProgram<lang::ir_serialization::FuncParser>(R"ir(
@0 main() => (i64) {
  {0}
    %0:lunique_ptr<i64> = make_unique
    store %0, #0:i64
    %1:lunique_ptr<i64> = make_unique
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
  )ir");
  lang::ir_checker::AssertProgramIsOkay(optimized_program.get());
  lang::ir_checker::AssertProgramIsOkay(expected_program.get());

  lang::ir_optimizers::ConvertSharedToUniquePointersInProgram(optimized_program.get());
  lang::ir_checker::AssertProgramIsOkay(optimized_program.get());
  EXPECT_TRUE(ir::IsEqual(optimized_program.get(), expected_program.get()));
}
