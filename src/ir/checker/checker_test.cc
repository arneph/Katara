//
//  checker_test.cc
//  Katara
//
//  Created by Arne Philipeit on 3/11/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/checker/checker.h"

#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/common/atomics/atomics.h"
#include "src/ir/checker/issues.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace {

using ::ir_checker::CheckProgram;
using ::ir_checker::Issue;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Property;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

TEST(CheckerTest, CatchesValueHasNullptrTypeForArg) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(/*type=*/nullptr, /*vnum=*/0);
  func->args().push_back(arg);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(Property("kind", &Issue::kind, Issue::Kind::kValueHasNullptrType),
                        Property("scope_object", &Issue::scope_object, arg.get()),
                        Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesValueHasNullptrTypeForValue) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  func->args().push_back(arg);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto value = std::make_shared<ir::Computed>(/*type=*/nullptr, /*vnum=*/1);
  block->instrs().push_back(std::make_unique<ir::LoadInstr>(value, arg));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(Property("kind", &Issue::kind, Issue::Kind::kValueHasNullptrType),
                        Property("scope_object", &Issue::scope_object, value.get()),
                        Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesInstrDefinesNullptrValue) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  func->args().push_back(arg);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::LoadInstr>(/*result=*/nullptr, arg));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kInstrDefinesNullptrValue),
                  Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesInstrUsesNullptrValue) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  func->result_types().push_back(ir::i8());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{nullptr}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kInstrUsesNullptrValue),
                  Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesInstrUsesNullptrValueForInheritedValue) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  func->result_types().push_back(ir::i8());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_a, block_b->number(), block_c->number()));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  auto inherited_a = std::make_shared<ir::InheritedValue>(arg_b, block_a->number());
  auto inherited_b = std::make_shared<ir::InheritedValue>(nullptr, block_b->number());
  auto arg_c = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/2);
  block_c->instrs().push_back(std::make_unique<ir::PhiInstr>(
      arg_c, std::vector<std::shared_ptr<ir::InheritedValue>>{inherited_a, inherited_b}));
  block_c->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{arg_c}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kInstrUsesNullptrValue),
                  Property("scope_object", &Issue::scope_object, block_c->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesNonPhiInstrUsesInheritedValue) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/0);
  func->args().push_back(arg);
  func->result_types().push_back(ir::i8());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto value = std::make_shared<ir::InheritedValue>(arg, block->number());
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{value}));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kNonPhiInstrUsesInheritedValue),
                Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                Property("involved_objects", &Issue::involved_objects, ElementsAre(value.get())))));
}

TEST(CheckerTest, CatchesMovInstrOriginAndResultHaveMismatchedTypes) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/0);
  func->args().push_back(arg);
  func->result_types().push_back(ir::i16());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto value = std::make_shared<ir::Computed>(ir::i16(), /*vnum=*/1);
  block->instrs().push_back(std::make_unique<ir::MovInstr>(value, arg));
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{value}));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kMovInstrOriginAndResultHaveMismatchedTypes),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects,
                   ElementsAre(arg.get(), value.get())))));
}

TEST(CheckerTest, CatchesPhiInstrOriginAndResultHaveMismatchedTypesForConstantValue) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  func->result_types().push_back(ir::i8());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_a, block_b->number(), block_c->number()));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  auto inherited_a = std::make_shared<ir::InheritedValue>(arg_b, block_a->number());
  auto inherited_b = std::make_shared<ir::InheritedValue>(ir::I16Zero(), block_b->number());
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/2);
  block_c->instrs().push_back(std::make_unique<ir::PhiInstr>(
      result, std::vector<std::shared_ptr<ir::InheritedValue>>{inherited_a, inherited_b}));
  block_c->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kPhiInstrArgAndResultHaveMismatchedTypes),
          Property("scope_object", &Issue::scope_object, block_c->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects,
                   ElementsAre(inherited_b.get(), result.get())))));
}

TEST(CheckerTest, CatchesPhiInstrOriginAndResultHaveMismatchedTypesForComputedValue) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  func->result_types().push_back(ir::i16());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_a, block_b->number(), block_c->number()));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  auto inherited_a = std::make_shared<ir::InheritedValue>(arg_b, block_a->number());
  auto inherited_b = std::make_shared<ir::InheritedValue>(ir::I16Zero(), block_b->number());
  auto result = std::make_shared<ir::Computed>(ir::i16(), /*vnum=*/2);
  block_c->instrs().push_back(std::make_unique<ir::PhiInstr>(
      result, std::vector<std::shared_ptr<ir::InheritedValue>>{inherited_a, inherited_b}));
  block_c->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kPhiInstrArgAndResultHaveMismatchedTypes),
          Property("scope_object", &Issue::scope_object, block_c->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects,
                   ElementsAre(inherited_a.get(), result.get())))));
}

TEST(CheckerTest, CatchesPhiInstrHasNoArgumentForParentBlock) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  func->result_types().push_back(ir::i8());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_a, block_b->number(), block_c->number()));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  auto inherited_a = std::make_shared<ir::InheritedValue>(arg_b, block_a->number());
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/2);
  block_c->instrs().push_back(std::make_unique<ir::PhiInstr>(
      result, std::vector<std::shared_ptr<ir::InheritedValue>>{inherited_a}));
  block_c->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kPhiInstrHasNoArgumentForParentBlock),
                  Property("scope_object", &Issue::scope_object, block_c->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects, ElementsAre(block_b)))));
}

TEST(CheckerTest, CatchesPhiInstrHasMultipleArgumentsForParentBlock) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  func->result_types().push_back(ir::i8());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_a, block_b->number(), block_c->number()));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  auto inherited_a = std::make_shared<ir::InheritedValue>(arg_b, block_a->number());
  auto inherited_b = std::make_shared<ir::InheritedValue>(ir::I8Zero(), block_b->number());
  auto inherited_c = std::make_shared<ir::InheritedValue>(arg_b, block_b->number());
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/2);
  block_c->instrs().push_back(std::make_unique<ir::PhiInstr>(
      result,
      std::vector<std::shared_ptr<ir::InheritedValue>>{inherited_a, inherited_b, inherited_c}));
  block_c->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kPhiInstrHasMultipleArgumentsForParentBlock),
          Property("scope_object", &Issue::scope_object, block_c->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects,
                   ElementsAre(inherited_b.get(), inherited_c.get())))));
}

