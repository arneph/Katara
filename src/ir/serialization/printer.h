//
//  printer.h
//  Katara
//
//  Created by Arne Philipeit on 9/17/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_printer_h
#define ir_serialization_printer_h

#include <functional>
#include <sstream>
#include <string>
#include <string_view>

#include "src/common/positions/positions.h"

namespace ir_serialization {

class Printer {
 public:
  static Printer FromPostion(common::positions::pos_t pos);

  std::string contents() const { return buffer_.str(); }

  common::positions::range_t Write(std::string_view s);
  common::positions::range_t WriteWithFunc(std::function<void()> f);

 private:
  Printer(common::positions::pos_t pos) : pos_(pos) {}

  common::positions::pos_t pos_;
  std::stringstream buffer_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_printer_h */
