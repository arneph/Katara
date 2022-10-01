//
//  parse_test.cc
//  Katara
//
//  Created by Arne Philipeit on 3/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/serialization/parse.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/checker/checker.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace {

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::IsNull;
using ::testing::Not;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

TEST(ParseTest, ParsesEmptyProgram) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram("");

  EXPECT_THAT(program->funcs(), IsEmpty());
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);
}

TEST(ParseTest, ParsesWhitespaceProgram) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram("\t\n\n    \t\t\t \n");

  EXPECT_THAT(ir_checker::CheckProgram(program.get()), IsEmpty());
  EXPECT_THAT(program->funcs(), IsEmpty());
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);
}

TEST(ParseTest, ParsesProgramWithEmptyFunc) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 () => () {
}
)ir");

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->number(), 0);
  EXPECT_THAT(func->name(), IsEmpty());
  EXPECT_THAT(func->args(), IsEmpty());
  EXPECT_THAT(func->result_types(), IsEmpty());
  EXPECT_THAT(func->blocks(), IsEmpty());
  EXPECT_THAT(func->entry_block(), IsNull());
  EXPECT_EQ(func->entry_block_num(), ir::kNoBlockNum);
  EXPECT_EQ(func->computed_count(), 0);
}

TEST(ParseTest, ParsesProgramWithSimpleFunc) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 () => () {
{0}
  ret
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->number(), 0);
  EXPECT_THAT(func->name(), IsEmpty());
  EXPECT_THAT(func->args(), IsEmpty());
  EXPECT_THAT(func->result_types(), IsEmpty());
  EXPECT_EQ(func->computed_count(), 0);
  ASSERT_THAT(func->blocks(), SizeIs(1));
  ASSERT_THAT(func->blocks().front(), Not(IsNull()));
  ASSERT_TRUE(func->HasBlock(0));

  ir::Block* block = func->GetBlock(0);
  EXPECT_EQ(func->entry_block(), block);
  EXPECT_EQ(func->entry_block_num(), block->number());
  EXPECT_EQ(block->number(), 0);
  EXPECT_THAT(block->name(), IsEmpty());
  ASSERT_THAT(block->instrs(), SizeIs(1));
  ASSERT_THAT(block->instrs().front(), Not(IsNull()));
  EXPECT_TRUE(block->HasControlFlowInstr());
  EXPECT_THAT(block->parents(), IsEmpty());
  EXPECT_THAT(block->children(), IsEmpty());

  ir::Instr* instr = block->instrs().front().get();
  ASSERT_EQ(instr->instr_kind(), ir::InstrKind::kReturn);
  EXPECT_EQ(block->ControlFlowInstr(), instr);

  auto return_instr = static_cast<ir::ReturnInstr*>(instr);
  EXPECT_THAT(return_instr->args(), IsEmpty());
}

TEST(ParseTest, ParsesFuncWithOneResult) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 () => (b) {
{0}
  ret #f
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->number(), 0);
  EXPECT_THAT(func->name(), IsEmpty());
  EXPECT_THAT(func->args(), IsEmpty());
  ASSERT_THAT(func->result_types(), SizeIs(1));
  ASSERT_THAT(func->result_types().front(), Not(IsNull()));
  EXPECT_EQ(func->computed_count(), 0);
  ASSERT_THAT(func->blocks(), SizeIs(1));
  ASSERT_THAT(func->blocks().front(), Not(IsNull()));
  ASSERT_TRUE(func->HasBlock(0));

  const ir::Type* result_type = func->result_types().front();
  EXPECT_EQ(result_type, ir::bool_type());

  ir::Block* block = func->GetBlock(0);
  EXPECT_EQ(func->entry_block(), block);
  EXPECT_EQ(func->entry_block_num(), block->number());
  EXPECT_EQ(block->number(), 0);
  EXPECT_THAT(block->name(), IsEmpty());
  ASSERT_THAT(block->instrs(), SizeIs(1));
  ASSERT_THAT(block->instrs().front(), Not(IsNull()));
  EXPECT_TRUE(block->HasControlFlowInstr());
  EXPECT_THAT(block->parents(), IsEmpty());
  EXPECT_THAT(block->children(), IsEmpty());

  ir::Instr* instr = block->instrs().front().get();
  ASSERT_EQ(instr->instr_kind(), ir::InstrKind::kReturn);
  EXPECT_EQ(block->ControlFlowInstr(), instr);

  auto return_instr = static_cast<ir::ReturnInstr*>(instr);
  ASSERT_THAT(return_instr->args(), SizeIs(1));
  ASSERT_THAT(return_instr->args().front(), Not(IsNull()));

  auto result = return_instr->args().front();
  EXPECT_EQ(result, ir::False());
}

