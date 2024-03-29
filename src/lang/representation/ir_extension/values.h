//
//  values.h
//  Katara
//
//  Created by Arne Philipeit on 5/13/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_ext_values_h
#define lang_ir_ext_values_h

#include <string>

#include "src/ir/representation/values.h"
#include "src/lang/representation/ir_extension/types.h"

namespace lang {
namespace ir_ext {

class StringConstant : public ir::Constant {
 public:
  StringConstant(std::string value) : value_(value) {}

  std::string value() const { return value_; }
  const ir::Type* type() const override { return string(); }

  void WriteRefString(std::ostream& os) const override;
  void WriteRefStringWithType(std::ostream& os) const override { WriteRefString(os); }

  bool operator==(const ir::Value& that) const override;

 private:
  std::string value_;
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_values_h */
