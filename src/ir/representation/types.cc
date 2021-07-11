//
//  types.cc
//  Katara
//
//  Created by Arne Philipeit on 5/1/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "types.h"

namespace ir {

bool IsAtomicType(TypeKind type_kind) {
  switch (type_kind) {
    case TypeKind::kBool:
    case TypeKind::kInt:
    case TypeKind::kPointer:
    case TypeKind::kFunc:
      return true;
    default:
      return false;
  }
}

const BoolType kBool;
const IntType kI8{common::IntType::kI8};
const IntType kI16{common::IntType::kI16};
const IntType kI32{common::IntType::kI32};
const IntType kI64{common::IntType::kI64};
const IntType kU8{common::IntType::kU8};
const IntType kU16{common::IntType::kU16};
const IntType kU32{common::IntType::kU32};
const IntType kU64{common::IntType::kU64};
const PointerType kPointer;
const FuncType kFunc;

Type* TypeTable::AddType(std::unique_ptr<Type> type) {
  Type* type_ptr = type.get();
  types_.push_back(std::move(type));
  return type_ptr;
}

}  // namespace ir