TEST(ParseTest, ParsesFuncWithMultipleResult) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 () => (u32, func, ptr, b) {
{0}
  ret #42:u32, @0, 0x0, #t
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->number(), 0);
  EXPECT_THAT(func->name(), IsEmpty());
  EXPECT_THAT(func->args(), IsEmpty());
  ASSERT_THAT(func->result_types(), SizeIs(4));
  ASSERT_THAT(func->result_types(), Not(Contains(IsNull())));
  EXPECT_EQ(func->computed_count(), 0);
  ASSERT_THAT(func->blocks(), SizeIs(1));
  ASSERT_THAT(func->blocks().front(), Not(IsNull()));
  ASSERT_TRUE(func->HasBlock(0));

  const ir::Type* result_type_a = func->result_types().at(0);
  EXPECT_EQ(result_type_a, ir::u32());
  const ir::Type* result_type_b = func->result_types().at(1);
  EXPECT_EQ(result_type_b, ir::func_type());
  const ir::Type* result_type_c = func->result_types().at(2);
  EXPECT_EQ(result_type_c, ir::pointer_type());
  const ir::Type* result_type_d = func->result_types().at(3);
  EXPECT_EQ(result_type_d, ir::bool_type());

  ir::Block* block = func->GetBlock(0);
  EXPECT_EQ(func->entry_block(), block);
  EXPECT_EQ(func->entry_block_num(), block->number());
  EXPECT_EQ(block->number(), 0);
  EXPECT_THAT(block->name(), IsEmpty());
  ASSERT_THAT(block->instrs(), SizeIs(1));
  ASSERT_THAT(block->instrs().front(), Not(IsNull()));
  EXPECT_TRUE(block->HasControlFlowInstr());
  EXPECT_THAT(block->parents(), IsEmpty());
  EXPECT_THAT(block->children(), IsEmpty());

  ir::Instr* instr = block->instrs().front().get();
  ASSERT_EQ(instr->instr_kind(), ir::InstrKind::kReturn);
  EXPECT_EQ(block->ControlFlowInstr(), instr);

  auto return_instr = static_cast<ir::ReturnInstr*>(instr);
  ASSERT_THAT(return_instr->args(), SizeIs(4));
  ASSERT_THAT(return_instr->args(), Not(Contains(IsNull())));

  auto result_a = return_instr->args().at(0);
  ASSERT_THAT(result_a->type(), ir::u32());
  EXPECT_EQ(static_cast<ir::IntConstant*>(result_a.get())->int_type(), common::IntType::kU32);
  EXPECT_TRUE(common::Int::Compare(static_cast<ir::IntConstant*>(result_a.get())->value(),
                                   common::Int::CompareOp::kEq, common::Int(uint32_t(42))));
  auto result_b = return_instr->args().at(1);
  ASSERT_THAT(result_b->type(), ir::func_type());
  EXPECT_EQ(static_cast<ir::FuncConstant*>(result_b.get())->value(), 0);
  auto result_c = return_instr->args().at(2);
  EXPECT_EQ(result_c, ir::NilPointer());
  auto result_d = return_instr->args().at(3);
  EXPECT_EQ(result_d, ir::True());
}

TEST(ParseTest, ParsesFuncWithOneArgument) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 (%0:i16) => () {
{0}
  ret
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->number(), 0);
  EXPECT_THAT(func->name(), IsEmpty());
  ASSERT_THAT(func->args(), SizeIs(1));
  EXPECT_THAT(func->result_types(), IsEmpty());
  EXPECT_EQ(func->computed_count(), 1);
  ASSERT_THAT(func->blocks(), SizeIs(1));
  ASSERT_THAT(func->blocks().front(), Not(IsNull()));
  ASSERT_TRUE(func->HasBlock(0));

  ir::Computed* arg = func->args().at(0).get();
  EXPECT_EQ(arg->type(), ir::i16());
  EXPECT_EQ(arg->number(), 0);

  ir::Block* block = func->GetBlock(0);
  EXPECT_EQ(func->entry_block(), block);
  EXPECT_EQ(func->entry_block_num(), block->number());
  EXPECT_EQ(block->number(), 0);
  EXPECT_THAT(block->name(), IsEmpty());
  ASSERT_THAT(block->instrs(), SizeIs(1));
  ASSERT_THAT(block->instrs().front(), Not(IsNull()));
  EXPECT_TRUE(block->HasControlFlowInstr());
  EXPECT_THAT(block->parents(), IsEmpty());
  EXPECT_THAT(block->children(), IsEmpty());

  ir::Instr* instr = block->instrs().front().get();
  ASSERT_EQ(instr->instr_kind(), ir::InstrKind::kReturn);
  EXPECT_EQ(block->ControlFlowInstr(), instr);

  auto return_instr = static_cast<ir::ReturnInstr*>(instr);
  EXPECT_THAT(return_instr->args(), IsEmpty());
}

TEST(ParseTest, ParsesFuncWithMultipleArguments) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 (%0:u32, %1:ptr, %2:b) => () {
{0}
  ret
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->number(), 0);
  EXPECT_THAT(func->name(), IsEmpty());
  ASSERT_THAT(func->args(), SizeIs(3));
  EXPECT_THAT(func->result_types(), IsEmpty());
  EXPECT_EQ(func->computed_count(), 3);
  ASSERT_THAT(func->blocks(), SizeIs(1));
  ASSERT_THAT(func->blocks().front(), Not(IsNull()));
  ASSERT_TRUE(func->HasBlock(0));

  ir::Computed* arg_a = func->args().at(0).get();
  EXPECT_EQ(arg_a->type(), ir::u32());
  EXPECT_EQ(arg_a->number(), 0);

  ir::Computed* arg_b = func->args().at(1).get();
  EXPECT_EQ(arg_b->type(), ir::pointer_type());
  EXPECT_EQ(arg_b->number(), 1);

  ir::Computed* arg_c = func->args().at(2).get();
  EXPECT_EQ(arg_c->type(), ir::bool_type());
  EXPECT_EQ(arg_c->number(), 2);

  ir::Block* block = func->GetBlock(0);
  EXPECT_EQ(func->entry_block(), block);
  EXPECT_EQ(func->entry_block_num(), block->number());
  EXPECT_EQ(block->number(), 0);
  EXPECT_THAT(block->name(), IsEmpty());
  ASSERT_THAT(block->instrs(), SizeIs(1));
  ASSERT_THAT(block->instrs().front(), Not(IsNull()));
  EXPECT_TRUE(block->HasControlFlowInstr());
  EXPECT_THAT(block->parents(), IsEmpty());
  EXPECT_THAT(block->children(), IsEmpty());

  ir::Instr* instr = block->instrs().front().get();
  ASSERT_EQ(instr->instr_kind(), ir::InstrKind::kReturn);
  EXPECT_EQ(block->ControlFlowInstr(), instr);

  auto return_instr = static_cast<ir::ReturnInstr*>(instr);
  EXPECT_THAT(return_instr->args(), IsEmpty());
}

