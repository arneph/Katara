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

bool Type::is_lang_type() const {
  TypeKind kind = type_kind();
  return TypeKind::kLangStart <= kind && kind <= TypeKind::kLangEnd;
}

bool IsIntegral(AtomicTypeKind type) {
  switch (type) {
    case AtomicTypeKind::kBool:
    case AtomicTypeKind::kI8:
    case AtomicTypeKind::kI16:
    case AtomicTypeKind::kI32:
    case AtomicTypeKind::kI64:
    case AtomicTypeKind::kU8:
    case AtomicTypeKind::kU16:
    case AtomicTypeKind::kU32:
    case AtomicTypeKind::kU64:
    case AtomicTypeKind::kPtr:
      return true;
    default:
      return false;
  }
}

bool IsUnsigned(AtomicTypeKind type) {
  switch (type) {
    case AtomicTypeKind::kBool:
    case AtomicTypeKind::kU8:
    case AtomicTypeKind::kU16:
    case AtomicTypeKind::kU32:
    case AtomicTypeKind::kU64:
    case AtomicTypeKind::kPtr:
      return true;
    case AtomicTypeKind::kI8:
    case AtomicTypeKind::kI16:
    case AtomicTypeKind::kI32:
    case AtomicTypeKind::kI64:
      return false;
    default:
      throw "type is non-integral";
  }
}

extern int8_t SizeOf(AtomicTypeKind type) {
  switch (type) {
    case AtomicTypeKind::kBool:
    case AtomicTypeKind::kI8:
    case AtomicTypeKind::kU8:
      return 8;
    case AtomicTypeKind::kI16:
    case AtomicTypeKind::kU16:
      return 16;
    case AtomicTypeKind::kI32:
    case AtomicTypeKind::kU32:
      return 32;
    case AtomicTypeKind::kI64:
    case AtomicTypeKind::kU64:
    case AtomicTypeKind::kPtr:
    case AtomicTypeKind::kFunc:
      return 64;
    default:
      throw "type has no associated size";
  }
}

AtomicTypeKind ToAtomicTypeKind(std::string type_str) {
  if (type_str == "b") return AtomicTypeKind::kBool;
  if (type_str == "i8") return AtomicTypeKind::kI8;
  if (type_str == "i16") return AtomicTypeKind::kI16;
  if (type_str == "i32") return AtomicTypeKind::kI32;
  if (type_str == "i64") return AtomicTypeKind::kI64;
  if (type_str == "u8") return AtomicTypeKind::kU8;
  if (type_str == "u16") return AtomicTypeKind::kU16;
  if (type_str == "u32") return AtomicTypeKind::kU32;
  if (type_str == "u64") return AtomicTypeKind::kU64;
  if (type_str == "ptr") return AtomicTypeKind::kPtr;
  if (type_str == "func") return AtomicTypeKind::kFunc;
  throw "unknown type string";
}

std::string ToString(AtomicTypeKind type) {
  switch (type) {
    case AtomicTypeKind::kBool:
      return "b";
    case AtomicTypeKind::kI8:
      return "i8";
    case AtomicTypeKind::kI16:
      return "i16";
    case AtomicTypeKind::kI32:
      return "i32";
    case AtomicTypeKind::kI64:
      return "i64";
    case AtomicTypeKind::kU8:
      return "u8";
    case AtomicTypeKind::kU16:
      return "u16";
    case AtomicTypeKind::kU32:
      return "u32";
    case AtomicTypeKind::kU64:
      return "u64";
    case AtomicTypeKind::kPtr:
      return "ptr";
    case AtomicTypeKind::kFunc:
      return "func";
  }
}

AtomicTypeTable::AtomicTypeTable() {
  for (AtomicTypeKind kind : std::array<AtomicTypeKind, 11>{
           AtomicTypeKind::kBool, AtomicTypeKind::kI8, AtomicTypeKind::kI16, AtomicTypeKind::kI32,
           AtomicTypeKind::kI64, AtomicTypeKind::kU8, AtomicTypeKind::kU16, AtomicTypeKind::kU32,
           AtomicTypeKind::kU64, AtomicTypeKind::kPtr, AtomicTypeKind::kFunc}) {
    types_.push_back(std::make_unique<AtomicType>(kind));
  }
}

}  // namespace ir
