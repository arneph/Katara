//
//  stack_test.cc
//  Katara
//
//  Created by Arne Philipeit on 8/13/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/interpreter/stack.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/checker/checker.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/parse.h"

using testing::IsEmpty;
using testing::SizeIs;

TEST(StackTest, HandlesStackFramesCorrectly) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 (%0:u8, %1:u8) => (u8) {
{0}
  %2:u8 = iadd %0, %1
  ret %2
}

@1 (%0:u8) => (u8) {
{0}
  %1:u8 = isub %0, #1:u8
  %2:u8 = isub %0, #2:u8
  %3:u8 = call @1, %1
  %4:u8 = call @1, %2
  %5:u8 = call @0, %3, %4
  ret %5
}

)ir");
  ir_checker::AssertProgramIsOkay(program.get());

  ir::Func* func_a = program->GetFunc(0);
  ir::Func* func_b = program->GetFunc(1);

  ir_interpreter::Stack stack;

  EXPECT_EQ(stack.depth(), 0);
  EXPECT_THAT(stack.frames(), SizeIs(0));
  EXPECT_EQ(stack.current_frame(), nullptr);

  stack.PushFrame(func_a);

  EXPECT_EQ(stack.depth(), 1);
  EXPECT_THAT(stack.frames(), SizeIs(1));
  ASSERT_NE(stack.current_frame(), nullptr);

  ir_interpreter::StackFrame* frame_a = stack.current_frame();

  EXPECT_EQ(frame_a->parent(), nullptr);
  EXPECT_EQ(frame_a->func(), func_a);
  EXPECT_TRUE(frame_a->exec_point().is_at_block_entry());
  EXPECT_FALSE(frame_a->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_a->computed_values(), IsEmpty());

  frame_a->computed_values().insert({0, ir::ToIntConstant(common::Int(40))});
  frame_a->computed_values().insert({1, ir::ToIntConstant(common::Int(39))});
  frame_a->exec_point().AdvanceToNextInstr();
  frame_a->computed_values().insert({2, ir::ToIntConstant(common::Int(38))});
  frame_a->exec_point().AdvanceToNextInstr();

  EXPECT_FALSE(frame_a->exec_point().is_at_block_entry());
  EXPECT_TRUE(frame_a->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_a->computed_values(), SizeIs(3));

  stack.PushFrame(func_a);

  EXPECT_EQ(stack.depth(), 2);
  EXPECT_THAT(stack.frames(), SizeIs(2));
  ASSERT_NE(stack.current_frame(), nullptr);

  ir_interpreter::StackFrame* frame_b = stack.current_frame();

  EXPECT_NE(frame_a, frame_b);
  EXPECT_EQ(frame_b->parent(), frame_a);
  EXPECT_EQ(frame_b->func(), func_a);
  EXPECT_TRUE(frame_b->exec_point().is_at_block_entry());
  EXPECT_FALSE(frame_b->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_b->computed_values(), IsEmpty());

  EXPECT_EQ(frame_a->parent(), nullptr);
  EXPECT_FALSE(frame_a->exec_point().is_at_block_entry());
  EXPECT_TRUE(frame_a->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_a->computed_values(), SizeIs(3));

  frame_b->computed_values().insert({0, ir::ToIntConstant(common::Int(25))});
  frame_b->computed_values().insert({5, ir::ToIntConstant(common::Int(17))});
  frame_b->exec_point().AdvanceToFuncExit({frame_b->computed_values().at(5)});

  EXPECT_FALSE(frame_b->exec_point().is_at_block_entry());
  EXPECT_TRUE(frame_b->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_b->computed_values(), SizeIs(2));

  EXPECT_FALSE(frame_a->exec_point().is_at_block_entry());
  EXPECT_TRUE(frame_a->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_a->computed_values(), SizeIs(3));

  stack.PopCurrentFrame();

  EXPECT_EQ(stack.depth(), 1);
  EXPECT_THAT(stack.frames(), SizeIs(1));
  EXPECT_EQ(stack.current_frame(), frame_a);

  EXPECT_EQ(frame_a->parent(), nullptr);
  EXPECT_EQ(frame_a->func(), func_a);
  EXPECT_FALSE(frame_a->exec_point().is_at_block_entry());
  EXPECT_TRUE(frame_a->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_a->computed_values(), SizeIs(3));

  frame_a->exec_point().AdvanceToNextInstr();
  frame_a->computed_values().insert({3, ir::ToIntConstant(common::Int(37))});
  frame_a->exec_point().AdvanceToNextInstr();
  frame_a->computed_values().insert({4, ir::ToIntConstant(common::Int(36))});
  frame_a->exec_point().AdvanceToNextInstr();

  stack.PushFrame(func_b);

  EXPECT_EQ(stack.depth(), 2);
  EXPECT_THAT(stack.frames(), SizeIs(2));
  ASSERT_NE(stack.current_frame(), nullptr);

  ir_interpreter::StackFrame* frame_c = stack.current_frame();

  EXPECT_NE(frame_a, frame_c);
  EXPECT_EQ(frame_c->parent(), frame_a);
  EXPECT_EQ(frame_c->func(), func_b);
  EXPECT_TRUE(frame_c->exec_point().is_at_block_entry());
  EXPECT_FALSE(frame_c->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_c->computed_values(), IsEmpty());

  EXPECT_EQ(frame_a->parent(), nullptr);
  EXPECT_FALSE(frame_a->exec_point().is_at_block_entry());
  EXPECT_FALSE(frame_a->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_a->computed_values(), SizeIs(5));

  frame_c->computed_values().insert({0, ir::ToIntConstant(common::Int(111))});
  frame_c->computed_values().insert({1, ir::ToIntConstant(common::Int(222))});
  frame_c->exec_point().AdvanceToNextInstr();
  frame_c->computed_values().insert({2, ir::ToIntConstant(common::Int(77))});
  frame_c->exec_point().AdvanceToFuncExit({frame_c->computed_values().at(2)});

  EXPECT_FALSE(frame_c->exec_point().is_at_block_entry());
  EXPECT_TRUE(frame_c->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_c->computed_values(), SizeIs(3));

  EXPECT_FALSE(frame_a->exec_point().is_at_block_entry());
  EXPECT_FALSE(frame_a->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_a->computed_values(), SizeIs(5));

  stack.PopCurrentFrame();

  EXPECT_EQ(stack.depth(), 1);
  EXPECT_THAT(stack.frames(), SizeIs(1));
  EXPECT_EQ(stack.current_frame(), frame_a);

  EXPECT_EQ(frame_a->parent(), nullptr);
  EXPECT_EQ(frame_a->func(), func_a);
  EXPECT_FALSE(frame_a->exec_point().is_at_block_entry());
  EXPECT_FALSE(frame_a->exec_point().is_at_func_exit());
  EXPECT_THAT(frame_a->computed_values(), SizeIs(5));

  frame_a->exec_point().AdvanceToNextInstr();
  frame_a->computed_values().insert({5, ir::ToIntConstant(common::Int(35))});
  frame_a->exec_point().AdvanceToFuncExit({frame_a->computed_values().at(5)});

  stack.PopCurrentFrame();

  EXPECT_EQ(stack.depth(), 0);
  EXPECT_THAT(stack.frames(), IsEmpty());
  EXPECT_EQ(stack.current_frame(), nullptr);
}
