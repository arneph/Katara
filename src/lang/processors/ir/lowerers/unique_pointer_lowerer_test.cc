//
//  unique_pointer_lowerer_test.cc
//  Katara
//
//  Created by Arne Philipeit on 6/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/lang/processors/ir/lowerers/unique_pointer_lowerer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/print.h"
#include "src/lang/processors/ir/check/check_test_util.h"
#include "src/lang/processors/ir/serialization/parse.h"

TEST(UniquePointerLowererTest, LowersSimpleProgram) {
  std::unique_ptr<ir::Program> lowered_program = lang::ir_serialization::ParseProgramOrDie(R"ir(
@0 main() => (i16) {
{0}
  %0:lunique_ptr<i16> = make_unique #1:i64
  store %0, #123:i16
  %1:i16 = load %0
  %2:i16 = iadd %1, #42:i16
  store %0, %2
  %3:i16 = load %0
  delete_unique %0
  ret %3
}
)ir");
  std::unique_ptr<ir::Program> expected_program = lang::ir_serialization::ParseProgramOrDie(R"ir(
@0 main () => (i16) {
{0}
  %0:ptr = malloc #8:i64
  store %0, #123:i16
  %1:i16 = load %0
  %2:i16 = iadd %1, #42:i16
  store %0, %2
  %3:i16 = load %0
  free %0
  ret %3
}
)ir");
  lang::ir_check::CheckProgramOrDie(lowered_program.get());
  lang::ir_check::CheckProgramOrDie(expected_program.get());

  lang::ir_lowerers::LowerUniquePointersInProgram(lowered_program.get());
  lang::ir_check::CheckProgramOrDie(lowered_program.get());
  EXPECT_TRUE(ir::IsEqual(lowered_program.get(), expected_program.get()))
      << "Expected different lowered program:\n"
      << ir_serialization::Print(expected_program.get()) << "\ngot:\n"
      << ir_serialization::Print(lowered_program.get());
}
