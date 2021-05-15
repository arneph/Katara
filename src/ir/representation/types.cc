//
//  types.cc
//  Katara
//
//  Created by Arne Philipeit on 5/1/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "types.h"

#include <array>

namespace ir {

bool IsIntegral(AtomicKind type) {
  switch (type) {
    case AtomicKind::kBool:
    case AtomicKind::kI8:
    case AtomicKind::kI16:
    case AtomicKind::kI32:
    case AtomicKind::kI64:
    case AtomicKind::kU8:
    case AtomicKind::kU16:
    case AtomicKind::kU32:
    case AtomicKind::kU64:
    case AtomicKind::kPtr:
      return true;
    default:
      return false;
  }
}

bool IsUnsigned(AtomicKind type) {
  switch (type) {
    case AtomicKind::kBool:
    case AtomicKind::kU8:
    case AtomicKind::kU16:
    case AtomicKind::kU32:
    case AtomicKind::kU64:
    case AtomicKind::kPtr:
      return true;
    case AtomicKind::kI8:
    case AtomicKind::kI16:
    case AtomicKind::kI32:
    case AtomicKind::kI64:
      return false;
    default:
      throw "type is non-integral";
  }
}

extern int8_t SizeOf(AtomicKind type) {
  switch (type) {
    case AtomicKind::kBool:
    case AtomicKind::kI8:
    case AtomicKind::kU8:
      return 8;
    case AtomicKind::kI16:
    case AtomicKind::kU16:
      return 16;
    case AtomicKind::kI32:
    case AtomicKind::kU32:
      return 32;
    case AtomicKind::kI64:
    case AtomicKind::kU64:
    case AtomicKind::kPtr:
    case AtomicKind::kFunc:
      return 64;
    default:
      throw "type has no associated size";
  }
}

AtomicKind ToAtomicTypeKind(std::string type_str) {
  if (type_str == "b") return AtomicKind::kBool;
  if (type_str == "i8") return AtomicKind::kI8;
  if (type_str == "i16") return AtomicKind::kI16;
  if (type_str == "i32") return AtomicKind::kI32;
  if (type_str == "i64") return AtomicKind::kI64;
  if (type_str == "u8") return AtomicKind::kU8;
  if (type_str == "u16") return AtomicKind::kU16;
  if (type_str == "u32") return AtomicKind::kU32;
  if (type_str == "u64") return AtomicKind::kU64;
  if (type_str == "ptr") return AtomicKind::kPtr;
  if (type_str == "func") return AtomicKind::kFunc;
  throw "unknown type string";
}

std::string ToString(AtomicKind type) {
  switch (type) {
    case AtomicKind::kBool:
      return "b";
    case AtomicKind::kI8:
      return "i8";
    case AtomicKind::kI16:
      return "i16";
    case AtomicKind::kI32:
      return "i32";
    case AtomicKind::kI64:
      return "i64";
    case AtomicKind::kU8:
      return "u8";
    case AtomicKind::kU16:
      return "u16";
    case AtomicKind::kU32:
      return "u32";
    case AtomicKind::kU64:
      return "u64";
    case AtomicKind::kPtr:
      return "ptr";
    case AtomicKind::kFunc:
      return "func";
  }
}

TypeTable::TypeTable() {
  for (AtomicKind kind : std::array<AtomicKind, 11>{
           AtomicKind::kBool, AtomicKind::kI8, AtomicKind::kI16, AtomicKind::kI32, AtomicKind::kI64,
           AtomicKind::kU8, AtomicKind::kU16, AtomicKind::kU32, AtomicKind::kU64, AtomicKind::kPtr,
           AtomicKind::kFunc}) {
    atomic_types_.push_back(std::make_unique<Atomic>(kind));
  }
}

Type* TypeTable::AddType(std::unique_ptr<Type> type) {
  Type* type_ptr = type.get();
  types_.push_back(std::move(type));
  return type_ptr;
}

}  // namespace ir