TEST(CheckerTest, CatchesPhiInstrHasArgumentForNonParentBlock) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  func->result_types().push_back(ir::i8());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_a, block_b->number(), block_c->number()));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  auto inherited_a = std::make_shared<ir::InheritedValue>(arg_b, block_a->number());
  auto inherited_b = std::make_shared<ir::InheritedValue>(ir::I8Zero(), block_b->number());
  auto inherited_c = std::make_shared<ir::InheritedValue>(arg_b, /*origin=*/42);
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/2);
  block_c->instrs().push_back(std::make_unique<ir::PhiInstr>(
      result,
      std::vector<std::shared_ptr<ir::InheritedValue>>{inherited_a, inherited_b, inherited_c}));
  block_c->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kPhiInstrHasArgumentForNonParentBlock),
          Property("scope_object", &Issue::scope_object, block_c->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(inherited_c.get())))));
}

ir::Instr* PrepareSimpleComputationTest(ir::Program& program,
                                        std::unique_ptr<ir::Computation> instr) {
  ir::Func* func = program.AddFunc();
  for (auto& arg : instr->UsedValues()) {
    if (arg->kind() != ir::Value::Kind::kComputed) {
      continue;
    }
    func->args().push_back(std::static_pointer_cast<ir::Computed>(arg));
  }
  std::shared_ptr<ir::Computed> result = instr->result();
  func->result_types().push_back(result->type());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::move(instr));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>(
      /*args=*/std::vector<std::shared_ptr<ir::Value>>{result}));
  return block->instrs().front().get();
}

TEST(CheckerTest, CatchesBoolNotInstrOperandDoesNotHaveBoolType) {
  ir::Program program;
  auto arg = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  ir::Instr* instr =
      PrepareSimpleComputationTest(program, std::make_unique<ir::BoolNotInstr>(result, arg));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kBoolNotInstrOperandDoesNotHaveBoolType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(arg.get())))));
}

TEST(CheckerTest, CatchesBoolNotInstrResultDoesNotHaveBoolType) {
  ir::Program program;
  auto arg = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/1);
  ir::Instr* instr =
      PrepareSimpleComputationTest(program, std::make_unique<ir::BoolNotInstr>(result, arg));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kBoolNotInstrResultDoesNotHaveBoolType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
}

TEST(CheckerTest, CatchesBoolBinaryInstrOperandDoesNotHaveBoolType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::BoolBinaryInstr>(result, common::Bool::BinaryOp::kAnd, arg_a, arg_b));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kBoolBinaryInstrOperandDoesNotHaveBoolType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(arg_a.get())))));
}

TEST(CheckerTest, CatchesBoolBinaryInstrResultDoesNotHaveBoolType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::BoolBinaryInstr>(result, common::Bool::BinaryOp::kAnd, arg_a, arg_b));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kBoolBinaryInstrResultDoesNotHaveBoolType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
}

TEST(CheckerTest, CatchesIntUnaryInstrOperandDoesNotHaveIntType) {
  ir::Program program;
  auto arg = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::i32(), /*vnum=*/1);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntUnaryInstr>(result, common::Int::UnaryOp::kNeg, arg));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(
              Property("kind", &Issue::kind, Issue::Kind::kIntUnaryInstrOperandDoesNotHaveIntType),
              Property("scope_object", &Issue::scope_object, instr),
              Property("involved_objects", &Issue::involved_objects, ElementsAre(arg.get()))),
          AllOf(Property("kind", &Issue::kind,
                         Issue::Kind::kIntUnaryInstrResultAndOperandHaveDifferentTypes),
                Property("scope_object", &Issue::scope_object, instr),
                Property("involved_objects", &Issue::involved_objects,
                         ElementsAre(result.get(), arg.get())))));
}

TEST(CheckerTest, CatchesIntUnaryInstrResultDoesNotHaveIntType) {
  ir::Program program;
  auto arg = std::make_shared<ir::Computed>(ir::i16(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/1);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntUnaryInstr>(result, common::Int::UnaryOp::kNeg, arg));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kIntUnaryInstrResultDoesNotHaveIntType),
                Property("scope_object", &Issue::scope_object, instr),
                Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get()))),
          AllOf(Property("kind", &Issue::kind,
                         Issue::Kind::kIntUnaryInstrResultAndOperandHaveDifferentTypes),
                Property("scope_object", &Issue::scope_object, instr),
                Property("involved_objects", &Issue::involved_objects,
                         ElementsAre(result.get(), arg.get())))));
}

TEST(CheckerTest, CatchesIntCompareInstrOperandDoesNotHaveIntType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::IntCompareInstr>(result, common::Int::CompareOp::kLeq, arg_a, arg_b));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kIntCompareInstrOperandDoesNotHaveIntType),
                                Property("scope_object", &Issue::scope_object, instr),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(arg_b.get()))),
                          AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kIntCompareInstrOperandsHaveDifferentTypes),
                                Property("scope_object", &Issue::scope_object, instr),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(arg_a.get(), arg_b.get())))));
}

TEST(CheckerTest, CatchesIntCompareInstrOperandsHaveDifferentTypes) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::IntCompareInstr>(result, common::Int::CompareOp::kLeq, arg_a, arg_b));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kIntCompareInstrOperandsHaveDifferentTypes),
                                Property("scope_object", &Issue::scope_object, instr),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(arg_a.get(), arg_b.get())))));
}

TEST(CheckerTest, CatchesIntCompareInstrResultDoesNotHaveBoolType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::IntCompareInstr>(result, common::Int::CompareOp::kLeq, arg_a, arg_b));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kIntCompareInstrResultDoesNotHaveBoolType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
}

