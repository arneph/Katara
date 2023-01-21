//
//  checker_test.cc
//  Katara
//
//  Created by Arne Philipeit on 3/11/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/check/check.h"

#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/common/atomics/atomics.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace {

using ::common::atomics::Bool;
using ::common::atomics::Int;
using ::common::positions::FileSet;
using ::ir_check::CheckProgram;
using ::ir_issues::Issue;
using ::ir_issues::IssueKind;
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kValueHasNullptrType)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kValueHasNullptrType)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kInstrDefinesNullptrValue)));
}

TEST(CheckerTest, CatchesInstrUsesNullptrValue) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  func->result_types().push_back(ir::i8());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{nullptr}));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kInstrUsesNullptrValue)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kInstrUsesNullptrValue)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kNonPhiInstrUsesInheritedValue)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kMovInstrOriginAndResultHaveMismatchedTypes)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kPhiInstrArgAndResultHaveMismatchedTypes)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kPhiInstrArgAndResultHaveMismatchedTypes)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kPhiInstrHasNoArgumentForParentBlock)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kPhiInstrHasMultipleArgumentsForParentBlock)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kPhiInstrHasArgumentForNonParentBlock)));
}

void PrepareSimpleComputationTest(ir::Program& program, std::unique_ptr<ir::Computation> instr) {
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
}

TEST(CheckerTest, CatchesBoolNotInstrOperandDoesNotHaveBoolType) {
  ir::Program program;
  auto arg = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  PrepareSimpleComputationTest(program, std::make_unique<ir::BoolNotInstr>(result, arg));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kBoolNotInstrOperandDoesNotHaveBoolType)));
}

TEST(CheckerTest, CatchesBoolNotInstrResultDoesNotHaveBoolType) {
  ir::Program program;
  auto arg = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/1);
  PrepareSimpleComputationTest(program, std::make_unique<ir::BoolNotInstr>(result, arg));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kBoolNotInstrResultDoesNotHaveBoolType)));
}

TEST(CheckerTest, CatchesBoolBinaryInstrOperandDoesNotHaveBoolType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::BoolBinaryInstr>(result, Bool::BinaryOp::kAnd, arg_a, arg_b));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kBoolBinaryInstrOperandDoesNotHaveBoolType)));
}

TEST(CheckerTest, CatchesBoolBinaryInstrResultDoesNotHaveBoolType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::BoolBinaryInstr>(result, Bool::BinaryOp::kAnd, arg_a, arg_b));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kBoolBinaryInstrResultDoesNotHaveBoolType)));
}

TEST(CheckerTest, CatchesIntUnaryInstrOperandDoesNotHaveIntType) {
  ir::Program program;
  auto arg = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::i32(), /*vnum=*/1);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntUnaryInstr>(result, Int::UnaryOp::kNeg, arg));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(

          Property("kind", &Issue::kind, IssueKind::kIntUnaryInstrOperandDoesNotHaveIntType),
          Property("kind", &Issue::kind,
                   IssueKind::kIntUnaryInstrResultAndOperandHaveDifferentTypes)));
}

TEST(CheckerTest, CatchesIntUnaryInstrResultDoesNotHaveIntType) {
  ir::Program program;
  auto arg = std::make_shared<ir::Computed>(ir::i16(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/1);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntUnaryInstr>(result, Int::UnaryOp::kNeg, arg));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kIntUnaryInstrResultDoesNotHaveIntType),
                  Property("kind", &Issue::kind,
                           IssueKind::kIntUnaryInstrResultAndOperandHaveDifferentTypes)));
}

TEST(CheckerTest, CatchesIntCompareInstrOperandDoesNotHaveIntType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntCompareInstr>(result, Int::CompareOp::kLeq, arg_a, arg_b));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(
          Property("kind", &Issue::kind, IssueKind::kIntCompareInstrOperandDoesNotHaveIntType),
          Property("kind", &Issue::kind, IssueKind::kIntCompareInstrOperandsHaveDifferentTypes)));
}

TEST(CheckerTest, CatchesIntCompareInstrOperandsHaveDifferentTypes) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntCompareInstr>(result, Int::CompareOp::kLeq, arg_a, arg_b));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kIntCompareInstrOperandsHaveDifferentTypes)));
}

TEST(CheckerTest, CatchesIntCompareInstrResultDoesNotHaveBoolType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntCompareInstr>(result, Int::CompareOp::kLeq, arg_a, arg_b));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kIntCompareInstrResultDoesNotHaveBoolType)));
}

TEST(CheckerTest, CatchesIntBinaryInstrOperandDoesNotHaveIntType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntBinaryInstr>(result, Int::BinaryOp::kXor, arg_a, arg_b));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kIntBinaryInstrOperandDoesNotHaveIntType),
                          Property("kind", &Issue::kind,
                                   IssueKind::kIntBinaryInstrOperandsAndResultHaveDifferentTypes)));
}

TEST(CheckerTest, CatchesIntBinaryInstrResultDoesNotHaveIntType) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntBinaryInstr>(result, Int::BinaryOp::kXor, arg_a, arg_b));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(

          Property("kind", &Issue::kind, IssueKind::kIntBinaryInstrResultDoesNotHaveIntType),
          Property("kind", &Issue::kind,
                   IssueKind::kIntBinaryInstrOperandsAndResultHaveDifferentTypes)));
}