TEST(ParseTest, ParsesFuncWithMultipleBlocks) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 (%0:i64, %1:i64, %2:b) => (i64) {
{0}
  jcc %2, {1}, {2}
{1}
  ret %0:i64
{2}
  ret %1:i64
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->number(), 0);
  EXPECT_EQ(func->computed_count(), 3);
  ASSERT_THAT(func->blocks(), SizeIs(3));
  ASSERT_THAT(func->blocks().at(0), Not(IsNull()));
  ASSERT_THAT(func->blocks().at(1), Not(IsNull()));
  ASSERT_THAT(func->blocks().at(2), Not(IsNull()));
  ASSERT_TRUE(func->HasBlock(0));
  ASSERT_TRUE(func->HasBlock(1));
  ASSERT_TRUE(func->HasBlock(2));

  ir::Computed* arg_a = func->args().at(0).get();
  ir::Computed* arg_b = func->args().at(1).get();
  ir::Computed* arg_c = func->args().at(2).get();

  ir::Block* block_a = func->GetBlock(0);
  ir::Block* block_b = func->GetBlock(1);
  ir::Block* block_c = func->GetBlock(2);

  EXPECT_EQ(func->entry_block(), block_a);
  EXPECT_EQ(func->entry_block_num(), block_a->number());

  EXPECT_EQ(block_a->number(), 0);
  EXPECT_EQ(block_b->number(), 1);
  EXPECT_EQ(block_c->number(), 2);
  EXPECT_THAT(block_a->name(), IsEmpty());
  EXPECT_THAT(block_b->name(), IsEmpty());
  EXPECT_THAT(block_c->name(), IsEmpty());
  EXPECT_TRUE(block_a->HasControlFlowInstr());
  EXPECT_TRUE(block_b->HasControlFlowInstr());
  EXPECT_TRUE(block_c->HasControlFlowInstr());

  EXPECT_THAT(block_a->parents(), IsEmpty());
  EXPECT_THAT(block_a->children(), UnorderedElementsAre(1, 2));
  ASSERT_THAT(block_a->instrs(), SizeIs(1));
  ASSERT_THAT(block_a->instrs().front(), Not(IsNull()));
  ASSERT_EQ(block_a->instrs().front()->instr_kind(), ir::InstrKind::kJumpCond);

  auto jump_cond_instr = static_cast<ir::JumpCondInstr*>(block_a->instrs().front().get());
  EXPECT_EQ(jump_cond_instr->condition().get(), arg_c);
  EXPECT_EQ(jump_cond_instr->destination_true(), 1);
  EXPECT_EQ(jump_cond_instr->destination_false(), 2);

  EXPECT_THAT(block_b->parents(), UnorderedElementsAre(0));
  EXPECT_THAT(block_b->children(), IsEmpty());
  ASSERT_THAT(block_b->instrs(), SizeIs(1));
  ASSERT_THAT(block_b->instrs().front(), Not(IsNull()));
  ASSERT_EQ(block_b->instrs().front()->instr_kind(), ir::InstrKind::kReturn);

  auto return_instr_a = static_cast<ir::ReturnInstr*>(block_b->instrs().front().get());
  ASSERT_THAT(return_instr_a->args(), SizeIs(1));
  EXPECT_EQ(return_instr_a->args().front().get(), arg_a);

  EXPECT_THAT(block_c->parents(), UnorderedElementsAre(0));
  EXPECT_THAT(block_c->children(), IsEmpty());
  ASSERT_THAT(block_c->instrs(), SizeIs(1));
  ASSERT_THAT(block_c->instrs().front(), Not(IsNull()));
  ASSERT_EQ(block_c->instrs().front()->instr_kind(), ir::InstrKind::kReturn);

  auto return_instr_b = static_cast<ir::ReturnInstr*>(block_c->instrs().front().get());
  ASSERT_THAT(return_instr_b->args(), SizeIs(1));
  EXPECT_EQ(return_instr_b->args().front().get(), arg_b);
}

TEST(ParseTest, ParsesFuncWithIfStatement) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 (%0:func, %1:func, %2:b) => (func) {
{0}
  jcc %2, {1}, {2}
{1}
  jmp {2}
{2}
  %3:func = phi %0{0}, %1{1}
  ret %3:func
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->computed_count(), 4);

  ir::Computed* arg_a = func->args().at(0).get();
  ir::Computed* arg_b = func->args().at(1).get();

  ir::Block* block_c = func->GetBlock(2);

  auto phi_instr = static_cast<ir::PhiInstr*>(block_c->instrs().at(0).get());
  EXPECT_THAT(phi_instr->args(), SizeIs(2));
  EXPECT_EQ(phi_instr->UsedValues().at(0).get(), arg_a);
  EXPECT_EQ(phi_instr->UsedValues().at(1).get(), arg_b);

  ir::InheritedValue* phi_arg_a = phi_instr->args().at(0).get();
  EXPECT_EQ(phi_arg_a->type(), ir::func_type());
  EXPECT_EQ(phi_arg_a->value().get(), arg_a);
  EXPECT_EQ(phi_arg_a->origin(), 0);

  ir::InheritedValue* phi_arg_b = phi_instr->args().at(1).get();
  EXPECT_EQ(phi_arg_b->type(), ir::func_type());
  EXPECT_EQ(phi_arg_b->value().get(), arg_b);
  EXPECT_EQ(phi_arg_b->origin(), 1);

  auto return_instr = static_cast<ir::ReturnInstr*>(block_c->instrs().at(1).get());
  EXPECT_EQ(phi_instr->result(), return_instr->args().at(0));
}

