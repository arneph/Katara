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

Type* TypeTable::AddType(std::unique_ptr<Type> type) {
  Type* type_ptr = type.get();
  types_.push_back(std::move(type));
  return type_ptr;
}

}  // namespace ir