TEST(CheckerTest, CatchesIntBinaryInstrOperandsAndResultHaveDifferentTypes) {
  ir::Program program;
  auto arg_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto arg_b = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntBinaryInstr>(result, Int::BinaryOp::kXor, arg_a, arg_b));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kIntBinaryInstrOperandsAndResultHaveDifferentTypes)));
}

TEST(CheckerTest, CatchesIntShiftInstrOperandDoesNotHaveIntType) {
  ir::Program program;
  auto shifted = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntShiftInstr>(result, Int::ShiftOp::kLeft, shifted, offset));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kIntShiftInstrOperandDoesNotHaveIntType)));
}

TEST(CheckerTest, CatchesIntShiftInstrResultDoesNotHaveIntType) {
  ir::Program program;
  auto shifted = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntShiftInstr>(result, Int::ShiftOp::kLeft, shifted, offset));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kIntShiftInstrResultDoesNotHaveIntType),
                  Property("kind", &Issue::kind,
                           IssueKind::kIntShiftInstrShiftedAndResultHaveDifferentTypes)));
}

TEST(CheckerTest, CatchesIntShiftInstrShiftedAndResultHaveDifferentTypes) {
  ir::Program program;
  auto shifted = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  PrepareSimpleComputationTest(
      program, std::make_unique<ir::IntShiftInstr>(result, Int::ShiftOp::kLeft, shifted, offset));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kIntShiftInstrShiftedAndResultHaveDifferentTypes)));
}

TEST(CheckerTest, CatchesPointerOffsetInstrPointerDoesNotHavePointerType) {
  ir::Program program;
  auto pointer = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/2);
  PrepareSimpleComputationTest(program,
                               std::make_unique<ir::PointerOffsetInstr>(result, pointer, offset));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kPointerOffsetInstrPointerDoesNotHavePointerType)));
}

TEST(CheckerTest, CatchesPointerOffsetInstrOffsetDoesNotHaveI64Type) {
  ir::Program program;
  auto pointer = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/2);
  PrepareSimpleComputationTest(program,
                               std::make_unique<ir::PointerOffsetInstr>(result, pointer, offset));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kPointerOffsetInstrOffsetDoesNotHaveI64Type)));
}

TEST(CheckerTest, CatchesPointerOffsetInstrResultDoesNotHavePointerType) {
  ir::Program program;
  auto pointer = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  auto offset = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/2);
  PrepareSimpleComputationTest(program,
                               std::make_unique<ir::PointerOffsetInstr>(result, pointer, offset));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kPointerOffsetInstrResultDoesNotHavePointerType)));
}

TEST(CheckerTest, CatchesNilTestInstrTestedDoesNotHavePointerOrFuncType) {
  ir::Program program;
  auto tested = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/1);
  PrepareSimpleComputationTest(program, std::make_unique<ir::NilTestInstr>(result, tested));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kNilTestInstrTestedDoesNotHavePointerOrFuncType)));
}

TEST(CheckerTest, CatchesNilTestInstrResultDoesNotHaveBoolType) {
  ir::Program program;
  auto tested = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  PrepareSimpleComputationTest(program, std::make_unique<ir::NilTestInstr>(result, tested));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kNilTestInstrResultDoesNotHaveBoolType)));
}

TEST(CheckerTest, CatchesMallocInstrSizeDoesNotHaveI64Type) {
  ir::Program program;
  auto size = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  PrepareSimpleComputationTest(program, std::make_unique<ir::MallocInstr>(result, size));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kMallocInstrSizeDoesNotHaveI64Type)));
}

TEST(CheckerTest, CatchesMallocInstrResultDoesNotHavePointerType) {
  ir::Program program;
  auto size = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  PrepareSimpleComputationTest(program, std::make_unique<ir::MallocInstr>(result, size));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kMallocInstrResultDoesNotHavePointerType)));
}

TEST(CheckerTest, CatchesLoadInstrAddressDoesNotHavePointerType) {
  ir::Program program;
  auto address = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto result = std::make_shared<ir::Computed>(ir::func_type(), /*vnum=*/1);
  PrepareSimpleComputationTest(program, std::make_unique<ir::LoadInstr>(result, address));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kLoadInstrAddressDoesNotHavePointerType)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kStoreInstrAddressDoesNotHavePointerType)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kFreeInstrAddressDoesNotHavePointerType)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kJumpInstrDestinationIsNotChildBlock)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kJumpCondInstrConditionDoesNotHaveBoolType)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kJumpCondInstrHasDuplicateDestinations)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kJumpCondInstrDestinationIsNotChildBlock)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kSyscallInstrResultDoesNotHaveI64Type)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kSyscallInstrSyscallNumberDoesNotHaveI64Type)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kSyscallInstrArgDoesNotHaveI64Type)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kCallInstrCalleeDoesNotHaveFuncType)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kCallInstrCalleeDoesNotHaveFuncType)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kCallInstrStaticCalleeDoesNotExist)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kReturnInstrDoesNotMatchFuncSignature)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kReturnInstrDoesNotMatchFuncSignature)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kReturnInstrDoesNotMatchFuncSignature)));
}

