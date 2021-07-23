//
//  instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 5/13/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "instrs.h"

#include "src/common/logging.h"

namespace lang {
namespace ir_ext {

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

std::string CopySharedPointerInstr::ToString() const {
  return result()->ToStringWithType() + " = copy_shared " + copied_shared_pointer_->ToString() +
         ", " + pointer_offset_->ToString();
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

std::string DeleteSharedPointerInstr::ToString() const {
  return "delete_shared " + deleted_shared_pointer_->ToString();
}

std::string StringIndexInstr::ToString() const {
  return result()->ToStringWithType() + " = index:" + string_operand_->type()->ToString() + " " +
         string_operand_->ToString() + ", " + index_operand_->ToString();
}

std::string StringConcatInstr::ToString() const {
  std::string str =
      result()->ToStringWithType() + " = concat:" + operands_.front()->type()->ToString() + " ";
  bool first = true;
  for (auto& operand : operands_) {
    if (first) {
      first = false;
    } else {
      str += ", ";
    }
    str + operand->ToString();
  }
  return str;
}

}  // namespace ir_ext
}  // namespace lang