TEST(CheckerTest, CatchesIntBinaryInstrOperandDoesNotHaveIntType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::IntBinaryInstr>(result, common::Int::BinaryOp::kXor, arg_a, arg_b));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(
              Property("kind", &Issue::kind, Issue::Kind::kIntBinaryInstrOperandDoesNotHaveIntType),
              Property("scope_object", &Issue::scope_object, instr),
              Property("involved_objects", &Issue::involved_objects, ElementsAre(arg_a.get()))),
          AllOf(Property("kind", &Issue::kind,
                         Issue::Kind::kIntBinaryInstrOperandsAndResultHaveDifferentTypes),
                Property("scope_object", &Issue::scope_object, instr),
                Property("involved_objects", &Issue::involved_objects,
                         ElementsAre(result.get(), arg_a.get(), arg_b.get())))));
}

TEST(CheckerTest, CatchesIntBinaryInstrResultDoesNotHaveIntType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::IntBinaryInstr>(result, common::Int::BinaryOp::kXor, arg_a, arg_b));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(
              Property("kind", &Issue::kind, Issue::Kind::kIntBinaryInstrResultDoesNotHaveIntType),
              Property("scope_object", &Issue::scope_object, instr),
              Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get()))),
          AllOf(Property("kind", &Issue::kind,
                         Issue::Kind::kIntBinaryInstrOperandsAndResultHaveDifferentTypes),
                Property("scope_object", &Issue::scope_object, instr),
                Property("involved_objects", &Issue::involved_objects,
                         ElementsAre(result.get(), arg_a.get(), arg_b.get())))));
}

TEST(CheckerTest, CatchesIntBinaryInstrOperandsAndResultHaveDifferentTypes) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::IntBinaryInstr>(result, common::Int::BinaryOp::kXor, arg_a, arg_b));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 Issue::Kind::kIntBinaryInstrOperandsAndResultHaveDifferentTypes),
                        Property("scope_object", &Issue::scope_object, instr),
                        Property("involved_objects", &Issue::involved_objects,
                                 ElementsAre(result.get(), arg_a.get(), arg_b.get())))));
}

TEST(CheckerTest, CatchesIntShiftInstrOperandDoesNotHaveIntType) {
  ir::Program program;
  auto shifted = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::IntShiftInstr>(result, common::Int::ShiftOp::kLeft, shifted, offset));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kIntShiftInstrOperandDoesNotHaveIntType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(offset.get())))));
}

TEST(CheckerTest, CatchesIntShiftInstrResultDoesNotHaveIntType) {
  ir::Program program;
  auto shifted = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::IntShiftInstr>(result, common::Int::ShiftOp::kLeft, shifted, offset));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kIntShiftInstrResultDoesNotHaveIntType),
                Property("scope_object", &Issue::scope_object, instr),
                Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get()))),
          AllOf(Property("kind", &Issue::kind,
                         Issue::Kind::kIntShiftInstrShiftedAndResultHaveDifferentTypes),
                Property("scope_object", &Issue::scope_object, instr),
                Property("involved_objects", &Issue::involved_objects,
                         ElementsAre(result.get(), shifted.get())))));
}

TEST(CheckerTest, CatchesIntShiftInstrShiftedAndResultHaveDifferentTypes) {
  ir::Program program;
  auto shifted = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program,
      std::make_unique<ir::IntShiftInstr>(result, common::Int::ShiftOp::kLeft, shifted, offset));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 Issue::Kind::kIntShiftInstrShiftedAndResultHaveDifferentTypes),
                        Property("scope_object", &Issue::scope_object, instr),
                        Property("involved_objects", &Issue::involved_objects,
                                 ElementsAre(result.get(), shifted.get())))));
}

TEST(CheckerTest, CatchesPointerOffsetInstrPointerDoesNotHavePointerType) {
  ir::Program program;
  auto pointer = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program, std::make_unique<ir::PointerOffsetInstr>(result, pointer, offset));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kPointerOffsetInstrPointerDoesNotHavePointerType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(pointer.get())))));
}

TEST(CheckerTest, CatchesPointerOffsetInstrOffsetDoesNotHaveI64Type) {
  ir::Program program;
  auto pointer = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program, std::make_unique<ir::PointerOffsetInstr>(result, pointer, offset));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kPointerOffsetInstrOffsetDoesNotHaveI64Type),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(offset.get())))));
}

TEST(CheckerTest, CatchesPointerOffsetInstrResultDoesNotHavePointerType) {
  ir::Program program;
  auto pointer = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  ir::Instr* instr = PrepareSimpleComputationTest(
      program, std::make_unique<ir::PointerOffsetInstr>(result, pointer, offset));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kPointerOffsetInstrResultDoesNotHavePointerType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
}

TEST(CheckerTest, CatchesNilTestInstrTestedDoesNotHavePointerOrFuncType) {
  ir::Program program;
  auto tested = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  ir::Instr* instr =
      PrepareSimpleComputationTest(program, std::make_unique<ir::NilTestInstr>(result, tested));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kNilTestInstrTestedDoesNotHavePointerOrFuncType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(tested.get())))));
}

TEST(CheckerTest, CatchesNilTestInstrResultDoesNotHaveBoolType) {
  ir::Program program;
  auto tested = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  ir::Instr* instr =
      PrepareSimpleComputationTest(program, std::make_unique<ir::NilTestInstr>(result, tested));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kNilTestInstrResultDoesNotHaveBoolType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
}

TEST(CheckerTest, CatchesMallocInstrSizeDoesNotHaveI64Type) {
  ir::Program program;
  auto size = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  ir::Instr* instr =
      PrepareSimpleComputationTest(program, std::make_unique<ir::MallocInstr>(result, size));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kMallocInstrSizeDoesNotHaveI64Type),
                Property("scope_object", &Issue::scope_object, instr),
                Property("involved_objects", &Issue::involved_objects, ElementsAre(size.get())))));
}

TEST(CheckerTest, CatchesMallocInstrResultDoesNotHavePointerType) {
  ir::Program program;
  auto size = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  ir::Instr* instr =
      PrepareSimpleComputationTest(program, std::make_unique<ir::MallocInstr>(result, size));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kMallocInstrResultDoesNotHavePointerType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
}

