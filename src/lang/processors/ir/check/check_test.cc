//
//  checker_test.cc
//  Katara
//
//  Created by Arne Philipeit on 6/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/lang/processors/ir/check/check.h"

#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/print.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"

using ::common::positions::FileSet;
using ::ir_issues::Issue;
using ::ir_issues::IssueKind;
using ::lang::ir_check::CheckProgram;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Property;

TEST(CheckerTest, CatchesMakeSharedPointerInstrResultDoesNotHaveSharedPointerType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto result = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<lang::ir_ext::MakeSharedPointerInstr>(result, ir::I64One()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property(
                  "kind", &Issue::kind,
                  IssueKind::kLangMakeSharedPointerInstrResultDoesNotHaveSharedPointerType))));
}

TEST(CheckerTest, CatchesMakeSharedPointerInstrResultIsNotAStrongSharedPointer) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto result = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/false, ir::u32())),
      /*vnum=*/0);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<lang::ir_ext::MakeSharedPointerInstr>(result, ir::I64One()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property(
                  "kind", &Issue::kind,
                  IssueKind::kLangMakeSharedPointerInstrResultIsNotAStrongSharedPointer))));
}

TEST(CheckerTest, CatchesMakeSharedPointerInstrSizeDoesNotHaveI64Type) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto result = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/true, ir::u32())),
      /*vnum=*/0);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<lang::ir_ext::MakeSharedPointerInstr>(result, ir::I32Zero()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 IssueKind::kLangMakeSharedPointerInstrSizeDoesNotHaveI64Type))));
}

TEST(CheckerTest, CatchesCopySharedPointerInstrResultDoesNotHaveSharedPointerType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/false, ir::u32())),
      /*vnum=*/0);
  func->args().push_back(arg);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/1);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::CopySharedPointerInstr>(
      result, arg, /*pointer_offset=*/ir::I64Zero()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property(
                  "kind", &Issue::kind,
                  IssueKind::kLangCopySharedPointerInstrResultDoesNotHaveSharedPointerType))));
}

TEST(CheckerTest, CatchesCopySharedPointerInstrCopiedDoesNotHaveSharedPointerType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto arg = std::make_shared<ir::Computed>(ir::pointer_type(),
                                            /*vnum=*/0);
  func->args().push_back(arg);
  auto result = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/false, ir::u32())),
      /*vnum=*/1);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::CopySharedPointerInstr>(
      result, arg, /*pointer_offset=*/ir::I64Zero()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property(
                  "kind", &Issue::kind,
                  IssueKind::kLangCopySharedPointerInstrCopiedDoesNotHaveSharedPointerType))));
}

TEST(CheckerTest, CatchesCopySharedPointerInstrOffsetDoesNotHaveI64Type) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto copied = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/true, ir::u32())),
      /*vnum=*/0);
  func->args().push_back(copied);
  auto result = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/false, ir::u32())),
      /*vnum=*/1);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::CopySharedPointerInstr>(
      result, copied, /*pointer_offset=*/ir::U64Zero()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 IssueKind::kLangCopySharedPointerInstrOffsetDoesNotHaveI64Type))));
}

TEST(CheckerTest, CatchesCopySharedPointerInstrResultAndCopiedHaveDifferentElementTypes) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto copied = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/true, ir::i32())),
      /*vnum=*/0);
  func->args().push_back(copied);
  auto result = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/false, ir::u32())),
      /*vnum=*/1);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::CopySharedPointerInstr>(
      result, copied, /*pointer_offset=*/ir::I64Zero()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(AllOf(Property(
          "kind", &Issue::kind,
          IssueKind::kLangCopySharedPointerInstrResultAndCopiedHaveDifferentElementTypes))));
}

TEST(CheckerTest, CatchesCopySharedPointerInstrConvertsFromWeakToStrongSharedPointer) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto copied = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/false, ir::i16())),
      /*vnum=*/0);
  func->args().push_back(copied);
  auto result = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/true, ir::i16())),
      /*vnum=*/1);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::CopySharedPointerInstr>(
      result, copied, /*pointer_offset=*/ir::I64Zero()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property(
                  "kind", &Issue::kind,
                  IssueKind::kLangCopySharedPointerInstrConvertsFromWeakToStrongSharedPointer))));
}

TEST(CheckerTest, CatchesDeleteSharedPointerInstrArgumentDoesNotHaveSharedPointerType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto deleted = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  func->args().push_back(deleted);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::DeleteSharedPointerInstr>(deleted));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property(
                  "kind", &Issue::kind,
                  IssueKind::kLangDeleteSharedPointerInstrArgumentDoesNotHaveSharedPointerType))));
}

TEST(CheckerTest, CatchesMakeUniquePointerInstrResultDoesNotHaveUniquePointerType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto result = std::make_shared<ir::Computed>(ir::u64(), /*vnum=*/0);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<lang::ir_ext::MakeUniquePointerInstr>(result, ir::I64One()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property(
                  "kind", &Issue::kind,
                  IssueKind::kLangMakeUniquePointerInstrResultDoesNotHaveUniquePointerType))));
}

TEST(CheckerTest, CatchesMakeUniquePointerInstrSizeDoesNotHaveI64Type) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto result = std::make_shared<ir::Computed>(
      program.type_table().AddType(std::make_unique<lang::ir_ext::UniquePointer>(ir::i8())),
      /*vnum=*/0);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<lang::ir_ext::MakeUniquePointerInstr>(result, ir::U64Zero()));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 IssueKind::kLangMakeUniquePointerInstrSizeDoesNotHaveI64Type))));
}

