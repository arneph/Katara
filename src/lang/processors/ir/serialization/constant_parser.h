//
//  constant_parser.h
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_serialization_constant_parser_h
#define lang_ir_serialization_constant_parser_h

#include <memory>

#include "src/ir/issues/issues.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/constant_parser.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"

namespace lang {
namespace ir_serialization {

class ConstantParser : public ::ir_serialization::ConstantParser {
 public:
  ConstantParser(::ir_serialization::Scanner& scanner, ir_issues::IssueTracker& issue_tracker,
                 ::ir_serialization::TypeParser* type_parser, ir::Program* program,
                 int64_t func_num_offset)
      : ::ir_serialization::ConstantParser(scanner, issue_tracker, type_parser, program,
                                           func_num_offset) {}

 private:
  using ::ir_serialization::ConstantParser::ConstantParseResult;

  ConstantParseResult ParseConstant(const ir::Type* expected_type) override;
  ConstantParseResult ParseStringConstant();
};

}  // namespace ir_serialization
}  // namespace lang

#endif /* lang_ir_serialization_constant_parser_h */
