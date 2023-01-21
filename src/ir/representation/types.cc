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

bool IntType::operator==(const Type& that) const {
  if (that.type_kind() != TypeKind::kInt) {
    return false;
  }
  return int_type() == static_cast<const IntType&>(that).int_type();
}

namespace {

const BoolType kBool;
const IntType kI8{common::atomics::IntType::kI8};
const IntType kI16{common::atomics::IntType::kI16};
const IntType kI32{common::atomics::IntType::kI32};
const IntType kI64{common::atomics::IntType::kI64};
const IntType kU8{common::atomics::IntType::kU8};
const IntType kU16{common::atomics::IntType::kU16};
const IntType kU32{common::atomics::IntType::kU32};
const IntType kU64{common::atomics::IntType::kU64};
const PointerType kPointer;
const FuncType kFunc;

}  // namespace

const BoolType* bool_type() { return &kBool; }
const IntType* i8() { return &kI8; }
const IntType* i16() { return &kI16; }
const IntType* i32() { return &kI32; }
const IntType* i64() { return &kI64; }
const IntType* u8() { return &kU8; }
const IntType* u16() { return &kU16; }
const IntType* u32() { return &kU32; }
const IntType* u64() { return &kU64; }

const IntType* IntTypeFor(common::atomics::IntType type) {
  switch (type) {
    case common::atomics::IntType::kI8:
      return &kI8;
    case common::atomics::IntType::kI16:
      return &kI16;
    case common::atomics::IntType::kI32:
      return &kI32;
    case common::atomics::IntType::kI64:
      return &kI64;
    case common::atomics::IntType::kU8:
      return &kU8;
    case common::atomics::IntType::kU16:
      return &kU16;
    case common::atomics::IntType::kU32:
      return &kU32;
    case common::atomics::IntType::kU64:
      return &kU64;
  }
}

const PointerType* pointer_type() { return &kPointer; }
const FuncType* func_type() { return &kFunc; }

Type* TypeTable::AddType(std::unique_ptr<Type> type) {
  Type* type_ptr = type.get();
  types_.push_back(std::move(type));
  return type_ptr;
}

}  // namespace ir
