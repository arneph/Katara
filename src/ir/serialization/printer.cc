//
//  printer.cc
//  Katara
//
//  Created by Arne Philipeit on 9/17/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "printer.h"

#include <functional>
#include <string_view>

#include "src/common/positions/positions.h"

namespace ir_serialization {

using common::positions::pos_t;
using common::positions::range_t;

Printer Printer::FromPostion(pos_t pos) { return Printer(pos); }

range_t Printer::Write(std::string_view s) {
  pos_t start = pos_;
  pos_ += s.length();
  buffer_ << s;
  pos_t end = pos_ - 1;
  return range_t{.start = start, .end = end};
}

range_t Printer::WriteWithFunc(std::function<void()> f) {
  pos_t start = pos_;
  f();
  pos_t end = pos_ - 1;
  return range_t{.start = start, .end = end};
}

}  // namespace ir_serialization
