//
//  values.h
//  Katara
//
//  Created by Arne Philipeit on 6/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include <memory>

#ifndef ir_interpreter_values_h
#define ir_interpreter_values_h

namespace ir_interpreter {

class Value {
 public:
  Value(int64_t value) : value_(value) {}

  int64_t value() const { return value_; }

 private:
  int64_t value_;
};

}  // namespace ir_interpreter

#endif /* ir_interpreter_values_h */