TEST(CheckerTest, CatchesLoadInstrAddressDoesNotHavePointerType) {
  ir::Program program;
  auto address = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/1);
  ir::Instr* instr =
      PrepareSimpleComputationTest(program, std::make_unique<ir::LoadInstr>(result, address));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kLoadInstrAddressDoesNotHavePointerType),
          Property("scope_object", &Issue::scope_object, instr),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(address.get())))));
}

TEST(CheckerTest, CatchesStoreInstrAddressDoesNotHavePointerType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto address = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto value = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  func->args().push_back(address);
  func->args().push_back(value);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::StoreInstr>(address, value));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kStoreInstrAddressDoesNotHavePointerType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(address.get())))));
}

TEST(CheckerTest, CatchesFreeInstrAddressDoesNotHavePointerType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto address = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  func->args().push_back(address);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::FreeInstr>(address));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kFreeInstrAddressDoesNotHavePointerType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(address.get())))));
}

TEST(CheckerTest, CatchesJumpInstrDestinationIsNotChildBlock) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  block_a->instrs().push_back(std::make_unique<ir::JumpInstr>(/*destination=*/123));
  block_b->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kJumpInstrDestinationIsNotChildBlock),
                  Property("scope_object", &Issue::scope_object, block_a),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(block_a->instrs().front().get())))));
}

TEST(CheckerTest, CatchesJumpCondInstrConditionDoesNotHaveBoolType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto cond = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  func->args().push_back(cond);
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(cond, block_b->number(), block_c->number()));
  block_b->instrs().push_back(std::make_unique<ir::ReturnInstr>());
  block_c->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kJumpCondInstrConditionDoesNotHaveBoolType),
          Property("scope_object", &Issue::scope_object, block_a->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(cond.get())))));
}

TEST(CheckerTest, CatchesJumpCondInstrHasDuplicateDestinations) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto cond = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  func->args().push_back(cond);
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(cond, block_b->number(), block_b->number()));
  block_b->instrs().push_back(std::make_unique<ir::ReturnInstr>());
  block_c->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kJumpCondInstrHasDuplicateDestinations),
                Property("scope_object", &Issue::scope_object, block_a->instrs().front().get()),
                Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesJumpCondInstrDestinationIsNotChildBlock) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto cond = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  func->args().push_back(cond);
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(cond, block_b->number(), /*destination_false=*/123));
  block_b->instrs().push_back(std::make_unique<ir::ReturnInstr>());
  block_c->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kJumpCondInstrDestinationIsNotChildBlock),
                                Property("scope_object", &Issue::scope_object, block_a),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(block_a->instrs().front().get())))));
}

TEST(CheckerTest, CatchesSyscallInstrResultDoesNotHaveI64Type) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto result = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::SyscallInstr>(
      result, /*syscall_num=*/ir::I64Zero(), /*args=*/std::vector<std::shared_ptr<ir::Value>>{}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kSyscallInstrResultDoesNotHaveI64Type),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
}

TEST(CheckerTest, CatchesSyscallInstrSyscallNumDoesNotHaveI64Type) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto syscall_num = ir::U64Zero();
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::SyscallInstr>(
      result, syscall_num, /*args=*/std::vector<std::shared_ptr<ir::Value>>{}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kSyscallInstrSyscallNumberDoesNotHaveI64Type),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(syscall_num.get())))));
}

TEST(CheckerTest, CatchesSyscallInstrArgDoesNotHaveI64Type) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto syscall_num = ir::I64Zero();
  auto arg_a = ir::I64Zero();
  auto arg_b = ir::U64Zero();
  auto arg_c = ir::I64Zero();
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::SyscallInstr>(
      result, syscall_num, /*args=*/std::vector<std::shared_ptr<ir::Value>>{arg_a, arg_b, arg_c}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kSyscallInstrArgDoesNotHaveI64Type),
                Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                Property("involved_objects", &Issue::involved_objects, ElementsAre(arg_b.get())))));
}

TEST(CheckerTest, CatchesCallInstrCalleeDoesNotHaveFuncTypeForConstant) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto callee = ir::I64Zero();
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::CallInstr>(
      callee, /*results=*/std::vector<std::shared_ptr<ir::Computed>>{},
      /*args=*/std::vector<std::shared_ptr<ir::Value>>{}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kCallInstrCalleeDoesNotHaveFuncType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(callee.get())))));
}

TEST(CheckerTest, CatchesCallInstrCalleeDoesNotHaveFuncTypeForComputed) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto callee = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  func->args().push_back(callee);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::CallInstr>(
      callee, /*results=*/std::vector<std::shared_ptr<ir::Computed>>{},
      /*args=*/std::vector<std::shared_ptr<ir::Value>>{}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kCallInstrCalleeDoesNotHaveFuncType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(callee.get())))));
}

TEST(CheckerTest, CatchesCallInstrStaticCalleeDoesNotExist) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto callee = ir::ToFuncConstant(123);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::CallInstr>(
      callee, /*results=*/std::vector<std::shared_ptr<ir::Computed>>{},
      /*args=*/std::vector<std::shared_ptr<ir::Value>>{}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kCallInstrStaticCalleeDoesNotExist),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(callee.get())))));
}

ir::Func* PrepareCalleeFuncForCallInstrTest(ir::Program& program) {
  ir::Func* callee = program.AddFunc();
  auto callee_arg_a = std::make_shared<ir::Computed>(ir::i32(), /*vnum=*/0);
  auto callee_arg_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  callee->args().push_back(callee_arg_a);
  callee->args().push_back(callee_arg_b);
  callee->result_types().push_back(ir::func_type());
  callee->result_types().push_back(ir::pointer_type());
  callee->result_types().push_back(ir::i16());
  ir::Block* callee_block = callee->AddBlock();
  callee->set_entry_block_num(callee_block->number());
  callee_block->instrs().push_back(std::make_unique<ir::ReturnInstr>(
      std::vector<std::shared_ptr<ir::Value>>{ir::NilFunc(), callee_arg_b, ir::I16Zero()}));
  return callee;
}

