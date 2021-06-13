//
//  selection.h
//  Katara
//
//  Created by Arne Philipeit on 12/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_selection_h
#define lang_types_selection_h

#include "src/lang/representation/types/objects.h"
#include "src/lang/representation/types/types.h"

namespace lang {
namespace types {

class Selection {
 public:
  enum class Kind {
    kFieldVal,
    kMethodVal,
    kMethodExpr,
  };

  Selection(Kind kind, Type* receiver_type, Type* type, Object* object)
      : kind_(kind), receiver_type_(receiver_type), type_(type), object_(object) {}

  Kind kind() const { return kind_; }
  Type* receiver_type() const { return receiver_type_; }
  Type* type() const { return type_; }
  Object* object() const { return object_; }

 private:
  Kind kind_;
  Type* receiver_type_;
  Type* type_;
  Object* object_;
};

}  // namespace types
}  // namespace lang

#endif /* lang_types_selection_h */
