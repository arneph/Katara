//
//  checker_test.cc
//  Katara
//
//  Created by Arne Philipeit on 6/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/lang/processors/ir/checker/checker.h"

#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/checker/issues.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"

using ::ir_checker::Issue;
using ::lang::ir_checker::CheckProgram;
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangMakeSharedPointerInstrResultDoesNotHaveSharedPointerType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangMakeSharedPointerInstrResultIsNotAStrongSharedPointer),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
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

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind,
                           Issue::Kind::kLangMakeSharedPointerInstrSizeDoesNotHaveI64Type),
                  Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(ir::I32Zero().get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangCopySharedPointerInstrResultDoesNotHaveSharedPointerType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangCopySharedPointerInstrCopiedDoesNotHaveSharedPointerType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(arg.get())))));
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

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind,
                           Issue::Kind::kLangCopySharedPointerInstrOffsetDoesNotHaveI64Type),
                  Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(ir::U64Zero().get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property(
              "kind", &Issue::kind,
              Issue::Kind::kLangCopySharedPointerInstrResultAndCopiedHaveDifferentElementTypes),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects,
                   ElementsAre(result.get(), copied.get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangCopySharedPointerInstrConvertsFromWeakToStrongSharedPointer),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects,
                   ElementsAre(result.get(), copied.get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangDeleteSharedPointerInstrArgumentDoesNotHaveSharedPointerType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(deleted.get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangMakeUniquePointerInstrResultDoesNotHaveUniquePointerType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
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

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind,
                           Issue::Kind::kLangMakeUniquePointerInstrSizeDoesNotHaveI64Type),
                  Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(ir::U64Zero().get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangDeleteUniquePointerInstrArgumentDoesNotHaveUniquePointerType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(deleted.get())))));
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

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind,
                           Issue::Kind::kLangLoadFromSmartPointerHasMismatchedElementType),
                  Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(address.get(), result.get())))));
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

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind,
                           Issue::Kind::kLangStoreToSmartPointerHasMismatchedElementType),
                  Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(address.get(), value.get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kLangStringIndexInstrResultDoesNotHaveI8Type),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
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

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind,
                           Issue::Kind::kLangStringIndexInstrStringOperandDoesNotHaveStringType),
                  Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(string_operand.get())))));
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

  EXPECT_THAT(CheckProgram(&program),
              ElementsAre(AllOf(
                  Property("kind", &Issue::kind,
                           Issue::Kind::kLangStringIndexInstrIndexOperandDoesNotHaveI64Type),
                  Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
                  Property("involved_objects", &Issue::involved_objects,
                           ElementsAre(index_operand.get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangStringConcatInstrResultDoesNotHaveStringType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(result.get())))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind, Issue::Kind::kLangStringConcatInstrDoesNotHaveArguments),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, IsEmpty()))));
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

  EXPECT_THAT(
      CheckProgram(&program),
      ElementsAre(AllOf(
          Property("kind", &Issue::kind,
                   Issue::Kind::kLangStringConcatInstrOperandDoesNotHaveStringType),
          Property("scope_object", &Issue::scope_object, block->instrs().front().get()),
          Property("involved_objects", &Issue::involved_objects, ElementsAre(operand2.get())))));
}
