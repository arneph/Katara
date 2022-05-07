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
namespace ir_ext {

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
      CheckMakeSharedPointerInstr(static_cast<const MakeSharedPointerInstr*>(instr));
      break;
    case ir::InstrKind::kLangCopySharedPointer:
      CheckCopySharedPointerInstr(static_cast<const CopySharedPointerInstr*>(instr));
      break;
    case ir::InstrKind::kLangDeleteSharedPointer:
      CheckDeleteSharedPointerInstr(static_cast<const DeleteSharedPointerInstr*>(instr));
      break;
    case ir::InstrKind::kLangStringIndex:
      CheckStringIndexInstr(static_cast<const StringIndexInstr*>(instr));
      break;
    case ir::InstrKind::kLangStringConcat:
      CheckStringConcatInstr(static_cast<const StringConcatInstr*>(instr));
      break;
    default:
      ::ir_checker::Checker::CheckInstr(instr, block, func);
      return;
  }
}

void Checker::CheckMakeSharedPointerInstr(const MakeSharedPointerInstr* make_shared_pointer_instr) {

}

void Checker::CheckCopySharedPointerInstr(const CopySharedPointerInstr* copy_shared_pointer_instr) {

}

void Checker::CheckDeleteSharedPointerInstr(
    const DeleteSharedPointerInstr* delete_shared_pointer_instr) {}

void Checker::CheckStringIndexInstr(const StringIndexInstr* string_index_instr) {}

void Checker::CheckStringConcatInstr(const StringConcatInstr* string_concat_instr) {}

void Checker::CheckLoadInstr(const ir::LoadInstr* load_instr) {
  if (load_instr->address() == nullptr || load_instr->address()->type() == nullptr ||
      load_instr->address()->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    ::ir_checker::Checker::CheckLoadInstr(load_instr);
    return;
  }
  auto shared_ptr = static_cast<const SharedPointer*>(load_instr->address()->type());
  if (load_instr->result()->type() != shared_ptr->element()) {
    AddIssue(Issue(load_instr, {load_instr->address().get(), load_instr->result().get()},
                   Issue::Kind::kLangLoadFromSharedPointerHasMismatchedElementType,
                   "ir::LoadInstr lang::ir_ext::SharedPointer does not match result type"));
  }
}

void Checker::CheckStoreInstr(const ir::StoreInstr* store_instr) {
  if (store_instr->address() == nullptr || store_instr->address()->type() == nullptr ||
      store_instr->address()->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    ::ir_checker::Checker::CheckStoreInstr(store_instr);
    return;
  }
  auto shared_ptr = static_cast<const SharedPointer*>(store_instr->address()->type());
  if (store_instr->value()->type() != shared_ptr->element()) {
    AddIssue(Issue(store_instr, {store_instr->address().get(), store_instr->value().get()},
                   Issue::Kind::kLangStoreToSharedPointerHasMismatchedElementType,
                   "ir::StoreInstr lang::ir_ext::SharedPointer does not match result type"));
  }
}

}  // namespace ir_ext
}  // namespace lang
