//
//  object.h
//  Katara
//
//  Created by Arne Philipeit on 2/20/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_object_h
#define ir_object_h

#include <ostream>
#include <string>

namespace ir {

class Object {
 public:
  enum class Kind {
    kType,
    kValue,
    kInstr,
    kBlock,
    kFunc,
    kProgram,
  };

  constexpr virtual ~Object() {}

  constexpr virtual Kind object_kind() const = 0;
  virtual void WriteRefString(std::ostream& os) const = 0;
  std::string RefString() const;
};

}  // namespace ir

#endif /* ir_object_h */