TEST(CheckerTest, CatchesCallInstrDoesNotMatchStaticCalleeSignatureForMissingArg) {
  ir::Program program;
  ir::Func* callee = PrepareCalleeFuncForCallInstrTest(program);
  ir::Func* caller = program.AddFunc();
  ir::Block* caller_block = caller->AddBlock();
  caller->set_entry_block_num(caller_block->number());
  auto result_a = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/0);
  auto result_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  auto result_c = std::make_shared<ir::Computed>(ir::i16(), /*vnum=*/2);
  caller_block->instrs().push_back(std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(callee->number()),
      std::vector<std::shared_ptr<ir::Computed>>{result_a, result_b, result_c},
      std::vector<std::shared_ptr<ir::Value>>{ir::I32Zero()}));
  caller_block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature),
          Property("scope_object", &Issue::scope_object, caller_block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(callee)))));
}

TEST(CheckerTest, CatchesCallInstrDoesNotMatchStaticCalleeSignatureForExcessArg) {
  ir::Program program;
  ir::Func* callee = PrepareCalleeFuncForCallInstrTest(program);
  ir::Func* caller = program.AddFunc();
  ir::Block* caller_block = caller->AddBlock();
  caller->set_entry_block_num(caller_block->number());
  auto result_a = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/0);
  auto result_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  auto result_c = std::make_shared<ir::Computed>(ir::i16(), /*vnum=*/2);
  caller_block->instrs().push_back(std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(callee->number()),
      std::vector<std::shared_ptr<ir::Computed>>{result_a, result_b, result_c},
      std::vector<std::shared_ptr<ir::Value>>{ir::I32Zero(), ir::NilPointer(), ir::U8Zero()}));
  caller_block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature),
          Property("scope_object", &Issue::scope_object, caller_block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(callee)))));
}

TEST(CheckerTest, CatchesCallInstrDoesNotMatchStaticCalleeSignatureForMismatchedArg) {
  ir::Program program;
  ir::Func* callee = PrepareCalleeFuncForCallInstrTest(program);
  ir::Func* caller = program.AddFunc();
  ir::Block* caller_block = caller->AddBlock();
  caller->set_entry_block_num(caller_block->number());
  auto result_a = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/0);
  auto result_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  auto result_c = std::make_shared<ir::Computed>(ir::i16(), /*vnum=*/2);
  auto mismatched_param = callee->args().front();
  auto mismatched_arg = ir::U32Zero();
  caller_block->instrs().push_back(std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(callee->number()),
      std::vector<std::shared_ptr<ir::Computed>>{result_a, result_b, result_c},
      std::vector<std::shared_ptr<ir::Value>>{mismatched_arg, ir::NilPointer()}));
  caller_block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature),
          Property("scope_object", &Issue::scope_object, caller_block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects,
                   ElementsAre(callee, mismatched_arg.get(), mismatched_param.get())))));
}

TEST(CheckerTest, CatchesCallInstrDoesNotMatchStaticCalleeSignatureForMissingResult) {
  ir::Program program;
  ir::Func* callee = PrepareCalleeFuncForCallInstrTest(program);
  ir::Func* caller = program.AddFunc();
  ir::Block* caller_block = caller->AddBlock();
  caller->set_entry_block_num(caller_block->number());
  auto result_a = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/0);
  auto result_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  caller_block->instrs().push_back(std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(callee->number()),
      std::vector<std::shared_ptr<ir::Computed>>{result_a, result_b},
      std::vector<std::shared_ptr<ir::Value>>{ir::I32Zero(), ir::NilPointer()}));
  caller_block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature),
          Property("scope_object", &Issue::scope_object, caller_block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(callee)))));
}

TEST(CheckerTest, CatchesCallInstrDoesNotMatchStaticCalleeSignatureForExcessResult) {
  ir::Program program;
  ir::Func* callee = PrepareCalleeFuncForCallInstrTest(program);
  ir::Func* caller = program.AddFunc();
  ir::Block* caller_block = caller->AddBlock();
  caller->set_entry_block_num(caller_block->number());
  auto result_a = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/0);
  auto result_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  auto result_c = std::make_shared<ir::Computed>(ir::i16(), /*vnum=*/2);
  auto result_d = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/3);
  caller_block->instrs().push_back(std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(callee->number()),
      std::vector<std::shared_ptr<ir::Computed>>{result_a, result_b, result_c, result_d},
      std::vector<std::shared_ptr<ir::Value>>{ir::I32Zero(), ir::NilPointer()}));
  caller_block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature),
          Property("scope_object", &Issue::scope_object, caller_block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(callee)))));
}

TEST(CheckerTest, CatchesCallInstrDoesNotMatchStaticCalleeSignatureForMismatchedResult) {
  ir::Program program;
  ir::Func* callee = PrepareCalleeFuncForCallInstrTest(program);
  ir::Func* caller = program.AddFunc();
  ir::Block* caller_block = caller->AddBlock();
  caller->set_entry_block_num(caller_block->number());
  auto result_a = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/0);
  auto result_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result_c = std::make_shared<ir::Computed>(ir::i16(), /*vnum=*/2);
  caller_block->instrs().push_back(std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(callee->number()),
      std::vector<std::shared_ptr<ir::Computed>>{result_a, result_b, result_c},
      std::vector<std::shared_ptr<ir::Value>>{ir::I32Zero(), ir::NilPointer()}));
  caller_block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature),
          Property("scope_object", &Issue::scope_object, caller_block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects,
                   ElementsAre(callee, result_b.get(), callee->result_types().at(1))))));
}

TEST(CheckerTest, CatchesReturnInstrDoesNotMatchFuncSignatureForMissingResult) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  func->args().push_back(arg);
  func->result_types().push_back(ir::pointer_type());
  func->result_types().push_back(ir::bool_type());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{ir::NilPointer()}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kReturnInstrDoesNotMatchFuncSignature),
                                Property("scope_object", &Issue::scope_object, func),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(block->instrs().front().get())))));
}

TEST(CheckerTest, CatchesReturnInstrDoesNotMatchFuncSignatureForExcessResult) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  func->args().push_back(arg);
  func->result_types().push_back(ir::pointer_type());
  func->result_types().push_back(ir::bool_type());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>(
      std::vector<std::shared_ptr<ir::Value>>{ir::NilPointer(), arg, ir::True()}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kReturnInstrDoesNotMatchFuncSignature),
                                Property("scope_object", &Issue::scope_object, func),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(block->instrs().front().get())))));
}

