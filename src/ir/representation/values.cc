//
//  values.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "values.h"

#include <sstream>

namespace ir {

std::shared_ptr<BoolConstant> False() {
  static auto kFalse = std::shared_ptr<ir::BoolConstant>(new ir::BoolConstant(false));
  return kFalse;
}

std::shared_ptr<BoolConstant> True() {
  static auto kTrue = std::shared_ptr<ir::BoolConstant>(new ir::BoolConstant(true));
  return kTrue;
}

std::shared_ptr<BoolConstant> ToBoolConstant(bool value) { return value ? True() : False(); }

std::shared_ptr<IntConstant> I64Zero() {
  static auto kI64Zero = std::make_shared<ir::IntConstant>(common::Int(int64_t{0}));
  return kI64Zero;
}

std::shared_ptr<IntConstant> I64One() {
  static auto kI64One = std::make_shared<ir::IntConstant>(common::Int(int64_t{1}));
  return kI64One;
}

std::shared_ptr<PointerConstant> NilPointer() {
  static auto kNilPointer = std::make_shared<ir::PointerConstant>(0);
  return kNilPointer;
}

std::shared_ptr<FuncConstant> NilFunc() {
  static auto kNilFunc = std::make_shared<ir::FuncConstant>(kNoFuncNum);
  return kNilFunc;
}

std::string PointerConstant::ToString() const {
  std::stringstream sstream;
  sstream << "0x" << std::hex << value_;
  return sstream.str();
}

}  // namespace ir
