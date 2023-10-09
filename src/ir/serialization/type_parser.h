//
//  type_parser.h
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_type_parser_h
#define ir_serialization_type_parser_h

#include <vector>

#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/serialization/scanner.h"

namespace ir_serialization {

class TypeParser {
 public:
  TypeParser(Scanner& scanner, ir_issues::IssueTracker& issue_tracker, ir::Program* program)
      : scanner_(scanner), issue_tracker_(issue_tracker), program_(program) {}
  virtual ~TypeParser() = default;

  struct TypesParseResult {
    std::vector<const ir::Type*> types;
    std::vector<common::positions::range_t> type_ranges;
    common::positions::range_t range;
  };
  TypesParseResult ParseTypes();
  struct TypeParseResult {
    const ir::Type* type;
    common::positions::range_t range;
  };
  virtual TypeParseResult ParseType();

 protected:
  Scanner& scanner() { return scanner_; }
  ir_issues::IssueTracker& issue_tracker() { return issue_tracker_; }
  ir::Program* program() { return program_; }

 private:
  Scanner& scanner_;
  ir_issues::IssueTracker& issue_tracker_;
  ir::Program* program_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_type_parser_h */
