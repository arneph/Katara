//
//  values.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "values.h"

#include <sstream>

namespace ir {

std::string Constant::ToString() const {
  switch (atomic_type()->kind()) {
    case AtomicTypeKind::kBool:
      return (value_) ? "#t" : "#f";
    case AtomicTypeKind::kI8:
    case AtomicTypeKind::kI16:
    case AtomicTypeKind::kI32:
    case AtomicTypeKind::kI64:
      return "#" + std::to_string(value_);
    case AtomicTypeKind::kU8:
    case AtomicTypeKind::kU16:
    case AtomicTypeKind::kU32:
    case AtomicTypeKind::kU64:
      return "#" + std::to_string(uint64_t(value_));
    case AtomicTypeKind::kPtr: {
      std::stringstream sstream;
      sstream << std::hex << value_;
      return "0x" + sstream.str();
    }
    case AtomicTypeKind::kFunc:
      return "@" + std::to_string(value_);
    default:
      throw "unexpected const type";
  }
}

std::string Constant::ToStringWithType() const {
  if (atomic_type()->kind() == AtomicTypeKind::kBool ||
      atomic_type()->kind() == AtomicTypeKind::kPtr ||
      atomic_type()->kind() == AtomicTypeKind::kFunc) {
    return ToString();
  }

  return ToString() + ":" + type()->ToString();
}

}  // namespace ir
