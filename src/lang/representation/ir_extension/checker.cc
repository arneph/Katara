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
    buf << "\tScope: " << issue.scope_object()->ToString() << "\n";
    if (!issue.involved_objects().empty()) {
      buf << "\tInvolved Objects:\n";
      for (const ir::Object* object : issue.involved_objects()) {
        buf << "\t\t" << object->ToString() << "\n";
      }
    }
  }
  common::fail(buf.str());
}

void Checker::CheckInstr(const ir::Instr* instr, const ir::Block* block, const ir::Func* func) {
  switch (instr->instr_kind()) {
    case ir::InstrKind::kLangPanic:
    case ir::InstrKind::kLangMakeSharedPointer:
    case ir::InstrKind::kLangCopySharedPointer:
    case ir::InstrKind::kLangDeleteSharedPointer:
    case ir::InstrKind::kLangStringIndex:
    case ir::InstrKind::kLangStringConcat:
      // TODO: implement checks
      return;
    default:
      ::ir_checker::Checker::CheckInstr(instr, block, func);
      return;
  }
}

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
