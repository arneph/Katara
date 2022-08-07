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

#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/constant_parser.h"
#include "src/ir/serialization/func_parser.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"

namespace lang {
namespace ir_serialization {

class FuncParser : public ::ir_serialization::FuncParser {
 public:
  FuncParser(::ir_serialization::Scanner& scanner, ::ir_serialization::TypeParser* type_parser,
             ::ir_serialization::ConstantParser* constant_parser, ir::Program* program,
             int64_t func_num_offset)
      : ::ir_serialization::FuncParser(scanner, type_parser, constant_parser, program,
                                       func_num_offset) {}

 private:
  std::unique_ptr<ir::Instr> ParseInstrWithResults(
      std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name) override;
  std::unique_ptr<ir_ext::PanicInstr> ParsePanicInstr();
  std::unique_ptr<ir_ext::MakeSharedPointerInstr> ParseMakeSharedInstr(
      std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir_ext::CopySharedPointerInstr> ParseCopySharedInstr(
      std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir_ext::DeleteSharedPointerInstr> ParseDeleteSharedInstr();
  std::unique_ptr<ir_ext::MakeUniquePointerInstr> ParseMakeUniqueInstr(
      std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir_ext::DeleteUniquePointerInstr> ParseDeleteUniqueInstr();
  std::unique_ptr<ir_ext::StringIndexInstr> ParseStringIndexInstr(
      std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir_ext::StringConcatInstr> ParseStringConcatInstr(
      std::shared_ptr<ir::Computed> result);
};

}  // namespace ir_serialization
}  // namespace lang

#endif /* lang_ir_serialization_func_parser_h */
