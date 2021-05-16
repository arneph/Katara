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
  std::string ToString() const override { return "str"; }
};

class RefCountPointer : public ir::Type {
 public:
  RefCountPointer() {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangRefCountPointer; }
  std::string ToString() const override { return "rcptr"; }
};

class Struct : public ir::Type {
 public:
  Struct() {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangStruct; }
  std::string ToString() const override { return "struct"; }

 private:
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_types_h */
