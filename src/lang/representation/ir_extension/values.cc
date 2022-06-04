//
//  values.c
//  Katara
//
//  Created by Arne Philipeit on 5/13/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "values.h"

namespace lang {
namespace ir_ext {

void StringConstant::WriteRefString(std::ostream& os) const {
  if (value_.length() > 3) {
    os << "\"...\"";
  } else {
    os << "\"" << value_ << "\"";
  }
}

bool StringConstant::operator==(const Value& that) const {
  if (that.kind() != ir::Value::Kind::kConstant) return false;
  if (that.type()->type_kind() != ir::TypeKind::kLangString) return false;
  return value() == static_cast<const StringConstant&>(that).value();
}

}  // namespace ir_ext
}  // namespace lang
