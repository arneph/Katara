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
using ::common::positions::range_t;
using ConstantParseResult = ::ir_serialization::ConstantParser::ConstantParseResult;

ConstantParseResult ConstantParser::ParseConstant(const ir::Type* expected_type) {
  if (scanner().token() == ::ir_serialization::Scanner::kString) {
    return ParseStringConstant();
  } else {
    return ::ir_serialization::ConstantParser::ParseConstant(expected_type);
  }
}

ConstantParseResult ConstantParser::ParseStringConstant() {
  if (scanner().token() != ::ir_serialization::Scanner::kString) {
    fail("expected string constant");
  }
  range_t str_range = scanner().token_range();
  std::string str = scanner().token_string();
  scanner().Next();

  return ConstantParseResult{
      .constant = std::make_shared<ir_ext::StringConstant>(str),
      .range = str_range,
  };
}

}  // namespace ir_serialization
}  // namespace lang