TEST(ParseTest, ParsesFuncWithForLoop) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 (%0:i64) => (i64) {
{0}
  jmp {1}
{1}
  %1:i64 = phi #1{0}, %5{2}
  %2:i64 = phi #0{0}, %4{2}
  %3:b = ileq %1:i64, %0
  jcc %3, {2}, {3}
{2}
  %4:i64 = iadd %2, %1
  %5:i64 = iadd %1, #1
  jmp {1}
{3}
  ret %2:i64
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->computed_count(), 6);

  ir::Computed* arg = func->args().at(0).get();
  EXPECT_EQ(arg->type(), ir::i64());

  ir::Block* block_b = func->GetBlock(1);
  ir::Block* block_c = func->GetBlock(2);
  ir::Block* block_d = func->GetBlock(3);

  ASSERT_EQ(block_b->instrs().at(0)->instr_kind(), ir::InstrKind::kPhi);
  ASSERT_EQ(block_b->instrs().at(1)->instr_kind(), ir::InstrKind::kPhi);
  ASSERT_EQ(block_b->instrs().at(2)->instr_kind(), ir::InstrKind::kIntCompare);
  ASSERT_EQ(block_b->instrs().at(3)->instr_kind(), ir::InstrKind::kJumpCond);
  ASSERT_EQ(block_c->instrs().at(0)->instr_kind(), ir::InstrKind::kIntBinary);
  ASSERT_EQ(block_c->instrs().at(1)->instr_kind(), ir::InstrKind::kIntBinary);
  ASSERT_EQ(block_d->instrs().at(0)->instr_kind(), ir::InstrKind::kReturn);

  auto phi_instr_a = static_cast<ir::PhiInstr*>(block_b->instrs().at(0).get());
  auto phi_instr_b = static_cast<ir::PhiInstr*>(block_b->instrs().at(1).get());
  auto leq_instr = static_cast<ir::IntCompareInstr*>(block_b->instrs().at(2).get());
  auto jcc_instr = static_cast<ir::JumpCondInstr*>(block_b->instrs().at(3).get());
  auto add_instr_a = static_cast<ir::IntBinaryInstr*>(block_c->instrs().at(0).get());
  auto add_instr_b = static_cast<ir::IntBinaryInstr*>(block_c->instrs().at(1).get());
  auto ret_instr = static_cast<ir::ReturnInstr*>(block_d->instrs().at(0).get());

  ir::Computed* value_a = phi_instr_a->result().get();
  ir::Computed* value_b = phi_instr_b->result().get();
  ir::Computed* value_c = leq_instr->result().get();
  ir::Computed* value_d = add_instr_a->result().get();
  ir::Computed* value_e = add_instr_b->result().get();

  EXPECT_THAT(phi_instr_a->args(), SizeIs(2));
  EXPECT_EQ(phi_instr_a->UsedValues().at(0), ir::I64One());
  EXPECT_EQ(phi_instr_a->UsedValues().at(1).get(), value_e);
  EXPECT_EQ(value_a->type(), ir::i64());

  ir::InheritedValue* phi_arg_a = phi_instr_a->args().at(0).get();
  EXPECT_EQ(phi_arg_a->type(), ir::i64());
  EXPECT_EQ(phi_arg_a->value(), ir::I64One());
  EXPECT_EQ(phi_arg_a->origin(), 0);

  ir::InheritedValue* phi_arg_b = phi_instr_a->args().at(1).get();
  EXPECT_EQ(phi_arg_b->type(), ir::i64());
  EXPECT_EQ(phi_arg_b->value().get(), value_e);
  EXPECT_EQ(phi_arg_b->origin(), 2);

  EXPECT_THAT(phi_instr_b->args(), SizeIs(2));
  EXPECT_EQ(phi_instr_b->UsedValues().at(0), ir::I64Zero());
  EXPECT_EQ(phi_instr_b->UsedValues().at(1).get(), value_d);
  EXPECT_EQ(value_b->type(), ir::i64());

  ir::InheritedValue* phi_arg_c = phi_instr_b->args().at(0).get();
  EXPECT_EQ(phi_arg_c->type(), ir::i64());
  EXPECT_EQ(phi_arg_c->value(), ir::I64Zero());
  EXPECT_EQ(phi_arg_c->origin(), 0);

  ir::InheritedValue* phi_arg_d = phi_instr_b->args().at(1).get();
  EXPECT_EQ(phi_arg_d->type(), ir::i64());
  EXPECT_EQ(phi_arg_d->value().get(), value_d);
  EXPECT_EQ(phi_arg_d->origin(), 2);

  EXPECT_EQ(leq_instr->operation(), common::Int::CompareOp::kLeq);
  EXPECT_EQ(leq_instr->operand_a().get(), value_a);
  EXPECT_EQ(leq_instr->operand_b().get(), arg);
  EXPECT_EQ(value_c->type(), ir::bool_type());

  EXPECT_EQ(jcc_instr->condition().get(), value_c);
  EXPECT_EQ(jcc_instr->destination_true(), 2);
  EXPECT_EQ(jcc_instr->destination_false(), 3);

  EXPECT_EQ(add_instr_a->operation(), common::Int::BinaryOp::kAdd);
  EXPECT_EQ(add_instr_a->operand_a().get(), value_b);
  EXPECT_EQ(add_instr_a->operand_b().get(), value_a);
  EXPECT_EQ(value_d->type(), ir::i64());

  EXPECT_EQ(add_instr_b->operation(), common::Int::BinaryOp::kAdd);
  EXPECT_EQ(add_instr_b->operand_a().get(), value_a);
  EXPECT_EQ(add_instr_b->operand_b(), ir::I64One());
  EXPECT_EQ(value_e->type(), ir::i64());

  EXPECT_THAT(ret_instr->args(), SizeIs(1));
  EXPECT_EQ(ret_instr->args().at(0).get(), value_b);
}

