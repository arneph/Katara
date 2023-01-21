//
//  constant_parser.cc
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "constant_parser.h"

#include <string>

#include "src/common/logging/logging.h"

namespace lang {
namespace ir_serialization {

using ::common::logging::fail;

std::shared_ptr<ir::Constant> ConstantParser::ParseConstant(const ir::Type* expected_type) {
  if (scanner().token() == ::ir_serialization::Scanner::kString) {
    return ParseStringConstant();
  } else {
    return ::ir_serialization::ConstantParser::ParseConstant(expected_type);
  }
}

std::shared_ptr<ir_ext::StringConstant> ConstantParser::ParseStringConstant() {
  if (scanner().token() != ::ir_serialization::Scanner::kString) {
    fail("expected string constant");
  }
  std::string str = scanner().token_string();
  scanner().Next();

  return std::make_shared<ir_ext::StringConstant>(str);
}

}  // namespace ir_serialization
}  // namespace lang