TEST(CheckerTest, CatchesReturnInstrDoesNotMatchFuncSignatureForMismatchedResult) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  func->args().push_back(arg);
  auto mismatched_result_type = ir::pointer_type();
  auto mismatched_result = ir::NilFunc();
  func->result_types().push_back(mismatched_result_type);
  func->result_types().push_back(ir::bool_type());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>(
      std::vector<std::shared_ptr<ir::Value>>{mismatched_result, arg}));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kReturnInstrDoesNotMatchFuncSignature),
                Property("scope_object", &Issue::scope_object, func),
                Property("involved_objects", &Issue::involved_objects,
                         ElementsAre(block->instrs().front().get(), mismatched_result.get(),
                                     mismatched_result_type)))));
}

TEST(CheckerTest, CatchesEntryBlockHasParents) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  func->AddControlFlow(block->number(), block->number());
  block->instrs().push_back(std::make_unique<ir::JumpInstr>(block->number()));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kEntryBlockHasParents),
                  Property("scope_object", &Issue::scope_object, func),
                  Property("involved_objects", &Issue::involved_objects, ElementsAre(block)))));
}

TEST(CheckerTest, CatchesNonEntryBlockHasNoParents) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  block_a->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  block_c->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kNonEntryBlockHasNoParents),
                  Property("scope_object", &Issue::scope_object, func),
                  Property("involved_objects", &Issue::involved_objects, ElementsAre(block_b)))));
}

TEST(CheckerTest, CatchesBlockContainsNoInstrs) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(Property("kind", &Issue::kind, Issue::Kind::kBlockContainsNoInstrs),
                        Property("scope_object", &Issue::scope_object, block),
                        Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesPhiInBlockWithoutMultipleParentsInEntryBlock) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto phi_result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  block->instrs().push_back(std::make_unique<ir::PhiInstr>(
      phi_result, std::vector<std::shared_ptr<ir::InheritedValue>>{}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kPhiInBlockWithoutMultipleParents),
                  Property("scope_object", &Issue::scope_object, block),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(block->instrs().front().get())))));
}

TEST(CheckerTest, CatchesPhiInBlockWithoutMultipleParentsInBlockWithSingleParent) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  block_a->instrs().push_back(std::make_unique<ir::JumpInstr>(block_b->number()));
  auto phi_result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  block_b->instrs().push_back(std::make_unique<ir::PhiInstr>(
      phi_result, std::vector<std::shared_ptr<ir::InheritedValue>>{
                      std::make_shared<ir::InheritedValue>(ir::I64One(), block_a->number())}));
  block_b->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kPhiInBlockWithoutMultipleParents),
                  Property("scope_object", &Issue::scope_object, block_b),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(block_b->instrs().front().get())))));
}

TEST(CheckerTest, CatchesPhiAfterRegularInstrInBlock) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_a, block_b->number(), block_c->number()));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  block_c->instrs().push_back(std::make_unique<ir::FreeInstr>(arg_b));
  auto phi_result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  block_c->instrs().push_back(std::make_unique<ir::PhiInstr>(
      phi_result, std::vector<std::shared_ptr<ir::InheritedValue>>{
                      std::make_shared<ir::InheritedValue>(ir::I64One(), block_a->number()),
                      std::make_shared<ir::InheritedValue>(ir::I64Eight(), block_b->number())}));
  block_c->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kPhiAfterRegularInstrInBlock),
          Property("scope_object", &Issue::scope_object, block_c),
          Property("involved_objects", &Issue::involved_objects,
                   ElementsAre(block_c->instrs().at(0).get(), block_c->instrs().at(1).get())))));
}

TEST(CheckerTest, CatchesControlFlowInstrBeforeEndOfBlockForJumpInstr) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  func->args().push_back(arg);
  func->result_types().push_back(ir::i64());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  block_a->instrs().push_back(
      std::make_unique<ir::IntUnaryInstr>(result, common::Int::UnaryOp::kNot, arg));
  block_a->instrs().push_back(std::make_unique<ir::JumpInstr>(block_b->number()));
  block_a->instrs().push_back(std::make_unique<ir::JumpInstr>(block_b->number()));
  block_b->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kControlFlowInstrBeforeEndOfBlock),
                  Property("scope_object", &Issue::scope_object, block_a),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(block_a->instrs().at(1).get())))));
}

TEST(CheckerTest, CatchesControlFlowInstrBeforeEndOfBlockForJumpCondInstr) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  func->result_types().push_back(ir::i64());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_b, block_b->number(), block_c->number()));
  block_a->instrs().push_back(
      std::make_unique<ir::IntUnaryInstr>(result, common::Int::UnaryOp::kNot, arg_a));
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_b, block_b->number(), block_c->number()));
  block_b->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));
  block_c->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kControlFlowInstrBeforeEndOfBlock),
                  Property("scope_object", &Issue::scope_object, block_a),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(block_a->instrs().at(0).get())))));
}

TEST(CheckerTest, CatchesControlFlowInstrBeforeEndOfBlockForReturnInstr) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  func->args().push_back(arg);
  func->result_types().push_back(ir::i64());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  block->instrs().push_back(
      std::make_unique<ir::IntUnaryInstr>(result, common::Int::UnaryOp::kNot, arg));
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kControlFlowInstrBeforeEndOfBlock),
                  Property("scope_object", &Issue::scope_object, block),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(block->instrs().at(1).get())))));
}

TEST(CheckerTest, CatchesControlFlowInstrMissingAtEndOfBlock) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  func->args().push_back(arg);
  func->result_types().push_back(ir::i64());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  block->instrs().push_back(
      std::make_unique<ir::IntUnaryInstr>(result, common::Int::UnaryOp::kNot, arg));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kControlFlowInstrMissingAtEndOfBlock),
                  Property("scope_object", &Issue::scope_object, block),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(block->instrs().back().get())))));
}