TEST(ParseTest, ParsesFuncWithRecursiveCall) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@42 fib(%0:u64) => (u64) {
{0}
  %1:b = ieq %0:u64, #1
  jcc %1, {1}, {2}
{1}
  ret #1:u64
{2}
  %2:u64 = isub %0, #1
  %3:u64 = call @42, %2:u64
  %4:u64 = imul %3, %0
  ret %4:u64
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  EXPECT_EQ(func->number(), 42);
  EXPECT_EQ(func->name(), "fib");
  EXPECT_EQ(func->computed_count(), 5);

  ir::Computed* arg = func->args().at(0).get();
  EXPECT_EQ(arg->type(), ir::u64());

  ir::Block* block_a = func->GetBlock(0);
  ir::Block* block_b = func->GetBlock(1);
  ir::Block* block_c = func->GetBlock(2);

  ASSERT_EQ(block_a->instrs().at(0)->instr_kind(), ir::InstrKind::kIntCompare);
  ASSERT_EQ(block_a->instrs().at(1)->instr_kind(), ir::InstrKind::kJumpCond);
  ASSERT_EQ(block_b->instrs().at(0)->instr_kind(), ir::InstrKind::kReturn);
  ASSERT_EQ(block_c->instrs().at(0)->instr_kind(), ir::InstrKind::kIntBinary);
  ASSERT_EQ(block_c->instrs().at(1)->instr_kind(), ir::InstrKind::kCall);
  ASSERT_EQ(block_c->instrs().at(2)->instr_kind(), ir::InstrKind::kIntBinary);
  ASSERT_EQ(block_c->instrs().at(3)->instr_kind(), ir::InstrKind::kReturn);

  auto eq_instr = static_cast<ir::IntCompareInstr*>(block_a->instrs().at(0).get());
  auto jcc_instr = static_cast<ir::JumpCondInstr*>(block_a->instrs().at(1).get());
  auto ret_instr_a = static_cast<ir::ReturnInstr*>(block_b->instrs().at(0).get());
  auto sub_instr = static_cast<ir::IntBinaryInstr*>(block_c->instrs().at(0).get());
  auto call_instr = static_cast<ir::CallInstr*>(block_c->instrs().at(1).get());
  auto mul_instr = static_cast<ir::IntBinaryInstr*>(block_c->instrs().at(2).get());
  auto ret_instr_b = static_cast<ir::ReturnInstr*>(block_c->instrs().at(3).get());

  EXPECT_EQ(eq_instr->operation(), common::Int::CompareOp::kEq);
  EXPECT_EQ(eq_instr->operand_a().get(), arg);
  ASSERT_EQ(eq_instr->operand_b()->type(), ir::u64());
  ASSERT_EQ(eq_instr->operand_b()->kind(), ir::Value::Kind::kConstant);
  auto const_a = static_cast<ir::IntConstant*>(eq_instr->operand_b().get());
  EXPECT_EQ(const_a->int_type(), common::IntType::kU64);
  EXPECT_EQ(const_a->value().AsUint64(), 1);
  ir::Computed* value_a = eq_instr->result().get();
  EXPECT_EQ(value_a->type(), ir::bool_type());

  EXPECT_EQ(jcc_instr->condition().get(), value_a);
  EXPECT_EQ(jcc_instr->destination_true(), 1);
  EXPECT_EQ(jcc_instr->destination_false(), 2);

  EXPECT_THAT(ret_instr_a->args(), SizeIs(1));
  ASSERT_EQ(ret_instr_a->args().at(0)->type(), ir::u64());
  ASSERT_EQ(ret_instr_a->args().at(0)->kind(), ir::Value::Kind::kConstant);
  auto const_b = static_cast<ir::IntConstant*>(ret_instr_a->args().at(0).get());
  EXPECT_EQ(const_b->int_type(), common::IntType::kU64);
  EXPECT_EQ(const_b->value().AsUint64(), 1);

  EXPECT_EQ(sub_instr->operation(), common::Int::BinaryOp::kSub);
  EXPECT_EQ(sub_instr->operand_a().get(), arg);
  ASSERT_EQ(sub_instr->operand_b()->type(), ir::u64());
  ASSERT_EQ(sub_instr->operand_b()->kind(), ir::Value::Kind::kConstant);
  auto const_c = static_cast<ir::IntConstant*>(sub_instr->operand_b().get());
  EXPECT_EQ(const_c->int_type(), common::IntType::kU64);
  EXPECT_EQ(const_c->value().AsUint64(), 1);
  ir::Computed* value_b = sub_instr->result().get();
  EXPECT_EQ(value_b->type(), ir::u64());

  ASSERT_EQ(call_instr->func()->kind(), ir::Value::Kind::kConstant);
  auto const_d = static_cast<ir::FuncConstant*>(call_instr->func().get());
  EXPECT_EQ(const_d->value(), 42);
  EXPECT_THAT(call_instr->args(), SizeIs(1));
  EXPECT_EQ(call_instr->args().at(0).get(), value_b);
  EXPECT_THAT(call_instr->results(), SizeIs(1));
  ir::Computed* value_c = call_instr->results().at(0).get();
  EXPECT_EQ(value_c->type(), ir::u64());

  EXPECT_EQ(mul_instr->operation(), common::Int::BinaryOp::kMul);
  EXPECT_EQ(mul_instr->operand_a().get(), value_c);
  EXPECT_EQ(mul_instr->operand_b().get(), arg);
  ir::Computed* value_d = mul_instr->result().get();
  EXPECT_EQ(value_d->type(), ir::u64());

  EXPECT_THAT(ret_instr_b->args(), SizeIs(1));
  EXPECT_EQ(ret_instr_b->args().at(0).get(), value_d);
}

