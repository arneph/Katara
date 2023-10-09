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

namespace lang::ir_check {

using ::ir_issues::IssueKind;
using ::ir_serialization::BlockPositions;
using ::ir_serialization::FuncPositions;
using ::ir_serialization::InstrPositions;

void Checker::CheckInstr(const ir::Instr* instr,
                         const ir_serialization::InstrPositions& instr_positions,
                         const ir::Block* block, const ir::Func* func,
                         const ir_serialization::FuncPositions& func_positions) {
  switch (instr->instr_kind()) {
    case ir::InstrKind::kLangPanic:
      break;
    case ir::InstrKind::kLangMakeSharedPointer:
      CheckMakeSharedPointerInstr(static_cast<const ir_ext::MakeSharedPointerInstr*>(instr),
                                  instr_positions);
      break;
    case ir::InstrKind::kLangCopySharedPointer:
      CheckCopySharedPointerInstr(static_cast<const ir_ext::CopySharedPointerInstr*>(instr),
                                  instr_positions);
      break;
    case ir::InstrKind::kLangDeleteSharedPointer:
      CheckDeleteSharedPointerInstr(static_cast<const ir_ext::DeleteSharedPointerInstr*>(instr),
                                    instr_positions);
      break;
    case ir::InstrKind::kLangMakeUniquePointer:
      CheckMakeUniquePointerInstr(static_cast<const ir_ext::MakeUniquePointerInstr*>(instr),
                                  instr_positions);
      break;
    case ir::InstrKind::kLangDeleteUniquePointer:
      CheckDeleteUniquePointerInstr(static_cast<const ir_ext::DeleteUniquePointerInstr*>(instr),
                                    instr_positions);
      break;
    case ir::InstrKind::kLangStringIndex:
      CheckStringIndexInstr(static_cast<const ir_ext::StringIndexInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kLangStringConcat:
      CheckStringConcatInstr(static_cast<const ir_ext::StringConcatInstr*>(instr), instr_positions);
      break;
    default:
      ::ir_check::Checker::CheckInstr(instr, instr_positions, block, func, func_positions);
      return;
  }
}

void Checker::CheckMakeSharedPointerInstr(
    const ir_ext::MakeSharedPointerInstr* make_shared_pointer_instr,
    const InstrPositions& make_shared_pointer_instr_positions) {
  if (make_shared_pointer_instr->pointer_type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    issue_tracker().Add(IssueKind::kLangMakeSharedPointerInstrResultDoesNotHaveSharedPointerType,
                        make_shared_pointer_instr_positions.defined_value_ranges().at(0),
                        "lang::ir_ext::MakeSharedPointerInstr result does not have "
                        "lang::ir_ext::SharedPointer type");
  } else if (!make_shared_pointer_instr->pointer_type()->is_strong()) {
    issue_tracker().Add(IssueKind::kLangMakeSharedPointerInstrResultIsNotAStrongSharedPointer,
                        make_shared_pointer_instr_positions.defined_value_ranges().at(0),
                        "lang::ir_ext::MakeSharedPointerInstr result is not a strong "
                        "lang::ir_ext::SharedPointer");
  }
  if (make_shared_pointer_instr->size()->type() != ir::i64()) {
    issue_tracker().Add(IssueKind::kLangMakeSharedPointerInstrSizeDoesNotHaveI64Type,
                        make_shared_pointer_instr_positions.used_value_ranges().at(0),
                        "lang::ir_ext::MakeSharedPointerInstr size does not have I64 type");
  }
}

void Checker::CheckCopySharedPointerInstr(
    const ir_ext::CopySharedPointerInstr* copy_shared_pointer_instr,
    const InstrPositions& copy_shared_pointer_instr_positions) {
  bool pointers_have_issues = false;
  if (copy_shared_pointer_instr->result()->type()->type_kind() !=
      ir::TypeKind::kLangSharedPointer) {
    issue_tracker().Add(IssueKind::kLangCopySharedPointerInstrResultDoesNotHaveSharedPointerType,
                        copy_shared_pointer_instr_positions.defined_value_ranges().at(0),
                        "lang::ir_ext::CopySharedPointerInstr result does not have "
                        "lang::ir_ext::SharedPointer type");
    pointers_have_issues = true;
  }
  if (copy_shared_pointer_instr->copied_shared_pointer()->type()->type_kind() !=
      ir::TypeKind::kLangSharedPointer) {
    issue_tracker().Add(IssueKind::kLangCopySharedPointerInstrCopiedDoesNotHaveSharedPointerType,
                        copy_shared_pointer_instr_positions.used_value_ranges().at(0),
                        "lang::ir_ext::CopySharedPointerInstr copied shared pointer does not have "
                        "lang::ir_ext::SharedPointer type");
    pointers_have_issues = true;
  }
  if (copy_shared_pointer_instr->underlying_pointer_offset()->type() != ir::i64()) {
    issue_tracker().Add(IssueKind::kLangCopySharedPointerInstrOffsetDoesNotHaveI64Type,
                        copy_shared_pointer_instr_positions.used_value_ranges().at(1),
                        "lang::ir_ext::CopySharedPointerInstr pointer offset does not have I64 "
                        "type");
  }
  if (pointers_have_issues) return;
  if (!ir::IsEqual(copy_shared_pointer_instr->copy_pointer_type()->element(),
                   copy_shared_pointer_instr->copied_pointer_type()->element())) {
    issue_tracker().Add(
        IssueKind::kLangCopySharedPointerInstrResultAndCopiedHaveDifferentElementTypes,
        {copy_shared_pointer_instr_positions.defined_value_ranges().at(0),
         copy_shared_pointer_instr_positions.used_value_ranges().at(0)},
        "lang::ir_ext::CopySharedPointerInstr result and copied "
        "lang::ir_ext::SharedPointer have different element types");
  }
  if (copy_shared_pointer_instr->copy_pointer_type()->is_strong() &&
      !copy_shared_pointer_instr->copied_pointer_type()->is_strong()) {
    issue_tracker().Add(IssueKind::kLangCopySharedPointerInstrConvertsFromWeakToStrongSharedPointer,
                        {copy_shared_pointer_instr_positions.defined_value_ranges().at(0),
                         copy_shared_pointer_instr_positions.used_value_ranges().at(0)},
                        "lang::ir_ext::CopySharedPointerInstr converts from weak to strong "
                        "lang::ir_ext::SharedPointer");
  }
}

void Checker::CheckDeleteSharedPointerInstr(
    const ir_ext::DeleteSharedPointerInstr* delete_shared_pointer_instr,
    const InstrPositions& delete_shared_pointer_instr_positions) {
  if (delete_shared_pointer_instr->pointer_type()->type_kind() !=
      ir::TypeKind::kLangSharedPointer) {
    issue_tracker().Add(
        IssueKind::kLangDeleteSharedPointerInstrArgumentDoesNotHaveSharedPointerType,
        delete_shared_pointer_instr_positions.used_value_ranges().at(0),
        "lang::ir_ext::DeleteSharedPointerInstr argument does not have "
        "lang::ir_ext::UniquePointer type");
  }
}

void Checker::CheckMakeUniquePointerInstr(
    const ir_ext::MakeUniquePointerInstr* make_unique_pointer_instr,
    const InstrPositions& make_unique_pointer_instr_positions) {
  if (make_unique_pointer_instr->pointer_type()->type_kind() != ir::TypeKind::kLangUniquePointer) {
    issue_tracker().Add(IssueKind::kLangMakeUniquePointerInstrResultDoesNotHaveUniquePointerType,
                        make_unique_pointer_instr_positions.defined_value_ranges().at(0),
                        "lang::ir_ext::MakeUniquePointerInstr result does not have "
                        "lang::ir_ext::UniquePointer type");
  }
  if (make_unique_pointer_instr->size()->type() != ir::i64()) {
    issue_tracker().Add(IssueKind::kLangMakeUniquePointerInstrSizeDoesNotHaveI64Type,
                        make_unique_pointer_instr_positions.used_value_ranges().at(0),
                        "lang::ir_ext::MakeUniquePointerInstr size does not have I64 type");
  }
}

void Checker::CheckDeleteUniquePointerInstr(
    const ir_ext::DeleteUniquePointerInstr* delete_unique_pointer_instr,
    const InstrPositions& delete_unique_pointer_instr_positions) {
  if (delete_unique_pointer_instr->pointer_type()->type_kind() !=
      ir::TypeKind::kLangUniquePointer) {
    issue_tracker().Add(
        IssueKind::kLangDeleteUniquePointerInstrArgumentDoesNotHaveUniquePointerType,
        delete_unique_pointer_instr_positions.used_value_ranges().at(0),
        "lang::ir_ext::DeleteUniquePointerInstr argument does not have "
        "lang::ir_ext::UniquePointer type");
  }
}

void Checker::CheckLoadInstr(const ir::LoadInstr* load_instr,
                             const InstrPositions& load_instr_positions) {
  if (load_instr->address() == nullptr || load_instr->address()->type() == nullptr ||
      (load_instr->address()->type()->type_kind() != ir::TypeKind::kLangSharedPointer &&
       load_instr->address()->type()->type_kind() != ir::TypeKind::kLangUniquePointer)) {
    ::ir_check::Checker::CheckLoadInstr(load_instr, load_instr_positions);
    return;
  }
  auto smart_ptr = static_cast<const ir_ext::SmartPointer*>(load_instr->address()->type());
  if (!ir::IsEqual(load_instr->result()->type(), smart_ptr->element())) {
    issue_tracker().Add(IssueKind::kLangLoadFromSmartPointerHasMismatchedElementType,
                        {load_instr_positions.defined_value_ranges().at(0),
                         load_instr_positions.used_value_ranges().at(0)},
                        "ir::LoadInstr lang::ir_ext::SmartPointer does not match result type");
  }
}

void Checker::CheckStoreInstr(const ir::StoreInstr* store_instr,
                              const InstrPositions& store_instr_positions) {
  if (store_instr->address() == nullptr || store_instr->address()->type() == nullptr ||
      (store_instr->address()->type()->type_kind() != ir::TypeKind::kLangSharedPointer &&
       store_instr->address()->type()->type_kind() != ir::TypeKind::kLangUniquePointer)) {
    ::ir_check::Checker::CheckStoreInstr(store_instr, store_instr_positions);
    return;
  }
  auto smart_ptr = static_cast<const ir_ext::SmartPointer*>(store_instr->address()->type());
  if (ir::IsEqual(store_instr->value().get(), ir::NilPointer().get())) {
    return;
  } else if (!ir::IsEqual(store_instr->value()->type(), smart_ptr->element())) {
    issue_tracker().Add(IssueKind::kLangStoreToSmartPointerHasMismatchedElementType,
                        {store_instr_positions.used_value_ranges().at(0),
                         store_instr_positions.used_value_ranges().at(1)},
                        "ir::StoreInstr lang::ir_ext::SmartPointer does not match result type");
  }
}

void Checker::CheckMovInstr(const ir::MovInstr* mov_instr,
                            const InstrPositions& mov_instr_positions) {
  if ((mov_instr->result()->type()->type_kind() == ir::TypeKind::kLangSharedPointer ||
       mov_instr->result()->type()->type_kind() == ir::TypeKind::kLangUniquePointer) &&
      ir::IsEqual(mov_instr->origin().get(), ir::NilPointer().get())) {
    return;
  } else {
    return ::ir_check::Checker::CheckMovInstr(mov_instr, mov_instr_positions);
  }
}

void Checker::CheckStringIndexInstr(const ir_ext::StringIndexInstr* string_index_instr,
                                    const InstrPositions& string_index_instr_positions) {
  if (string_index_instr->result()->type() != ir::i8()) {
    issue_tracker().Add(IssueKind::kLangStringIndexInstrResultDoesNotHaveI8Type,
                        string_index_instr_positions.defined_value_ranges().at(0),
                        "lang::ir_ext::StringIndexInstr result does not have I8 type");
  }
  if (string_index_instr->string_operand()->type() != lang::ir_ext::string()) {
    issue_tracker().Add(IssueKind::kLangStringIndexInstrStringOperandDoesNotHaveStringType,
                        string_index_instr_positions.used_value_ranges().at(0),
                        "lang::ir_ext::StringIndexInstr string operand does not have "
                        "lang::ir_ext::String type");
  }
  if (string_index_instr->index_operand()->type() != ir::i64()) {
    issue_tracker().Add(IssueKind::kLangStringIndexInstrIndexOperandDoesNotHaveI64Type,
                        string_index_instr_positions.used_value_ranges().at(1),
                        "lang::ir_ext::StringIndexInstr index operand does not have I64 type");
  }
}

void Checker::CheckStringConcatInstr(const ir_ext::StringConcatInstr* string_concat_instr,
                                     const InstrPositions& string_concat_instr_positions) {
  if (string_concat_instr->result()->type() != lang::ir_ext::string()) {
    issue_tracker().Add(IssueKind::kLangStringConcatInstrResultDoesNotHaveStringType,
                        string_concat_instr_positions.defined_value_ranges().at(0),
                        "lang::ir_ext::StringConcatInstr result does not have "
                        "lang::ir_ext::String type");
  }
  if (string_concat_instr->operands().empty()) {
    issue_tracker().Add(IssueKind::kLangStringConcatInstrDoesNotHaveArguments,
                        string_concat_instr_positions.entire_instr(),
                        "lang::ir_ext::StringConcatInstr does not have any arguments");
  }
  for (std::size_t i = 0; i < string_concat_instr->operands().size(); i++) {
    const std::shared_ptr<ir::Value>& operand = string_concat_instr->operands().at(i);
    if (operand->type() != lang::ir_ext::string()) {
      issue_tracker().Add(IssueKind::kLangStringConcatInstrOperandDoesNotHaveStringType,
                          string_concat_instr_positions.used_value_ranges().at(i),
                          "lang::ir_ext::StringConcatInstr operand does not have "
                          "lang::ir_ext::String type");
    }
  }
}

}  // namespace lang::ir_check
