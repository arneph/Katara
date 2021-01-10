//
//  expr_info.cc
//  Katara
//
//  Created by Arne Philipeit on 1/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "expr_info.h"

namespace lang {
namespace types {

bool ExprInfo::is_type() const {
  switch (kind_) {
    case Kind::kType:
      return true;
    default:
      return false;
  }
}

bool ExprInfo::is_value() const {
  switch (kind_) {
    case Kind::kConstant:
    case Kind::kVariable:
    case Kind::kValue:
    case Kind::kValueOk:
      return true;
    default:
      return false;
  }
}

bool ExprInfo::is_addressable() const {
  switch (kind_) {
    case Kind::kVariable:
      return true;
    default:
      return false;
  }
}

}  // namespace types
}  // namespace lang