TEST(ParseTest, ParsesMultipleFuncs) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@123 () => (u16) {
{23}
  ret #47:u16
}

@456 name(%0:b, %1:ptr) => (u16) {
{49}
  jcc %0, {48}, {47}
{47}
  %2:u16 = call @123
  ret %2:u16
{48}
  %3:u16 = call @789, @-1, %1:ptr
  ret %3:u16
}

@789 x (%0:func, %1:ptr) => (u16) {
{1}
  %2:b = niltest %0:func
  jcc %2, {5}, {9}
{5}
  %3:u16 = load %1
  ret %3:u16
{9}
  %5:u16, %4:i32 = call %0, 0x1234, %1:ptr
  ret %5:u16
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(3));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func_a = program->funcs().at(0).get();
  EXPECT_EQ(func_a->number(), 123);
  EXPECT_THAT(func_a->name(), IsEmpty());
  EXPECT_EQ(func_a->computed_count(), 0);
  {
    ir::Block* block = func_a->GetBlock(23);

    auto ret_instr = static_cast<ir::ReturnInstr*>(block->instrs().at(0).get());

    ASSERT_THAT(ret_instr->args(), SizeIs(1));
    ASSERT_EQ(ret_instr->args().front()->kind(), ir::Value::Kind::kConstant);
    ASSERT_EQ(ret_instr->args().front()->type(), ir::u16());
    auto c = static_cast<ir::IntConstant*>(ret_instr->args().front().get());
    EXPECT_EQ(c->int_type(), common::IntType::kU16);
    EXPECT_EQ(c->value().AsUint64(), 47);
  }

  ir::Func* func_b = program->funcs().at(1).get();
  EXPECT_EQ(func_b->number(), 456);
  EXPECT_EQ(func_b->name(), "name");
  EXPECT_EQ(func_b->computed_count(), 4);
  {
    ir::Computed* arg_a = func_b->args().at(0).get();
    ir::Computed* arg_b = func_b->args().at(1).get();
    EXPECT_EQ(arg_a->type(), ir::bool_type());
    EXPECT_EQ(arg_b->type(), ir::pointer_type());

    ir::Block* block_a = func_b->GetBlock(49);
    ir::Block* block_b = func_b->GetBlock(47);
    ir::Block* block_c = func_b->GetBlock(48);

    ASSERT_THAT(block_a->instrs(), SizeIs(1));
    ASSERT_THAT(block_b->instrs(), SizeIs(2));
    ASSERT_THAT(block_c->instrs(), SizeIs(2));

    ASSERT_EQ(block_a->instrs().at(0)->instr_kind(), ir::InstrKind::kJumpCond);
    auto jump_cond_instr = static_cast<ir::JumpCondInstr*>(block_a->instrs().at(0).get());
    EXPECT_EQ(jump_cond_instr->condition().get(), arg_a);
    EXPECT_EQ(jump_cond_instr->destination_true(), 48);
    EXPECT_EQ(jump_cond_instr->destination_false(), 47);

    ASSERT_EQ(block_b->instrs().at(0)->instr_kind(), ir::InstrKind::kCall);
    ASSERT_EQ(block_b->instrs().at(1)->instr_kind(), ir::InstrKind::kReturn);
    auto call_instr_a = static_cast<ir::CallInstr*>(block_b->instrs().at(0).get());
    ASSERT_EQ(call_instr_a->func()->kind(), ir::Value::Kind::kConstant);
    auto const_a = static_cast<ir::FuncConstant*>(call_instr_a->func().get());
    EXPECT_EQ(const_a->value(), 123);
    EXPECT_THAT(call_instr_a->args(), IsEmpty());
    ASSERT_THAT(call_instr_a->results(), SizeIs(1));
    ir::Computed* value_a = call_instr_a->results().at(0).get();
    EXPECT_EQ(value_a->type(), ir::u16());
    auto return_instr_a = static_cast<ir::ReturnInstr*>(block_b->instrs().at(1).get());
    ASSERT_THAT(return_instr_a->args(), SizeIs(1));
    EXPECT_EQ(return_instr_a->args().at(0).get(), value_a);

    ASSERT_EQ(block_c->instrs().at(0)->instr_kind(), ir::InstrKind::kCall);
    ASSERT_EQ(block_c->instrs().at(1)->instr_kind(), ir::InstrKind::kReturn);
    auto call_instr_b = static_cast<ir::CallInstr*>(block_c->instrs().at(0).get());
    ASSERT_EQ(call_instr_b->func()->kind(), ir::Value::Kind::kConstant);
    auto const_b = static_cast<ir::FuncConstant*>(call_instr_b->func().get());
    EXPECT_EQ(const_b->value(), 789);
    ASSERT_THAT(call_instr_b->args(), SizeIs(2));
    ASSERT_EQ(call_instr_b->args().at(0)->type(), ir::func_type());
    ASSERT_EQ(call_instr_b->args().at(0)->kind(), ir::Value::Kind::kConstant);
    auto const_c = static_cast<ir::FuncConstant*>(call_instr_b->args().at(0).get());
    EXPECT_EQ(const_c->value(), -1);
    EXPECT_EQ(call_instr_b->args().at(1).get(), arg_b);
    ASSERT_THAT(call_instr_b->results(), SizeIs(1));
    ir::Computed* value_b = call_instr_b->results().at(0).get();
    EXPECT_EQ(value_b->type(), ir::u16());
    auto return_instr_b = static_cast<ir::ReturnInstr*>(block_c->instrs().at(1).get());
    ASSERT_THAT(return_instr_b->args(), SizeIs(1));
    EXPECT_EQ(return_instr_b->args().at(0).get(), value_b);
  }

  ir::Func* func_c = program->funcs().at(2).get();
  EXPECT_EQ(func_c->number(), 789);
  EXPECT_EQ(func_c->name(), "x");
  EXPECT_EQ(func_c->computed_count(), 6);
  {
    ir::Computed* arg_a = func_c->args().at(0).get();
    ir::Computed* arg_b = func_c->args().at(1).get();
    EXPECT_EQ(arg_a->type(), ir::func_type());
    EXPECT_EQ(arg_b->type(), ir::pointer_type());

    ir::Block* block_a = func_c->GetBlock(1);
    ir::Block* block_b = func_c->GetBlock(5);
    ir::Block* block_c = func_c->GetBlock(9);

    ASSERT_THAT(block_a->instrs(), SizeIs(2));
    ASSERT_THAT(block_b->instrs(), SizeIs(2));
    ASSERT_THAT(block_c->instrs(), SizeIs(2));

    ASSERT_EQ(block_a->instrs().at(0)->instr_kind(), ir::InstrKind::kNilTest);
    auto niltest_instr = static_cast<ir::NilTestInstr*>(block_a->instrs().at(0).get());
    EXPECT_EQ(niltest_instr->tested().get(), arg_a);
    ir::Computed* value_a = niltest_instr->result().get();
    EXPECT_EQ(value_a->type(), ir::bool_type());
    ASSERT_EQ(block_a->instrs().at(1)->instr_kind(), ir::InstrKind::kJumpCond);
    auto jump_cond_instr = static_cast<ir::JumpCondInstr*>(block_a->instrs().at(1).get());
    EXPECT_EQ(jump_cond_instr->condition().get(), value_a);
    EXPECT_EQ(jump_cond_instr->destination_true(), 5);
    EXPECT_EQ(jump_cond_instr->destination_false(), 9);

    ASSERT_EQ(block_b->instrs().at(0)->instr_kind(), ir::InstrKind::kLoad);
    auto load_instr = static_cast<ir::LoadInstr*>(block_b->instrs().at(0).get());
    EXPECT_EQ(load_instr->address().get(), arg_b);
    ir::Computed* value_b = load_instr->result().get();
    EXPECT_EQ(value_b->type(), ir::u16());
    ASSERT_EQ(block_b->instrs().at(1)->instr_kind(), ir::InstrKind::kReturn);
    auto return_instr_a = static_cast<ir::ReturnInstr*>(block_b->instrs().at(1).get());
    ASSERT_THAT(return_instr_a->args(), SizeIs(1));
    EXPECT_EQ(return_instr_a->args().at(0).get(), value_b);

    ASSERT_EQ(block_c->instrs().at(0)->instr_kind(), ir::InstrKind::kCall);
    auto call_instr = static_cast<ir::CallInstr*>(block_c->instrs().at(0).get());
    EXPECT_EQ(call_instr->func().get(), arg_a);
    ASSERT_THAT(call_instr->args(), SizeIs(2));
    ASSERT_EQ(call_instr->args().at(0)->type(), ir::pointer_type());
    ASSERT_EQ(call_instr->args().at(0)->kind(), ir::Value::Kind::kConstant);
    auto const_a = static_cast<ir::PointerConstant*>(call_instr->args().at(0).get());
    EXPECT_EQ(const_a->value(), 0x1234);
    EXPECT_EQ(call_instr->args().at(1).get(), arg_b);
    ASSERT_THAT(call_instr->results(), SizeIs(2));
    ir::Computed* value_c = call_instr->results().at(0).get();
    EXPECT_EQ(value_c->type(), ir::u16());
    EXPECT_EQ(value_c->number(), 5);
    ir::Computed* value_d = call_instr->results().at(1).get();
    EXPECT_EQ(value_d->type(), ir::i32());
    EXPECT_EQ(value_d->number(), 4);
    ASSERT_EQ(block_c->instrs().at(1)->instr_kind(), ir::InstrKind::kReturn);
    auto return_instr_b = static_cast<ir::ReturnInstr*>(block_c->instrs().at(1).get());
    ASSERT_THAT(return_instr_b->args(), SizeIs(1));
    EXPECT_EQ(return_instr_b->args().at(0).get(), value_c);
  }
}

