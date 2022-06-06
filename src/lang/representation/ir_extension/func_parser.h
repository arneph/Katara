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
namespace ir_ext {

class FuncParser : public ir_serialization::FuncParser {
 public:
  FuncParser(ir_serialization::Scanner& scanner, ir::Program* program)
      : ir_serialization::FuncParser(scanner, program) {}

 private:
  std::unique_ptr<ir::Instr> ParseInstrWithResults(
      std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name) override;
  std::unique_ptr<PanicInstr> ParsePanicInstr();
  std::unique_ptr<MakeSharedPointerInstr> ParseMakeSharedInstr(
      std::shared_ptr<ir::Computed> result);
  std::unique_ptr<CopySharedPointerInstr> ParseCopySharedInstr(
      std::shared_ptr<ir::Computed> result);
  std::unique_ptr<DeleteSharedPointerInstr> ParseDeleteSharedInstr();
  std::unique_ptr<MakeUniquePointerInstr> ParseMakeUniqueInstr(
      std::shared_ptr<ir::Computed> result);
  std::unique_ptr<DeleteUniquePointerInstr> ParseDeleteUniqueInstr();
  std::unique_ptr<StringIndexInstr> ParseStringIndexInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<StringConcatInstr> ParseStringConcatInstr(std::shared_ptr<ir::Computed> result);

  std::shared_ptr<ir::Constant> ParseConstant(const ir::Type* expected_type) override;
  std::shared_ptr<StringConstant> ParseStringConstant();

  const ir::Type* ParseType() override;
  const SharedPointer* ParseSharedPointer();
  const UniquePointer* ParseUniquePointer();
  const Array* ParseArray();
  const Struct* ParseStruct();
  void ParseStructField(StructBuilder& builder);
  const Interface* ParseInterface();
  void ParseInterfaceMethod(InterfaceBuilder& builder);
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_func_parser_h */
