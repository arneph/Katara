//
//  type_parser.cc
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "type_parser.h"

#include "src/common/logging/logging.h"
#include "src/common/positions/positions.h"

namespace ir_serialization {

using ::common::positions::kNoRange;
using ::common::positions::range_t;

// Types ::= Type (',' Type)?
TypeParser::TypesParseResult TypeParser::ParseTypes() {
  const auto& [first_type, first_range] = ParseType();
  std::vector<const ir::Type*> types{first_type};
  std::vector<range_t> type_ranges{first_range};
  while (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    const auto& [type, range] = ParseType();
    types.push_back(type);
    type_ranges.push_back(range);
  }
  return TypesParseResult{
      .types = types,
      .type_ranges = type_ranges,
      .range =
          range_t{
              .start = type_ranges.front().start,
              .end = type_ranges.back().end,
          },
  };
}

// Type ::= Identifier
TypeParser::TypeParseResult TypeParser::ParseType() {
  if (scanner().token() != Scanner::kIdentifier) {
    return TypeParseResult{
        .type = nullptr,
        .range = kNoRange,
    };
  }
  range_t name_range = scanner().token_range();
  std::string name = scanner().ConsumeIdentifier().value();

  if (name == "b") {
    return TypeParseResult{
        .type = ir::bool_type(),
        .range = name_range,
    };
  } else if (auto int_type = common::atomics::ToIntType(name); int_type) {
    return TypeParseResult{
        .type = ir::IntTypeFor(int_type.value()),
        .range = name_range,
    };
  } else if (name == "ptr") {
    return TypeParseResult{
        .type = ir::pointer_type(),
        .range = name_range,
    };
  } else if (name == "func") {
    return TypeParseResult{
        .type = ir::func_type(),
        .range = name_range,
    };
  } else {
    issue_tracker().Add(ir_issues::IssueKind::kUnknownTypeName, name_range, "unknown type name");
    return TypeParseResult{
        .type = nullptr,
        .range = name_range,
    };
  }
}

}  // namespace ir_serialization
