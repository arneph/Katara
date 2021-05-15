//
//  types.h
//  Katara
//
//  Created by Arne Philipeit on 5/14/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_ext_types_h
#define lang_ir_ext_types_h


#include <string>

#include "ir/representation/types.h"

namespace lang {
namespace ir_ext {

class String : public ir::Type {
 public:
  String() {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangString; }
  std::string ToString() const override { return "s"; }
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_types_h */