TEST(ParseTest, ParsesSyscall) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 (%0:i64, %1:i64) => (i64) {
{0}
  %2:i64 = syscall #42:i64, %1, #123, %0
  ret %2
}
)ir");

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(1));
  EXPECT_THAT(program->entry_func(), IsNull());
  EXPECT_EQ(program->entry_func_num(), ir::kNoFuncNum);

  ir::Func* func = program->funcs().front().get();
  ASSERT_THAT(func->args(), SizeIs(2));
  ASSERT_THAT(func->result_types(), SizeIs(1));
  ASSERT_EQ(func->computed_count(), 3);
  ASSERT_THAT(func->blocks(), SizeIs(1));
  ASSERT_THAT(func->blocks().front(), Not(IsNull()));
  ASSERT_TRUE(func->HasBlock(0));

  ir::Computed* arg_a = func->args().at(0).get();
  ir::Computed* arg_b = func->args().at(1).get();

  ir::Block* block = func->GetBlock(0);
  ASSERT_THAT(block->instrs(), SizeIs(2));
  ASSERT_THAT(block->instrs().front(), Not(IsNull()));
  ASSERT_THAT(block->instrs().back(), Not(IsNull()));

  ir::Instr* instr_a = block->instrs().at(0).get();
  ir::Instr* instr_b = block->instrs().at(1).get();
  ASSERT_EQ(instr_a->instr_kind(), ir::InstrKind::kSyscall);
  ASSERT_EQ(instr_b->instr_kind(), ir::InstrKind::kReturn);

  auto syscall_instr = static_cast<ir::SyscallInstr*>(instr_a);
  ASSERT_EQ(syscall_instr->syscall_num()->kind(), ir::Value::Kind::kConstant);
  ASSERT_EQ(syscall_instr->syscall_num()->type(), ir::i64());
  auto c1 = static_cast<ir::IntConstant*>(syscall_instr->syscall_num().get());
  EXPECT_EQ(c1->int_type(), common::IntType::kI64);
  EXPECT_EQ(c1->value().AsInt64(), 42);

  ASSERT_THAT(syscall_instr->args(), SizeIs(3));
  EXPECT_EQ(syscall_instr->args().at(0).get(), arg_b);
  EXPECT_EQ(syscall_instr->args().at(2).get(), arg_a);
  ASSERT_EQ(syscall_instr->args().at(1)->kind(), ir::Value::Kind::kConstant);
  ASSERT_EQ(syscall_instr->args().at(1)->type(), ir::i64());
  auto c2 = static_cast<ir::IntConstant*>(syscall_instr->args().at(1).get());
  EXPECT_EQ(c2->int_type(), common::IntType::kI64);
  EXPECT_EQ(c2->value().AsInt64(), 123);

  auto return_instr = static_cast<ir::ReturnInstr*>(instr_b);
  EXPECT_THAT(return_instr->args(), SizeIs(1));
  EXPECT_THAT(return_instr->args().front().get(), syscall_instr->result().get());
}

