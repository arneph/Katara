//
//  checker.cc
//  Katara
//
//  Created by Arne Philipeit on 3/20/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "checker.h"

#include <sstream>

#include "src/common/logging/logging.h"

namespace lang {
namespace ir_checker {

using ::ir_checker::Issue;

std::vector<::ir_checker::Issue> CheckProgram(const ir::Program* program) {
  Checker checker(program);
  checker.CheckProgram();
  return checker.issues();
}

void AssertProgramIsOkay(const ir::Program* program) {
  std::vector<Issue> issues = CheckProgram(program);
  if (issues.empty()) {
    return;
  }
  std::stringstream buf;
  buf << "IR checker found issues:\n";
  for (const Issue& issue : issues) {
    buf << "[" << int64_t(issue.kind()) << "] " << issue.message() << "\n";
    buf << "\tScope: " << issue.scope_object()->RefString() << "\n";
    if (!issue.involved_objects().empty()) {
      buf << "\tInvolved Objects:\n";
      for (const ir::Object* object : issue.involved_objects()) {
        buf << "\t\t" << object->RefString() << "\n";
      }
    }
  }
  common::fail(buf.str());
}

void Checker::CheckInstr(const ir::Instr* instr, const ir::Block* block, const ir::Func* func) {
  switch (instr->instr_kind()) {
    case ir::InstrKind::kLangPanic:
      break;
    case ir::InstrKind::kLangMakeSharedPointer:
      CheckMakeSharedPointerInstr(static_cast<const ir_ext::MakeSharedPointerInstr*>(instr));
      break;
    case ir::InstrKind::kLangCopySharedPointer:
      CheckCopySharedPointerInstr(static_cast<const ir_ext::CopySharedPointerInstr*>(instr));
      break;
    case ir::InstrKind::kLangDeleteSharedPointer:
      CheckDeleteSharedPointerInstr(static_cast<const ir_ext::DeleteSharedPointerInstr*>(instr));
      break;
    case ir::InstrKind::kLangMakeUniquePointer:
      CheckMakeUniquePointerInstr(static_cast<const ir_ext::MakeUniquePointerInstr*>(instr));
      break;
    case ir::InstrKind::kLangDeleteUniquePointer:
      CheckDeleteUniquePointerInstr(static_cast<const ir_ext::DeleteUniquePointerInstr*>(instr));
      break;
    case ir::InstrKind::kLangStringIndex:
      CheckStringIndexInstr(static_cast<const ir_ext::StringIndexInstr*>(instr));
      break;
    case ir::InstrKind::kLangStringConcat:
      CheckStringConcatInstr(static_cast<const ir_ext::StringConcatInstr*>(instr));
      break;
    default:
      ::ir_checker::Checker::CheckInstr(instr, block, func);
      return;
  }
}

void Checker::CheckMakeSharedPointerInstr(
    const ir_ext::MakeSharedPointerInstr* make_shared_pointer_instr) {
  if (make_shared_pointer_instr->pointer_type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    AddIssue(Issue(make_shared_pointer_instr, {make_shared_pointer_instr->result().get()},
                   Issue::Kind::kLangMakeSharedPointerInstrResultDoesNotHaveSharedPointerType,
                   "lang::ir_ext::MakeSharedPointerInstr result does not have "
                   "lang::ir_ext::SharedPointer type"));
  } else if (!make_shared_pointer_instr->pointer_type()->is_strong()) {
    AddIssue(Issue(make_shared_pointer_instr, {make_shared_pointer_instr->result().get()},
                   Issue::Kind::kLangMakeSharedPointerInstrResultIsNotAStrongSharedPointer,
                   "lang::ir_ext::MakeSharedPointerInstr result is not a strong "
                   "lang::ir_ext::SharedPointer"));
  }
  if (make_shared_pointer_instr->size()->type() != ir::i64()) {
    AddIssue(Issue(make_shared_pointer_instr, {make_shared_pointer_instr->size().get()},
                   Issue::Kind::kLangMakeSharedPointerInstrSizeDoesNotHaveI64Type,
                   "lang::ir_ext::MakeSharedPointerInstr size does not have I64 type"));
  }
}

void Checker::CheckCopySharedPointerInstr(
    const ir_ext::CopySharedPointerInstr* copy_shared_pointer_instr) {
  bool pointers_have_issues = false;
  if (copy_shared_pointer_instr->result()->type()->type_kind() !=
      ir::TypeKind::kLangSharedPointer) {
    AddIssue(Issue(copy_shared_pointer_instr, {copy_shared_pointer_instr->result().get()},
                   Issue::Kind::kLangCopySharedPointerInstrResultDoesNotHaveSharedPointerType,
                   "lang::ir_ext::CopySharedPointerInstr result does not have "
                   "lang::ir_ext::SharedPointer type"));
    pointers_have_issues = true;
  }
  if (copy_shared_pointer_instr->copied_shared_pointer()->type()->type_kind() !=
      ir::TypeKind::kLangSharedPointer) {
    AddIssue(Issue(copy_shared_pointer_instr,
                   {copy_shared_pointer_instr->copied_shared_pointer().get()},
                   Issue::Kind::kLangCopySharedPointerInstrCopiedDoesNotHaveSharedPointerType,
                   "lang::ir_ext::CopySharedPointerInstr copied shared pointer does not have "
                   "lang::ir_ext::SharedPointer type"));
    pointers_have_issues = true;
  }
  if (copy_shared_pointer_instr->underlying_pointer_offset()->type() != ir::i64()) {
    AddIssue(Issue(copy_shared_pointer_instr,
                   {copy_shared_pointer_instr->underlying_pointer_offset().get()},
                   Issue::Kind::kLangCopySharedPointerInstrOffsetDoesNotHaveI64Type,
                   "lang::ir_ext::CopySharedPointerInstr pointer offset does not have I64 type"));
  }
  if (pointers_have_issues) return;
  if (!ir::IsEqual(copy_shared_pointer_instr->copy_pointer_type()->element(),
                   copy_shared_pointer_instr->copied_pointer_type()->element())) {
    AddIssue(Issue(copy_shared_pointer_instr,
                   {copy_shared_pointer_instr->result().get(),
                    copy_shared_pointer_instr->copied_shared_pointer().get()},
                   Issue::Kind::kLangCopySharedPointerInstrResultAndCopiedHaveDifferentElementTypes,
                   "lang::ir_ext::CopySharedPointerInstr result and copied "
                   "lang::ir_ext::SharedPointer have different element types"));
  }
  if (copy_shared_pointer_instr->copy_pointer_type()->is_strong() &&
      !copy_shared_pointer_instr->copied_pointer_type()->is_strong()) {
    AddIssue(Issue(copy_shared_pointer_instr,
                   {copy_shared_pointer_instr->result().get(),
                    copy_shared_pointer_instr->copied_shared_pointer().get()},
                   Issue::Kind::kLangCopySharedPointerInstrConvertsFromWeakToStrongSharedPointer,
                   "lang::ir_ext::CopySharedPointerInstr converts from weak to strong "
                   "lang::ir_ext::SharedPointer"));
  }
}

void Checker::CheckDeleteSharedPointerInstr(
    const ir_ext::DeleteSharedPointerInstr* delete_shared_pointer_instr) {
  if (delete_shared_pointer_instr->pointer_type()->type_kind() !=
      ir::TypeKind::kLangSharedPointer) {
    AddIssue(Issue(delete_shared_pointer_instr,
                   {delete_shared_pointer_instr->deleted_shared_pointer().get()},
                   Issue::Kind::kLangDeleteSharedPointerInstrArgumentDoesNotHaveSharedPointerType,
                   "lang::ir_ext::DeleteSharedPointerInstr argument does not have "
                   "lang::ir_ext::SharedPointer type"));
  }
}

void Checker::CheckMakeUniquePointerInstr(
    const ir_ext::MakeUniquePointerInstr* make_unique_pointer_instr) {
  if (make_unique_pointer_instr->pointer_type()->type_kind() != ir::TypeKind::kLangUniquePointer) {
    AddIssue(Issue(make_unique_pointer_instr, {make_unique_pointer_instr->result().get()},
                   Issue::Kind::kLangMakeUniquePointerInstrResultDoesNotHaveUniquePointerType,
                   "lang::ir_ext::MakeUniquePointerInstr result does not have "
                   "lang::ir_ext::UniquePointer type"));
  }
  if (make_unique_pointer_instr->size()->type() != ir::i64()) {
    AddIssue(Issue(make_unique_pointer_instr, {make_unique_pointer_instr->size().get()},
                   Issue::Kind::kLangMakeUniquePointerInstrSizeDoesNotHaveI64Type,
                   "lang::ir_ext::MakeUniquePointerInstr size does not have I64 type"));
  }
}

void Checker::CheckDeleteUniquePointerInstr(
    const ir_ext::DeleteUniquePointerInstr* delete_unique_pointer_instr) {
  if (delete_unique_pointer_instr->pointer_type()->type_kind() !=
      ir::TypeKind::kLangUniquePointer) {
    AddIssue(Issue(delete_unique_pointer_instr,
                   {delete_unique_pointer_instr->deleted_unique_pointer().get()},
                   Issue::Kind::kLangDeleteUniquePointerInstrArgumentDoesNotHaveUniquePointerType,
                   "lang::ir_ext::DeleteUniquePointerInstr argument does not have "
                   "lang::ir_ext::SharedPointer type"));
  }
}

void Checker::CheckLoadInstr(const ir::LoadInstr* load_instr) {
  if (load_instr->address() == nullptr || load_instr->address()->type() == nullptr ||
      (load_instr->address()->type()->type_kind() != ir::TypeKind::kLangSharedPointer &&
       load_instr->address()->type()->type_kind() != ir::TypeKind::kLangUniquePointer)) {
    ::ir_checker::Checker::CheckLoadInstr(load_instr);
    return;
  }
  auto smart_ptr = static_cast<const ir_ext::SmartPointer*>(load_instr->address()->type());
  if (load_instr->result()->type() != smart_ptr->element()) {
    AddIssue(Issue(load_instr, {load_instr->address().get(), load_instr->result().get()},
                   Issue::Kind::kLangLoadFromSmartPointerHasMismatchedElementType,
                   "ir::LoadInstr lang::ir_ext::SmartPointer does not match result type"));
  }
}

void Checker::CheckStoreInstr(const ir::StoreInstr* store_instr) {
  if (store_instr->address() == nullptr || store_instr->address()->type() == nullptr ||
      (store_instr->address()->type()->type_kind() != ir::TypeKind::kLangSharedPointer &&
       store_instr->address()->type()->type_kind() != ir::TypeKind::kLangUniquePointer)) {
    ::ir_checker::Checker::CheckStoreInstr(store_instr);
    return;
  }
  auto smart_ptr = static_cast<const ir_ext::SmartPointer*>(store_instr->address()->type());
  if (store_instr->value()->type() != smart_ptr->element()) {
    AddIssue(Issue(store_instr, {store_instr->address().get(), store_instr->value().get()},
                   Issue::Kind::kLangStoreToSmartPointerHasMismatchedElementType,
                   "ir::StoreInstr lang::ir_ext::SmartPointer does not match result type"));
  }
}

void Checker::CheckStringIndexInstr(const ir_ext::StringIndexInstr* string_index_instr) {
  if (string_index_instr->result()->type() != ir::i8()) {
    AddIssue(Issue(string_index_instr, {string_index_instr->result().get()},
                   Issue::Kind::kLangStringIndexInstrResultDoesNotHaveI8Type,
                   "lang::ir_ext::StringIndexInstr result does not have I8 type"));
  }
  if (string_index_instr->string_operand()->type() != lang::ir_ext::string()) {
    AddIssue(Issue(
        string_index_instr, {string_index_instr->string_operand().get()},
        Issue::Kind::kLangStringIndexInstrStringOperandDoesNotHaveStringType,
        "lang::ir_ext::StringIndexInstr string operand does not have lang::ir_ext::String type"));
  }
  if (string_index_instr->index_operand()->type() != ir::i64()) {
    AddIssue(Issue(string_index_instr, {string_index_instr->index_operand().get()},
                   Issue::Kind::kLangStringIndexInstrIndexOperandDoesNotHaveI64Type,
                   "lang::ir_ext::StringIndexInstr index operand does not have I64 type"));
  }
}

void Checker::CheckStringConcatInstr(const ir_ext::StringConcatInstr* string_concat_instr) {
  if (string_concat_instr->result()->type() != lang::ir_ext::string()) {
    AddIssue(
        Issue(string_concat_instr, {string_concat_instr->result().get()},
              Issue::Kind::kLangStringConcatInstrResultDoesNotHaveStringType,
              "lang::ir_ext::StringConcatInstr result does not have lang::ir_ext::String type"));
  }
  if (string_concat_instr->operands().empty()) {
    AddIssue(Issue(string_concat_instr, {}, Issue::Kind::kLangStringConcatInstrDoesNotHaveArguments,
                   "lang::ir_ext::StringConcatInstr does not have any arguments"));
  }
  for (const std::shared_ptr<ir::Value>& operand : string_concat_instr->operands()) {
    if (operand->type() != lang::ir_ext::string()) {
      AddIssue(
          Issue(string_concat_instr, {operand.get()},
                Issue::Kind::kLangStringConcatInstrOperandDoesNotHaveStringType,
                "lang::ir_ext::StringConcatInstr operand does not have lang::ir_ext::String type"));
    }
  }
}

}  // namespace ir_checker
}  // namespace lang
