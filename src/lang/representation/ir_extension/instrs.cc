//
//  instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 5/13/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "instrs.h"

#include "src/common/logging/logging.h"

namespace lang {
namespace ir_ext {

bool PanicInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangPanic) return false;
  auto that = static_cast<const PanicInstr*>(&that_instr);
  if (!ir::IsEqual(reason().get(), that->reason().get())) return false;
  return true;
}

const ir_ext::SharedPointer* MakeSharedPointerInstr::pointer_type() const {
  return static_cast<const ir_ext::SharedPointer*>(result()->type());
}

bool MakeSharedPointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangMakeSharedPointer) return false;
  auto that = static_cast<const MakeSharedPointerInstr*>(&that_instr);
  if (!ir::IsEqual(result().get(), that->result().get())) return false;
  return true;
}

const ir_ext::SharedPointer* CopySharedPointerInstr::copied_pointer_type() const {
  return static_cast<const ir_ext::SharedPointer*>(copied_shared_pointer_->type());
}

const ir_ext::SharedPointer* CopySharedPointerInstr::copy_pointer_type() const {
  return static_cast<const ir_ext::SharedPointer*>(result()->type());
}

bool CopySharedPointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangCopySharedPointer) return false;
  auto that = static_cast<const CopySharedPointerInstr*>(&that_instr);
  if (!ir::IsEqual(result().get(), that->result().get())) return false;
  if (!ir::IsEqual(copied_shared_pointer().get(), that->copied_shared_pointer().get()))
    return false;
  if (!ir::IsEqual(underlying_pointer_offset().get(), that->underlying_pointer_offset().get())) {
    return false;
  }
  return true;
}

const ir_ext::SharedPointer* DeleteSharedPointerInstr::pointer_type() const {
  return static_cast<const ir_ext::SharedPointer*>(deleted_shared_pointer_->type());
}

bool DeleteSharedPointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangDeleteSharedPointer) return false;
  auto that = static_cast<const DeleteSharedPointerInstr*>(&that_instr);
  if (!ir::IsEqual(deleted_shared_pointer().get(), that->deleted_shared_pointer().get())) {
    return false;
  }
  return true;
}

const ir_ext::UniquePointer* MakeUniquePointerInstr::pointer_type() const {
  return static_cast<const ir_ext::UniquePointer*>(result()->type());
}

bool MakeUniquePointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangMakeUniquePointer) return false;
  auto that = static_cast<const MakeUniquePointerInstr*>(&that_instr);
  if (!ir::IsEqual(result().get(), that->result().get())) return false;
  return true;
}

const ir_ext::UniquePointer* DeleteUniquePointerInstr::pointer_type() const {
  return static_cast<const ir_ext::UniquePointer*>(deleted_unique_pointer_->type());
}

bool DeleteUniquePointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangDeleteUniquePointer) return false;
  auto that = static_cast<const DeleteUniquePointerInstr*>(&that_instr);
  if (!ir::IsEqual(deleted_unique_pointer().get(), that->deleted_unique_pointer().get())) {
    return false;
  }
  return true;
}

bool StringIndexInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangStringIndex) return false;
  auto that = static_cast<const StringIndexInstr*>(&that_instr);
  if (!ir::IsEqual(result().get(), that->result().get())) return false;
  if (!ir::IsEqual(string_operand().get(), that->string_operand().get())) return false;
  if (!ir::IsEqual(index_operand().get(), that->index_operand().get())) return false;
  return true;
}

bool StringConcatInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangStringConcat) return false;
  auto that = static_cast<const StringConcatInstr*>(&that_instr);
  if (!ir::IsEqual(result().get(), that->result().get())) return false;
  if (operands().size() != that->operands().size()) return false;
  for (std::size_t i = 0; i < operands().size(); i++) {
    const ir::Value* value_a = operands().at(i).get();
    const ir::Value* value_b = that->operands().at(i).get();
    if (!ir::IsEqual(value_a, value_b)) return false;
  }
  return true;
}

}  // namespace ir_ext
}  // namespace lang
