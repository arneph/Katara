//
//  shared_pointer_lowerer_test.cc
//  Katara
//
//  Created by Arne Philipeit on 6/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/lang/processors/ir/lowerers/shared_pointer_lowerer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/print.h"
#include "src/lang/processors/ir/checker/checker.h"
#include "src/lang/processors/ir/serialization/parse.h"

TEST(SharedPointerLowererTest, LowersSimpleProgram) {
  std::unique_ptr<ir::Program> lowered_program = lang::ir_serialization::ParseProgram(R"ir(
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
)ir");
  std::unique_ptr<ir::Program> expected_program = lang::ir_serialization::ParseProgram(R"ir(
@0 main () => (i64) {
{0}
  %10:ptr, %11:ptr = call @1, #8:i64, @-1
  store %11, #0:i64
  %12:ptr, %13:ptr = call @1, #8:i64, @-1
  store %13, #0:i64
  jmp {1}
{1}
  %2:i64 = load %13
  %3:b = ilss %2, #10:i64
  jcc %3, {4}, {3}
{2}
  %4:i64 = load %13
  %5:i64 = iadd %4, #1:i64
  store %13, %5
  jmp {1}
{3}
  call @4, %12
  %9:i64 = load %11
  call @4, %10
  ret %9
{4}
  %6:i64 = load %13
  %7:i64 = load %11
  %8:i64 = iadd %7, %6
  store %11, %8
  jmp {2}
}
@1 make_shared (%0:i64, %1:func) => (ptr, ptr) {
{0}
  %2:i64 = iadd #24:i64, %0
  %3:ptr = malloc %2
  store %3, #1:i64
  %4:ptr = poff %3, #8:i64
  store %4, #0:i64
  %5:ptr = poff %3, #16:i64
  store %5, %1
  %6:ptr = poff %3, #24:i64
  ret %3, %6
}

@2 strong_copy_shared (%0:ptr, %1:ptr, %2:i64) => (ptr) {
{0}
  %3:i64 = load %0
  %4:i64 = iadd %3, #1:i64
  store %0, %4
  %5:ptr = poff %1, %2
  ret %5
}

@3 weak_copy_shared (%0:ptr, %1:ptr, %2:i64) => (ptr) {
{0}
  %3:ptr = poff %0, #8:i64
  %4:i64 = load %3
  %5:i64 = iadd %4, #1:i64
  store %3, %5
  %6:ptr = poff %1, %2
  ret %6
}

@4 delete_strong_shared (%0:ptr) => () {
{0}
  %1:i64 = load %0
  %2:b = ieq %1, #1:i64
  jcc %2, {2}, {1}
{1}
  %3:i64 = isub %1, #1:i64
  store %0, %3
  ret
{2}
  %4:ptr = poff %0, #16:i64
  %5:func = load %4
  %6:b = niltest %5
  jcc %6, {4}, {3}
{3}
  %7:ptr = poff %0, #24:i64
  call %5, %7
  jmp {4}
{4}
  %8:ptr = poff %0, #8:i64
  %9:i64 = load %8
  %10:b = ieq %9, #0:i64
  jcc %10, {6}, {5}
{5}
  ret
{6}
  free %0
  ret
}

@5 delete_weak_shared (%0:ptr) => () {
{0}
  %1:ptr = poff %0, #8:i64
  %2:i64 = load %1
  %3:b = ieq %2, #1:i64
  jcc %3, {2}, {1}
{1}
  %4:i64 = isub %2, #1:i64
  store %1, %4
  ret
{2}
  %5:i64 = load %0
  %6:b = ieq %5, #0:i64
  jcc %6, {4}, {3}
{3}
  ret
{4}
  free %0
  ret
}

@6 validate_weak_shared (%0:ptr) => () {
{0}
  %1:i64 = load %0
  %2:b = ieq %1, #0:i64
  jcc %2, {2}, {1}
{1}
  ret
{2}
  panic "0x600000210e18"
}
)ir");
  lang::ir_checker::AssertProgramIsOkay(lowered_program.get());
  lang::ir_checker::AssertProgramIsOkay(expected_program.get());

  lang::ir_lowerers::LowerSharedPointersInProgram(lowered_program.get());
  lang::ir_checker::AssertProgramIsOkay(lowered_program.get());
  EXPECT_TRUE(ir::IsEqual(lowered_program->GetFunc(0), expected_program->GetFunc(0)))
      << "Expected different lowered function, got:\n"
      << ir_serialization::Print(lowered_program->GetFunc(0)) << "\nexpected:\n"
      << ir_serialization::Print(expected_program->GetFunc(0));
}
