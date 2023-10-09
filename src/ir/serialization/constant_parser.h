//
//  constant_parser.h
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_constant_parser_h
#define ir_serialization_constant_parser_h

#include <memory>

#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"

namespace ir_serialization {

class ConstantParser {
 public:
  ConstantParser(Scanner& scanner, ir_issues::IssueTracker& issue_tracker, TypeParser* type_parser,
                 ir::Program* program, int64_t func_num_offset)
      : scanner_(scanner),
        issue_tracker_(issue_tracker),
        type_parser_(type_parser),
        program_(program),
        func_num_offset_(func_num_offset) {}
  virtual ~ConstantParser() = default;

  struct ConstantParseResult {
    std::shared_ptr<ir::Constant> constant;
    common::positions::range_t range;
  };
  static ConstantParseResult NoConstantParseResult();
  virtual ConstantParseResult ParseConstant(const ir::Type* expected_type);

 protected:
  Scanner& scanner() { return scanner_; }
  ir_issues::IssueTracker& issue_tracker() { return issue_tracker_; }
  TypeParser* type_parser() { return type_parser_; }
  ir::Program* program() { return program_; }

 private:
  ConstantParseResult ParsePointerConstant();
  ConstantParseResult ParseFuncConstant();
  ConstantParseResult ParseBoolOrIntConstant(const ir::Type* expected_type);

  Scanner& scanner_;
  ir_issues::IssueTracker& issue_tracker_;
  TypeParser* type_parser_;
  ir::Program* program_;
  int64_t func_num_offset_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_constant_parser_h */