TEST(ParseTest, ParsesAdditionalFuncs) {
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(R"ir(
@0 (%0:i64, %1:i64, %2:func) => (ptr, i64) {
{0}
  %3:i64, %4:ptr = call %2, #42:i64, %1, #123:u16, %0
  ret %4, %3
}

@1 (%0:i64, %1:i64) => (i64) {
{0}
  %2:i64 = syscall #42:i64, %1, #123, %0
  ret %2
}
)ir");
  ir::Func* func_a = program->GetFunc(0);
  ir::Func* func_b = program->GetFunc(1);
  std::vector<ir::Func*> additional_funcs =
      ir_serialization::ParseAdditionalFuncsForProgram(program.get(), R"ir(
@0 toast(%2:func) => (b) {
{0}
  %0:i64 = call %2, #987:i64, #-1:i64
  %1:b = igtr %0, #0:i64
  ret %1
}

@42 main() => (i64) {
{0}
  %0:b = call @0, @-1
  ret #0:i64
}

)ir");

  ASSERT_THAT(additional_funcs, SizeIs(2));
  ir::Func* func_c = additional_funcs.at(0);
  ir::Func* func_d = additional_funcs.at(1);

  ir_checker::AssertProgramIsOkay(program.get());

  ASSERT_THAT(program->funcs(), SizeIs(4));
  EXPECT_EQ(program->entry_func(), func_d);
  EXPECT_EQ(program->entry_func_num(), 44);

  EXPECT_EQ(func_c->number(), 2);
  EXPECT_EQ(program->GetFunc(2), func_c);
  EXPECT_EQ(func_d->number(), 44);
  EXPECT_EQ(program->GetFunc(44), func_d);

  EXPECT_EQ(func_a->number(), 0);
  EXPECT_THAT(func_a->name(), IsEmpty());
  EXPECT_THAT(func_a->args(), SizeIs(3));
  ASSERT_THAT(func_a->result_types(), SizeIs(2));
  EXPECT_EQ(func_a->computed_count(), 5);

  EXPECT_EQ(func_b->number(), 1);
  EXPECT_THAT(func_b->name(), IsEmpty());
  EXPECT_THAT(func_b->args(), SizeIs(2));
  ASSERT_THAT(func_b->result_types(), SizeIs(1));
  EXPECT_EQ(func_b->computed_count(), 3);

  EXPECT_EQ(func_c->name(), "toast");
  EXPECT_THAT(func_c->args(), SizeIs(1));
  ASSERT_THAT(func_c->result_types(), SizeIs(1));
  EXPECT_EQ(func_c->computed_count(), 3);

  EXPECT_EQ(func_d->name(), "main");
  EXPECT_THAT(func_d->args(), IsEmpty());
  ASSERT_THAT(func_d->result_types(), SizeIs(1));
  EXPECT_EQ(func_d->computed_count(), 1);

  ir::Block* block = func_d->blocks().at(0).get();

  ASSERT_EQ(block->instrs().at(0)->instr_kind(), ir::InstrKind::kCall);
  auto call_instr = static_cast<ir::CallInstr*>(block->instrs().at(0).get());
  EXPECT_TRUE(ir::IsEqual(call_instr->func().get(), ir::ToFuncConstant(2).get()));
  ASSERT_THAT(call_instr->args(), SizeIs(1));
  ASSERT_EQ(call_instr->args().at(0)->type(), ir::func_type());
  ASSERT_EQ(call_instr->args().at(0)->kind(), ir::Value::Kind::kConstant);
  EXPECT_TRUE(ir::IsEqual(call_instr->args().at(0).get(), ir::NilFunc().get()));
}

}  // namespace