TEST(CheckerTest, CatchesControlFlowInstrMismatchedWithBlockGraphForMissingControlFlowOfJumpInstr) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_b, block_b->number(), block_c->number()));
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  block_b->instrs().push_back(
      std::make_unique<ir::IntUnaryInstr>(result, common::Int::UnaryOp::kNot, arg_a));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  block_c->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kControlFlowInstrMismatchedWithBlockGraph),
                                Property("scope_object", &Issue::scope_object, block_b),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(block_b->instrs().back().get())))));
}

TEST(CheckerTest,
     CatchesControlFlowInstrMismatchedWithBlockGraphForMissingControlFlowOfJumpCondInstr) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  auto arg_c = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/2);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  func->args().push_back(arg_c);
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  ir::Block* block_d = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_d->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_b, block_b->number(), block_c->number()));
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/3);
  block_b->instrs().push_back(
      std::make_unique<ir::IntUnaryInstr>(result, common::Int::UnaryOp::kNot, arg_a));
  block_b->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_c, block_c->number(), block_d->number()));
  block_c->instrs().push_back(std::make_unique<ir::ReturnInstr>());
  block_d->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kControlFlowInstrMismatchedWithBlockGraph),
                                Property("scope_object", &Issue::scope_object, block_b),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(block_b->instrs().back().get())))));
}

TEST(CheckerTest, CatchesControlFlowInstrMismatchedWithBlockGraphForExcessControlFlow) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  func->args().push_back(arg);
  func->result_types().push_back(ir::i64());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  block_a->instrs().push_back(
      std::make_unique<ir::IntUnaryInstr>(result, common::Int::UnaryOp::kNot, arg));
  block_a->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));
  block_b->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kControlFlowInstrMismatchedWithBlockGraph),
                                Property("scope_object", &Issue::scope_object, block_a),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(block_a->instrs().back().get())))));
}

TEST(CheckerTest, CatchesFuncDefinesNullptrArg) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto arg_c = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  func->args().push_back(arg_a);
  func->args().push_back(nullptr);
  func->args().push_back(arg_c);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(Property("kind", &Issue::kind, Issue::Kind::kFuncDefinesNullptrArg),
                        Property("scope_object", &Issue::scope_object, func),
                        Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesFuncHasNullptrResultType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  func->result_types().push_back(ir::bool_type());
  func->result_types().push_back(nullptr);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto mismatched_result = ir::I16Zero();
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>(
      std::vector<std::shared_ptr<ir::Value>>{ir::False(), mismatched_result}));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kReturnInstrDoesNotMatchFuncSignature),
                Property("scope_object", &Issue::scope_object, func),
                Property(
                    "involved_objects", &Issue::involved_objects,
                    ElementsAre(block->instrs().front().get(), mismatched_result.get(), nullptr))),
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kFuncHasNullptrResultType),
                Property("scope_object", &Issue::scope_object, func),
                Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesFuncHasNoEntryBlock) {
  ir::Program program;
  ir::Func* func = program.AddFunc();

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(Property("kind", &Issue::kind, Issue::Kind::kFuncHasNoEntryBlock),
                        Property("scope_object", &Issue::scope_object, func),
                        Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
}

TEST(CheckerTest, CatchesComputedValueUsedInMultipleFunctionsForSharedArg) {
  ir::Program program;
  auto arg = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/0);

  ir::Func* func_a = program.AddFunc();
  func_a->args().push_back(arg);
  ir::Block* block_a = func_a->AddBlock();
  func_a->set_entry_block_num(block_a->number());
  block_a->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  ir::Func* func_b = program.AddFunc();
  func_b->args().push_back(arg);
  ir::Block* block_b = func_b->AddBlock();
  func_b->set_entry_block_num(block_b->number());
  block_b->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kComputedValueUsedInMultipleFunctions),
                                Property("scope_object", &Issue::scope_object, &program),
                                Property("involved_objects", &Issue::involved_objects,
                                         UnorderedElementsAre(arg.get(), func_a, func_b)))));
}

TEST(CheckerTest, CatchesComputedValueUsedInMultipleFunctionsForSharedComputationResult) {
  ir::Program program;
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);

  ir::Func* func_a = program.AddFunc();
  ir::Block* block_a = func_a->AddBlock();
  func_a->set_entry_block_num(block_a->number());
  block_a->instrs().push_back(std::make_unique<ir::MallocInstr>(result, ir::I64Eight()));
  block_a->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  ir::Func* func_b = program.AddFunc();
  ir::Block* block_b = func_b->AddBlock();
  func_b->set_entry_block_num(block_b->number());
  block_b->instrs().push_back(std::make_unique<ir::MallocInstr>(result, ir::I64Eight()));
  block_b->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kComputedValueUsedInMultipleFunctions),
                                Property("scope_object", &Issue::scope_object, &program),
                                Property("involved_objects", &Issue::involved_objects,
                                         UnorderedElementsAre(result.get(), func_a, func_b)))));
}

TEST(CheckerTest, CatchesComputedValueUsedInMultipleFunctionsForArgAndComputationResult) {
  ir::Program program;
  auto value = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);

  ir::Func* func_a = program.AddFunc();
  ir::Block* block_a = func_a->AddBlock();
  func_a->set_entry_block_num(block_a->number());
  block_a->instrs().push_back(std::make_unique<ir::MallocInstr>(value, ir::I64Eight()));
  block_a->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  ir::Func* func_b = program.AddFunc();
  func_b->args().push_back(value);
  ir::Block* block_b = func_b->AddBlock();
  func_b->set_entry_block_num(block_b->number());
  block_b->instrs().push_back(std::make_unique<ir::FreeInstr>(value));
  block_b->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kComputedValueUsedInMultipleFunctions),
                                Property("scope_object", &Issue::scope_object, &program),
                                Property("involved_objects", &Issue::involved_objects,
                                         UnorderedElementsAre(value.get(), func_a, func_b)))));
}

TEST(CheckerTest, CatchesComputedValueNumberUsedMultipleTimesForArgs) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  func->args().push_back(arg_a);
  func->args().push_back(arg_b);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kComputedValueNumberUsedMultipleTimes),
                Property("scope_object", &Issue::scope_object, func),
                Property("involved_objects", &Issue::involved_objects,
                         UnorderedElementsAre(arg_a.get(), arg_b.get()))),
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kComputedValueHasMultipleDefinitions),
                Property("scope_object", &Issue::scope_object, func),
                Property("involved_objects", &Issue::involved_objects,
                         Contains(AnyOf(arg_a.get(), arg_b.get()))))));
}

