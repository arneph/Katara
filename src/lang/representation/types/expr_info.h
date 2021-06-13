//
//  expr_info.h
//  Katara
//
//  Created by Arne Philipeit on 1/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_expr_info_h
#define lang_types_expr_info_h

#include <optional>

#include "src/lang/representation/constants/constants.h"
#include "src/lang/representation/types/types.h"

namespace lang {
namespace types {

class ExprInfo {
 public:
  enum class Kind {
    kInvalid,
    kNoValue,
    kBuiltin,
    kType,
    kConstant,
    kVariable,
    kValue,
    kValueOk,
  };

  ExprInfo(Kind kind, Type* type, std::optional<constants::Value> constant_value = std::nullopt);

  bool is_type() const;
  bool is_value() const;
  bool is_constant() const;
  bool is_addressable() const;
  Kind kind() const { return kind_; }
  Type* type() const { return type_; }
  constants::Value constant_value() const { return constant_value_.value(); }

 private:
  Kind kind_;
  Type* type_;
  std::optional<constants::Value> constant_value_;
};

}  // namespace types
}  // namespace lang

#endif /* lang_types_expr_info_h */
