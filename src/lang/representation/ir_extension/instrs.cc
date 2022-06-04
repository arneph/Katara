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
  auto that = static_cast<const PanicInstr&>(that_instr);
  if (reason() != that.reason()) return false;
  return true;
}

MakeSharedPointerInstr::MakeSharedPointerInstr(std::shared_ptr<ir::Computed> result)
    : ir::Computation(result) {
  if (result->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    common::fail(
        "attempted to create make shared pointer instr with non-shared pointer "
        "result");
  } else if (!pointer_type()->is_strong()) {
    common::fail(
        "attempted to create make shared pointer instr with weak shared pointer "
        "result");
  }
}

const ir_ext::SharedPointer* MakeSharedPointerInstr::pointer_type() const {
  return static_cast<const ir_ext::SharedPointer*>(result()->type());
}

bool MakeSharedPointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangMakeSharedPointer) return false;
  auto that = static_cast<const MakeSharedPointerInstr&>(that_instr);
  if (!ir::IsEqual(result().get(), that.result().get())) return false;
  return true;
}

CopySharedPointerInstr::CopySharedPointerInstr(std::shared_ptr<ir::Computed> result,
                                               std::shared_ptr<ir::Computed> copied_shared_pointer,
                                               std::shared_ptr<ir::Value> pointer_offset)
    : ir::Computation(result),
      copied_shared_pointer_(copied_shared_pointer),
      pointer_offset_(pointer_offset) {
  if (result->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    common::fail(
        "attempted to create copy shared pointer instr with non-shared pointer "
        "result");
  }
  if (copied_shared_pointer->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    common::fail(
        "attempted to create copy shared pointer instr with non-shared pointer "
        "argument");
  }
}

const ir_ext::SharedPointer* CopySharedPointerInstr::copied_pointer_type() const {
  return static_cast<const ir_ext::SharedPointer*>(copied_shared_pointer_->type());
}

const ir_ext::SharedPointer* CopySharedPointerInstr::copy_pointer_type() const {
  return static_cast<const ir_ext::SharedPointer*>(result()->type());
}

bool CopySharedPointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangCopySharedPointer) return false;
  auto that = static_cast<const CopySharedPointerInstr&>(that_instr);
  if (!ir::IsEqual(result().get(), that.result().get())) return false;
  if (!ir::IsEqual(copied_shared_pointer().get(), that.copied_shared_pointer().get())) return false;
  if (!ir::IsEqual(underlying_pointer_offset().get(), that.underlying_pointer_offset().get())) {
    return false;
  }
  return true;
}

DeleteSharedPointerInstr::DeleteSharedPointerInstr(
    std::shared_ptr<ir::Computed> deleted_shared_pointer)
    : deleted_shared_pointer_(deleted_shared_pointer) {
  if (deleted_shared_pointer->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    common::fail(
        "attempted to create delete shared pointer instr with non-shared pointer "
        "argument");
  }
}

const ir_ext::SharedPointer* DeleteSharedPointerInstr::pointer_type() const {
  return static_cast<const ir_ext::SharedPointer*>(deleted_shared_pointer_->type());
}

bool DeleteSharedPointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangDeleteSharedPointer) return false;
  auto that = static_cast<const DeleteSharedPointerInstr&>(that_instr);
  if (!ir::IsEqual(deleted_shared_pointer().get(), that.deleted_shared_pointer().get())) {
    return false;
  }
  return true;
}

MakeUniquePointerInstr::MakeUniquePointerInstr(std::shared_ptr<ir::Computed> result)
    : ir::Computation(result) {
  if (result->type()->type_kind() != ir::TypeKind::kLangUniquePointer) {
    common::fail(
        "attempted to create make unique pointer instr with non-unique pointer "
        "result");
  }
}

const ir_ext::UniquePointer* MakeUniquePointerInstr::pointer_type() const {
  return static_cast<const ir_ext::UniquePointer*>(result()->type());
}

bool MakeUniquePointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangMakeUniquePointer) return false;
  auto that = static_cast<const MakeUniquePointerInstr&>(that_instr);
  if (!ir::IsEqual(result().get(), that.result().get())) return false;
  return true;
}

DeleteUniquePointerInstr::DeleteUniquePointerInstr(
    std::shared_ptr<ir::Computed> deleted_unique_pointer)
    : deleted_unique_pointer_(deleted_unique_pointer) {
  if (deleted_unique_pointer_->type()->type_kind() != ir::TypeKind::kLangUniquePointer) {
    common::fail(
        "attempted to create delete unique pointer instr with non-unique pointer "
        "argument");
  }
}

const ir_ext::UniquePointer* DeleteUniquePointerInstr::pointer_type() const {
  return static_cast<const ir_ext::UniquePointer*>(deleted_unique_pointer_->type());
}

bool DeleteUniquePointerInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangDeleteUniquePointer) return false;
  auto that = static_cast<const DeleteUniquePointerInstr&>(that_instr);
  if (!ir::IsEqual(deleted_unique_pointer().get(), that.deleted_unique_pointer().get())) {
    return false;
  }
  return true;
}

bool StringIndexInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangStringIndex) return false;
  auto that = static_cast<const StringIndexInstr&>(that_instr);
  if (!ir::IsEqual(result().get(), that.result().get())) return false;
  if (!ir::IsEqual(string_operand().get(), that.string_operand().get())) return false;
  if (!ir::IsEqual(index_operand().get(), that.index_operand().get())) return false;
  return true;
}

bool StringConcatInstr::operator==(const ir::Instr& that_instr) const {
  if (that_instr.instr_kind() != ir::InstrKind::kLangStringConcat) return false;
  auto that = static_cast<const StringConcatInstr&>(that_instr);
  if (!ir::IsEqual(result().get(), that.result().get())) return false;
  if (operands().size() != that.operands().size()) return false;
  for (std::size_t i = 0; i < operands().size(); i++) {
    const ir::Value* value_a = operands().at(i).get();
    const ir::Value* value_b = that.operands().at(i).get();
    if (!ir::IsEqual(value_a, value_b)) return false;
  }
  return true;
}

}  // namespace ir_ext
}  // namespace lang