TEST(CheckerTest, CatchesComputedValueNumberUsedMultipleTimesForComputations) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto result_a = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  auto result_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  block->instrs().push_back(std::make_unique<ir::MallocInstr>(result_a, ir::I64Eight()));
  block->instrs().push_back(std::make_unique<ir::MallocInstr>(result_b, ir::I64Eight()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kComputedValueNumberUsedMultipleTimes),
                Property("scope_object", &Issue::scope_object, func),
                Property("involved_objects", &Issue::involved_objects,
                         UnorderedElementsAre(result_a.get(), result_b.get()))),
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kComputedValueHasMultipleDefinitions),
                Property("scope_object", &Issue::scope_object, func),
                Property("involved_objects", &Issue::involved_objects,
                         Contains(AnyOf(result_a.get(), result_b.get()))))));
}

TEST(CheckerTest, CatchesComputedValueNumberUsedMultipleTimesForArgAndComputation) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  func->args().push_back(arg);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  block->instrs().push_back(std::make_unique<ir::MallocInstr>(result, ir::I64Eight()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kComputedValueNumberUsedMultipleTimes),
                Property("scope_object", &Issue::scope_object, func),
                Property("involved_objects", &Issue::involved_objects,
                         UnorderedElementsAre(arg.get(), result.get()))),
          AllOf(Property("kind", &Issue::kind, Issue::Kind::kComputedValueHasMultipleDefinitions),
                Property("scope_object", &Issue::scope_object, func),
                Property("involved_objects", &Issue::involved_objects,
                         Contains(AnyOf(arg.get(), result.get()))))));
}

TEST(CheckerTest, CatchesComputedValueHasNoDefinition) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  func->result_types().push_back(ir::u16());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  auto result = std::make_shared<ir::Computed>(ir::u16(), /*vnum=*/0);
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kComputedValueHasNoDefinition),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
}

TEST(CheckerTest, CatchesComputedValueHasMultipleDefinitions) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto value = std::make_shared<ir::Computed>(ir::u16(), /*vnum=*/0);
  func->args().push_back(value);
  func->result_types().push_back(ir::u16());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<ir::IntUnaryInstr>(value, common::Int::UnaryOp::kNeg, value));
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{value}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind, Issue::Kind::kComputedValueHasMultipleDefinitions),
                  Property("scope_object", &Issue::scope_object, func),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(value.get(), block->instrs().front().get())))));
}

TEST(CheckerTest, CatchesComputedValueDefinitionDoesNotDominateUse) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  func->args().push_back(arg);
  func->result_types().push_back(ir::pointer_type());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_a->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg, block_b->number(), block_c->number()));
  auto value = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  block_b->instrs().push_back(std::make_unique<ir::MallocInstr>(value, ir::I64Eight()));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  block_c->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{value}));

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         Issue::Kind::kComputedValueDefinitionDoesNotDominateUse),
                                Property("scope_object", &Issue::scope_object, func),
                                Property("involved_objects", &Issue::involved_objects,
                                         ElementsAre(block_b->instrs().front().get(),
                                                     block_c->instrs().front().get())))));
}

TEST(CheckerTest, FindsNoComputedValueDefinitionDoesNotDominateUseForCorrectInheritedValues) {
  // Constructs a loop that sums numbers from 1 to 10. This ensures that the loop header block B can
  // inherit the values computed in the loop body block C, which does not dominate B, and requires
  // that the checker correctly handles phi instrs and inherited values.
  ir::Program program;
  ir::Func* func = program.AddFunc();
  func->result_types().push_back(ir::i64());
  ir::Block* block_a = func->AddBlock();
  ir::Block* block_b = func->AddBlock();
  ir::Block* block_c = func->AddBlock();
  ir::Block* block_d = func->AddBlock();

  func->set_entry_block_num(block_a->number());
  func->AddControlFlow(block_a->number(), block_b->number());
  func->AddControlFlow(block_b->number(), block_c->number());
  func->AddControlFlow(block_b->number(), block_d->number());
  func->AddControlFlow(block_c->number(), block_b->number());

  auto value_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto value_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto value_c = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/2);
  auto value_d = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/3);
  auto value_e = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/4);

  block_a->instrs().push_back(std::make_unique<ir::JumpInstr>(block_b->number()));

  auto inherited_a = std::make_shared<ir::InheritedValue>(ir::I64One(), block_a->number());
  auto inherited_b = std::make_shared<ir::InheritedValue>(value_e, block_c->number());
  block_b->instrs().push_back(std::make_unique<ir::PhiInstr>(
      value_a, std::vector<std::shared_ptr<ir::InheritedValue>>{inherited_a, inherited_b}));
  auto inherited_c = std::make_shared<ir::InheritedValue>(ir::I64Zero(), block_a->number());
  auto inherited_d = std::make_shared<ir::InheritedValue>(value_d, block_c->number());
  block_b->instrs().push_back(std::make_unique<ir::PhiInstr>(
      value_b, std::vector<std::shared_ptr<ir::InheritedValue>>{inherited_c, inherited_d}));
  block_b->instrs().push_back(std::make_unique<ir::IntCompareInstr>(
      value_c, common::Int::CompareOp::kLeq, value_a, ir::ToIntConstant(common::Int(int64_t{10}))));
  block_b->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(value_c, block_c->number(), block_d->number()));

  block_c->instrs().push_back(
      std::make_unique<ir::IntBinaryInstr>(value_d, common::Int::BinaryOp::kAdd, value_b, value_a));
  block_c->instrs().push_back(std::make_unique<ir::IntBinaryInstr>(
      value_e, common::Int::BinaryOp::kAdd, value_a, ir::I64One()));
  block_c->instrs().push_back(std::make_unique<ir::JumpInstr>(block_b->number()));

  block_d->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{value_b}));

  EXPECT_THAT(CheckProgram(&program), IsEmpty());
  ir_checker::AssertProgramIsOkay(&program);
}

}  // namespace
