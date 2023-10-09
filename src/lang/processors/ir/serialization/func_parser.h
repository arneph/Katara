//
//  func_parser.h
//  Katara
//
//  Created by Arne Philipeit on 6/4/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_serialization_func_parser_h
#define lang_ir_serialization_func_parser_h

#include <memory>
#include <string>
#include <vector>

#include "src/ir/issues/issues.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/constant_parser.h"
#include "src/ir/serialization/func_parser.h"
#include "src/ir/serialization/positions.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"

namespace lang {
namespace ir_serialization {

class FuncParser : public ::ir_serialization::FuncParser {
 public:
  FuncParser(::ir_serialization::Scanner& scanner, ir_issues::IssueTracker& issue_tracker,
             ::ir_serialization::TypeParser* type_parser,
             ::ir_serialization::ConstantParser* constant_parser, ir::Program* program,
             ::ir_serialization::ProgramPositions& program_positions, int64_t func_num_offset)
      : ::ir_serialization::FuncParser(scanner, issue_tracker, type_parser, constant_parser,
                                       program, program_positions, func_num_offset) {}

 private:
  InstrParseResult ParseInstrWithResults(std::vector<std::shared_ptr<ir::Computed>> results,
                                         std::string instr_name) override;
  InstrParseResult ParsePanicInstr();
  InstrParseResult ParseMakeSharedInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseCopySharedInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseDeleteSharedInstr();
  InstrParseResult ParseMakeUniqueInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseDeleteUniqueInstr();
  InstrParseResult ParseStringIndexInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseStringConcatInstr(std::shared_ptr<ir::Computed> result);
};

}  // namespace ir_serialization
}  // namespace lang

#endif /* lang_ir_serialization_func_parser_h */
