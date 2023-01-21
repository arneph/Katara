//
//  type_parser.cc
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "type_parser.h"

#include "src/common/logging/logging.h"

namespace ir_serialization {

using ::common::positions::pos_t;

// Types ::= Type (',' Type)?
std::vector<const ir::Type*> TypeParser::ParseTypes() {
  std::vector<const ir::Type*> types{ParseType()};
  while (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    types.push_back(ParseType());
  }
  return types;
}

// Type ::= Identifier
const ir::Type* TypeParser::ParseType() {
  if (scanner().token() != Scanner::kIdentifier) {
    return nullptr;
  }
  pos_t name_pos = scanner().token_start();
  std::string name = scanner().ConsumeIdentifier().value();

  if (name == "b") {
    return ir::bool_type();
  } else if (auto int_type = common::atomics::ToIntType(name); int_type) {
    return ir::IntTypeFor(int_type.value());
  } else if (name == "ptr") {
    return ir::pointer_type();
  } else if (name == "func") {
    return ir::func_type();
  } else {
    issue_tracker().Add(ir_issues::IssueKind::kUnknownTypeName, name_pos, "unknown type name");
    return nullptr;
  }
}

}  // namespace ir_serialization
