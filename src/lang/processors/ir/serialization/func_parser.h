//
//  func_parser.hpp
//  Katara
//
//  Created by Arne Philipeit on 6/4/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_ext_func_parser_h
#define lang_ir_ext_func_parser_h

#include <memory>
#include <string>
#include <vector>

#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/func_parser.h"
#include "src/ir/serialization/scanner.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"

namespace lang {
namespace ir_serialization {

class FuncParser : public ::ir_serialization::FuncParser {
 public:
  FuncParser(::ir_serialization::Scanner& scanner, ir::Program* program)
      : ::ir_serialization::FuncParser(scanner, program) {}

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

  std::shared_ptr<ir::Constant> ParseConstant(const ir::Type* expected_type) override;
  std::shared_ptr<ir_ext::StringConstant> ParseStringConstant();

  const ir::Type* ParseType() override;
  const ir_ext::SharedPointer* ParseSharedPointer();
  const ir_ext::UniquePointer* ParseUniquePointer();
  const ir_ext::Array* ParseArray();
  const ir_ext::Struct* ParseStruct();
  void ParseStructField(ir_ext::StructBuilder& builder);
  const ir_ext::Interface* ParseInterface();
  void ParseInterfaceMethod(ir_ext::InterfaceBuilder& builder);
};

}  // namespace ir_serialization
}  // namespace lang

#endif /* lang_ir_ext_func_parser_h */
