//
//  func_values_builder_test.cc
//  Katara
//
//  Created by Arne Philipeit on 11/12/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "src/ir/analyzers/func_values_builder.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/info/func_values.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/parse.h"

namespace ir_analyzers {
namespace {

using ::ir_info::FuncValues;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

TEST(FindValuesInFuncTest, HandlesEmptyFunc) {
  std::unique_ptr<ir::Program> input_program = ir_serialization::ParseProgramOrDie(R"ir(
@0 f() => () {
{0}
  ret
}
)ir");
  const FuncValues func_values = FindValuesInFunc(input_program->GetFunc(0));

  EXPECT_THAT(func_values.GetValues(), IsEmpty());

  EXPECT_THAT(func_values.GetValuesWithType(ir::bool_type()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::u8()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::i64()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::pointer_type()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::func_type()), IsEmpty());

  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kBool), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kInt), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kPointer), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kFunc), IsEmpty());

  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(0)), nullptr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(1)), nullptr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(123)), nullptr);

  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(0)), IsEmpty());
  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(1)), IsEmpty());
  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(123)), IsEmpty());
}

TEST(FindValuesInFuncTest, HandlesSingleComputation) {
  std::unique_ptr<ir::Program> input_program = ir_serialization::ParseProgramOrDie(R"ir(
@0 f() => (i64) {
{0}
  %42:i64 = ineg #1234
  ret %42
}
)ir");
  ir::Func* func = input_program->GetFunc(ir::func_num_t(0));
  ir::Block* block = func->entry_block();
  ir::Instr* ineg_instr = block->instrs().at(0).get();
  ir::Instr* ret_instr = block->instrs().at(1).get();

  const FuncValues func_values = FindValuesInFunc(func);

  EXPECT_THAT(func_values.GetValues(), UnorderedElementsAre(ir::value_num_t(42)));

  EXPECT_THAT(func_values.GetValuesWithType(ir::bool_type()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::u8()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::i64()), UnorderedElementsAre(ir::value_num_t(42)));
  EXPECT_THAT(func_values.GetValuesWithType(ir::pointer_type()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::func_type()), IsEmpty());

  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kBool), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kInt),
              UnorderedElementsAre(ir::value_num_t(42)));
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kPointer), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kFunc), IsEmpty());

  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(0)), nullptr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(1)), nullptr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(42)), ineg_instr);

  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(0)), IsEmpty());
  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(1)), IsEmpty());
  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(42)),
              UnorderedElementsAre(ret_instr));
}

TEST(FindValuesInFuncTest, HandlesFuncArgument) {
  std::unique_ptr<ir::Program> input_program = ir_serialization::ParseProgramOrDie(R"ir(
@0 f(%41:i64) => (i64) {
{0}
  %42:i64 = ineg %41
  ret %42
}
)ir");
  ir::Func* func = input_program->GetFunc(ir::func_num_t(0));
  ir::Block* block = func->entry_block();
  ir::Instr* ineg_instr = block->instrs().at(0).get();
  ir::Instr* ret_instr = block->instrs().at(1).get();

  const FuncValues func_values = FindValuesInFunc(func);

  EXPECT_THAT(func_values.GetValues(),
              UnorderedElementsAre(ir::value_num_t(41), ir::value_num_t(42)));

  EXPECT_THAT(func_values.GetValuesWithType(ir::bool_type()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::u8()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::i64()),
              UnorderedElementsAre(ir::value_num_t(41), ir::value_num_t(42)));
  EXPECT_THAT(func_values.GetValuesWithType(ir::pointer_type()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::func_type()), IsEmpty());

  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kBool), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kInt),
              UnorderedElementsAre(ir::value_num_t(41), ir::value_num_t(42)));
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kPointer), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kFunc), IsEmpty());

  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(41)), nullptr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(42)), ineg_instr);

  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(41)),
              UnorderedElementsAre(ineg_instr));
  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(42)),
              UnorderedElementsAre(ret_instr));
}

TEST(FindValuesInFuncTest, HandlesMultipleBlocks) {
  std::unique_ptr<ir::Program> input_program = ir_serialization::ParseProgramOrDie(R"ir(
@0 f(%40:b, %41:i64) => (i64, b) {
{0}
  jcc %40, {1}, {2}
{1}
  %42:i64 = ineg %41
  jmp {3}
{2}
  %43:i64 = inot %41
  jmp {3}
{3}
  %44:i64 = phi %42:{1}, %43:{2}
  ret %44, %40
}
)ir");

  ir::Func* func = input_program->GetFunc(ir::func_num_t(0));
  ir::Block* block0 = func->GetBlock(ir::block_num_t(0));
  ir::Instr* jcc_instr = block0->instrs().at(0).get();

  ir::Block* block1 = func->GetBlock(ir::block_num_t(1));
  ir::Instr* ineg_instr = block1->instrs().at(0).get();

  ir::Block* block2 = func->GetBlock(ir::block_num_t(2));
  ir::Instr* inot_instr = block2->instrs().at(0).get();

  ir::Block* block3 = func->GetBlock(ir::block_num_t(3));
  ir::Instr* phi_instr = block3->instrs().at(0).get();
  ir::Instr* ret_instr = block3->instrs().at(1).get();

  const FuncValues func_values = FindValuesInFunc(func);

  EXPECT_THAT(func_values.GetValues(),
              UnorderedElementsAre(ir::value_num_t(40), ir::value_num_t(41), ir::value_num_t(42),
                                   ir::value_num_t(43), ir::value_num_t(44)));

  EXPECT_THAT(func_values.GetValuesWithType(ir::bool_type()),
              UnorderedElementsAre(ir::value_num_t(40)));
  EXPECT_THAT(func_values.GetValuesWithType(ir::u8()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::i64()),
              UnorderedElementsAre(ir::value_num_t(41), ir::value_num_t(42), ir::value_num_t(43),
                                   ir::value_num_t(44)));
  EXPECT_THAT(func_values.GetValuesWithType(ir::pointer_type()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::func_type()), IsEmpty());

  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kBool),
              UnorderedElementsAre(ir::value_num_t(40)));
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kInt),
              UnorderedElementsAre(ir::value_num_t(41), ir::value_num_t(42), ir::value_num_t(43),
                                   ir::value_num_t(44)));
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kPointer), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kFunc), IsEmpty());

  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(40)), nullptr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(41)), nullptr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(42)), ineg_instr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(43)), inot_instr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(ir::value_num_t(44)), phi_instr);

  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(40)),
              UnorderedElementsAre(jcc_instr, ret_instr));
  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(41)),
              UnorderedElementsAre(ineg_instr, inot_instr));
  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(42)),
              UnorderedElementsAre(phi_instr));
  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(43)),
              UnorderedElementsAre(phi_instr));
  EXPECT_THAT(func_values.GetInstrsUsingValue(ir::value_num_t(44)),
              UnorderedElementsAre(ret_instr));
}

}  // namespace
}  // namespace ir_analyzers
