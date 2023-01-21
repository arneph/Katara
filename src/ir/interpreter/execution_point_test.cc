//
//  execution_point_test.cc
//  Katara
//
//  Created by Arne Philipeit on 8/13/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/interpreter/execution_point.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/check/check_test_util.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/parse.h"

using testing::SizeIs;

TEST(ExeuctionPointTest, AdvancesCorrectly) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgramOrDie(R"ir(
@0 () => (i64) {
{0}
  jmp {1}
{1}
  %0:i64 = phi #0:i64{0}, %3{2}
  %1:i64 = phi #0:i64{0}, %4{2}
  %2:b = ilss %0, #10:i64
  jcc %2, {2}, {3}
{2}
  %3:i64 = iadd %0, #1:i64
  %4:i64 = iadd %1, %3
  jmp {1}
{3}
  ret %1
}
)ir");
  ir_check::CheckProgramOrDie(program.get());

  ir::Func* func = program->GetFunc(0);
  ir::Block* block_a = func->GetBlock(0);
  ir::Block* block_b = func->GetBlock(1);
  ir::Block* block_d = func->GetBlock(3);

  ir_interpreter::ExecutionPoint exec_point = ir_interpreter::ExecutionPoint::AtFuncEntry(func);

  EXPECT_TRUE(exec_point.is_at_block_entry());
  EXPECT_FALSE(exec_point.is_at_func_exit());
  EXPECT_EQ(exec_point.previous_block(), nullptr);
  EXPECT_EQ(exec_point.current_block(), block_a);
  EXPECT_EQ(exec_point.next_instr(), block_a->instrs().at(0).get());

  exec_point.AdvanceToNextBlock(block_b);

  EXPECT_TRUE(exec_point.is_at_block_entry());
  EXPECT_FALSE(exec_point.is_at_func_exit());
  EXPECT_EQ(exec_point.previous_block(), block_a);
  EXPECT_EQ(exec_point.current_block(), block_b);
  EXPECT_EQ(exec_point.next_instr(), block_b->instrs().at(0).get());

  exec_point.AdvanceToNextInstr();

  EXPECT_FALSE(exec_point.is_at_block_entry());
  EXPECT_FALSE(exec_point.is_at_func_exit());
  EXPECT_EQ(exec_point.previous_block(), block_a);
  EXPECT_EQ(exec_point.current_block(), block_b);
  EXPECT_EQ(exec_point.next_instr(), block_b->instrs().at(1).get());

  exec_point.AdvanceToNextInstr();

  EXPECT_FALSE(exec_point.is_at_block_entry());
  EXPECT_FALSE(exec_point.is_at_func_exit());
  EXPECT_EQ(exec_point.previous_block(), block_a);
  EXPECT_EQ(exec_point.current_block(), block_b);
  EXPECT_EQ(exec_point.next_instr(), block_b->instrs().at(2).get());

  exec_point.AdvanceToNextInstr();

  EXPECT_FALSE(exec_point.is_at_block_entry());
  EXPECT_FALSE(exec_point.is_at_func_exit());
  EXPECT_EQ(exec_point.previous_block(), block_a);
  EXPECT_EQ(exec_point.current_block(), block_b);
  EXPECT_EQ(exec_point.next_instr(), block_b->instrs().at(3).get());

  exec_point.AdvanceToNextBlock(block_d);

  EXPECT_TRUE(exec_point.is_at_block_entry());
  EXPECT_FALSE(exec_point.is_at_func_exit());
  EXPECT_EQ(exec_point.previous_block(), block_b);
  EXPECT_EQ(exec_point.current_block(), block_d);
  EXPECT_EQ(exec_point.next_instr(), block_d->instrs().at(0).get());

  exec_point.AdvanceToFuncExit(
      std::vector<std::shared_ptr<ir::Constant>>{ir::ToIntConstant(common::Int(int64_t{55}))});

  EXPECT_FALSE(exec_point.is_at_block_entry());
  EXPECT_TRUE(exec_point.is_at_func_exit());
  EXPECT_EQ(exec_point.previous_block(), block_b);
  EXPECT_EQ(exec_point.current_block(), block_d);
  EXPECT_EQ(exec_point.next_instr(), nullptr);
  ASSERT_THAT(exec_point.results(), SizeIs(1));
  EXPECT_TRUE(ir::IsEqual(exec_point.results().at(0).get(),
                          ir::ToIntConstant(common::Int(int64_t{55})).get()));
}