TEST(CheckerTest, CatchesDeleteUniquePointerInstrArgumentDoesNotHaveUniquePointerType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto deleted = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  func->args().push_back(deleted);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::DeleteUniquePointerInstr>(deleted));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property(
                  "kind", &Issue::kind,
                  IssueKind::kLangDeleteUniquePointerInstrArgumentDoesNotHaveUniquePointerType))));
}

TEST(CheckerTest, CatchesLoadFromSmartPointerHasMismatchedElementType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto address = std::make_shared<ir::Computed>(
      program.type_table().AddType(
          std::make_unique<lang::ir_ext::SharedPointer>(/*is_strong=*/true, ir::i32())),
      /*vnum=*/0);
  func->args().push_back(address);
  auto result = std::make_shared<ir::Computed>(ir::u32(), /*vnum=*/1);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::LoadInstr>(result, address));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 IssueKind::kLangLoadFromSmartPointerHasMismatchedElementType))));
}

TEST(CheckerTest, CatchesStoreToSmartPointerHasMismatchedElementType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto address = std::make_shared<ir::Computed>(
      program.type_table().AddType(std::make_unique<lang::ir_ext::UniquePointer>(ir::i8())),
      /*vnum=*/0);
  auto value = std::make_shared<ir::Computed>(ir::u8(), /*vnum=*/1);
  func->args().push_back(address);
  func->args().push_back(value);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<ir::StoreInstr>(address, value));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 IssueKind::kLangStoreToSmartPointerHasMismatchedElementType))));
}

TEST(CheckerTest, CatchesStringIndexInstrResultDoesNotHaveI8Type) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto string_operand = std::make_shared<ir::Computed>(lang::ir_ext::string(), /*vnum=*/0);
  auto index_operand = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  func->args().push_back(string_operand);
  func->args().push_back(index_operand);
  auto result = std::make_shared<ir::Computed>(ir::u8(), /*vnum=*/2);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<lang::ir_ext::StringIndexInstr>(result, string_operand, index_operand));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property(
                  "kind", &Issue::kind, IssueKind::kLangStringIndexInstrResultDoesNotHaveI8Type))));
}

TEST(CheckerTest, CatchesStringIndexInstrStringOperandDoesNotHaveStringType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto string_operand = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/0);
  auto index_operand = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  func->args().push_back(string_operand);
  func->args().push_back(index_operand);
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/2);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<lang::ir_ext::StringIndexInstr>(result, string_operand, index_operand));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind,
                           IssueKind::kLangStringIndexInstrStringOperandDoesNotHaveStringType))));
}

TEST(CheckerTest, CatchesStringIndexInstrIndexOperandDoesNotHaveI64Type) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto string_operand = std::make_shared<ir::Computed>(lang::ir_ext::string(), /*vnum=*/0);
  auto index_operand = std::make_shared<ir::Computed>(ir::i32(), /*vnum=*/1);
  func->args().push_back(string_operand);
  func->args().push_back(index_operand);
  auto result = std::make_shared<ir::Computed>(ir::i8(), /*vnum=*/2);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(
      std::make_unique<lang::ir_ext::StringIndexInstr>(result, string_operand, index_operand));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 IssueKind::kLangStringIndexInstrIndexOperandDoesNotHaveI64Type))));
}

TEST(CheckerTest, CatchesStringConcatInstrResultDoesNotHaveStringType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto operand1 = std::make_shared<ir::Computed>(lang::ir_ext::string(), /*vnum=*/0);
  auto operand2 = std::make_shared<ir::Computed>(lang::ir_ext::string(), /*vnum=*/1);
  func->args().push_back(operand1);
  func->args().push_back(operand2);
  auto result = std::make_shared<ir::Computed>(ir::pointer_type(), /*vnum=*/2);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::StringConcatInstr>(
      result, std::vector<std::shared_ptr<ir::Value>>{operand1, operand2}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 IssueKind::kLangStringConcatInstrResultDoesNotHaveStringType))));
}

TEST(CheckerTest, CatchesStringConcatInstrDoesNotHaveArguments) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto result = std::make_shared<ir::Computed>(lang::ir_ext::string(), /*vnum=*/0);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::StringConcatInstr>(
      result, std::vector<std::shared_ptr<ir::Value>>{}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(issue_tracker.issues(),
              ElementsAre(AllOf(Property("kind", &Issue::kind,
                                         IssueKind::kLangStringConcatInstrDoesNotHaveArguments))));
}

TEST(CheckerTest, CatchesStringConcatInstrOperandDoesNotHaveStringType) {
  ir::Program program;
  ir::Func* func = program.AddFunc();
  auto operand1 = std::make_shared<ir::Computed>(lang::ir_ext::string(), /*vnum=*/0);
  auto operand2 = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);
  func->args().push_back(operand1);
  func->args().push_back(operand2);
  auto result = std::make_shared<ir::Computed>(lang::ir_ext::string(), /*vnum=*/2);
  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());
  block->instrs().push_back(std::make_unique<lang::ir_ext::StringConcatInstr>(
      result, std::vector<std::shared_ptr<ir::Value>>{operand1, operand2}));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", &program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram(&program, program_positions, issue_tracker);
  EXPECT_THAT(
      issue_tracker.issues(),
      ElementsAre(AllOf(Property("kind", &Issue::kind,
                                 IssueKind::kLangStringConcatInstrOperandDoesNotHaveStringType))));
}