TEST(CheckerTest, CatchesEntryBlockHasParents) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  func->AddControlFlow(block->number(), block->number());
  block->instrs().push_back(std::make_unique<ir::JumpInstr>(block->number()));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kEntryBlockHasParents)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kNonEntryBlockHasNoParents)));
}

TEST(CheckerTest, CatchesBlockContainsNoInstrs) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kBlockContainsNoInstrs)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kPhiInBlockWithoutMultipleParents)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kPhiInBlockWithoutMultipleParents)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kPhiAfterRegularInstrInBlock)));
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
  block_a->instrs().push_back(std::make_unique<ir::IntUnaryInstr>(result, Int::UnaryOp::kNot, arg));
  block_a->instrs().push_back(std::make_unique<ir::JumpInstr>(block_b->number()));
  block_a->instrs().push_back(std::make_unique<ir::JumpInstr>(block_b->number()));
  block_b->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kControlFlowInstrBeforeEndOfBlock)));
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
      std::make_unique<ir::IntUnaryInstr>(result, Int::UnaryOp::kNot, arg_a));
  block_a->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_b, block_b->number(), block_c->number()));
  block_b->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));
  block_c->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kControlFlowInstrBeforeEndOfBlock)));
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
  block->instrs().push_back(std::make_unique<ir::IntUnaryInstr>(result, Int::UnaryOp::kNot, arg));
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kControlFlowInstrBeforeEndOfBlock)));
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
  block->instrs().push_back(std::make_unique<ir::IntUnaryInstr>(result, Int::UnaryOp::kNot, arg));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kControlFlowInstrMissingAtEndOfBlock)));
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
      std::make_unique<ir::IntUnaryInstr>(result, Int::UnaryOp::kNot, arg_a));
  block_b->instrs().push_back(std::make_unique<ir::JumpInstr>(block_c->number()));
  block_c->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kControlFlowInstrMismatchedWithBlockGraph)));
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
      std::make_unique<ir::IntUnaryInstr>(result, Int::UnaryOp::kNot, arg_a));
  block_b->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(arg_c, block_c->number(), block_d->number()));
  block_c->instrs().push_back(std::make_unique<ir::ReturnInstr>());
  block_d->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kControlFlowInstrMismatchedWithBlockGraph)));
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
  block_a->instrs().push_back(std::make_unique<ir::IntUnaryInstr>(result, Int::UnaryOp::kNot, arg));
  block_a->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));
  block_b->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{result}));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kControlFlowInstrMismatchedWithBlockGraph)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kFuncDefinesNullptrArg)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kReturnInstrDoesNotMatchFuncSignature),
                  Property("kind", &Issue::kind, IssueKind::kFuncHasNullptrResultType)));
}

TEST(CheckerTest, CatchesFuncHasNoEntryBlock) {
  ir::Program program;
  program.AddFunc();

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind, IssueKind::kFuncHasNoEntryBlock)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kComputedValueUsedInMultipleFunctions)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kComputedValueUsedInMultipleFunctions)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kComputedValueUsedInMultipleFunctions)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kComputedValueNumberUsedMultipleTimes),
                  Property("kind", &Issue::kind, IssueKind::kComputedValueHasMultipleDefinitions)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kComputedValueNumberUsedMultipleTimes),
                  Property("kind", &Issue::kind, IssueKind::kComputedValueHasMultipleDefinitions)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kComputedValueNumberUsedMultipleTimes),
                  Property("kind", &Issue::kind, IssueKind::kComputedValueHasMultipleDefinitions)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kComputedValueHasNoDefinition)));
}

TEST(CheckerTest, CatchesComputedValueHasMultipleDefinitions) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto value = std::make_shared<ir::Computed>(ir::u16(), /*vnum=*/0);
  func->args().push_back(value);
  func->result_types().push_back(ir::u16());
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::IntUnaryInstr>(value, Int::UnaryOp::kNeg, value));
  block->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{value}));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(Property("kind", &Issue::kind, IssueKind::kComputedValueHasMultipleDefinitions)));
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

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(Property("kind", &Issue::kind,
                                   IssueKind::kComputedValueDefinitionDoesNotDominateUse)));
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
      value_c, Int::CompareOp::kLeq, value_a, ir::ToIntConstant(Int(int64_t{10}))));
  block_b->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(value_c, block_c->number(), block_d->number()));

  block_c->instrs().push_back(
      std::make_unique<ir::IntBinaryInstr>(value_d, Int::BinaryOp::kAdd, value_b, value_a));
  block_c->instrs().push_back(
      std::make_unique<ir::IntBinaryInstr>(value_e, Int::BinaryOp::kAdd, value_a, ir::I64One()));
  block_c->instrs().push_back(std::make_unique<ir::JumpInstr>(block_b->number()));

  block_d->instrs().push_back(
      std::make_unique<ir::ReturnInstr>(std::vector<std::shared_ptr<ir::Value>>{value_b}));

  FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

}  // namespace
