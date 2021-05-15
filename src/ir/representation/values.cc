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
    case AtomicKind::kBool:
      return (value_) ? "#t" : "#f";
    case AtomicKind::kI8:
    case AtomicKind::kI16:
    case AtomicKind::kI32:
    case AtomicKind::kI64:
      return "#" + std::to_string(value_);
    case AtomicKind::kU8:
    case AtomicKind::kU16:
    case AtomicKind::kU32:
    case AtomicKind::kU64:
      return "#" + std::to_string(uint64_t(value_));
    case AtomicKind::kPtr: {
      std::stringstream sstream;
      sstream << std::hex << value_;
      return "0x" + sstream.str();
    }
    case AtomicKind::kFunc:
      return "@" + std::to_string(value_);
    default:
      throw "unexpected const type";
  }
}

std::string Constant::ToStringWithType() const {
  if (atomic_type()->kind() == AtomicKind::kBool || atomic_type()->kind() == AtomicKind::kPtr ||
      atomic_type()->kind() == AtomicKind::kFunc) {
    return ToString();
  }

  return ToString() + ":" + type()->ToString();
}

}  // namespace ir
