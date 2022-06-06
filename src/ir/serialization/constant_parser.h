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

#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"

namespace ir_serialization {

class ConstantParser {
 public:
  ConstantParser(Scanner& scanner, TypeParser* type_parser, ir::Program* program)
      : scanner_(scanner), type_parser_(type_parser), program_(program) {}
  virtual ~ConstantParser() = default;

  virtual std::shared_ptr<ir::Constant> ParseConstant(const ir::Type* expected_type);

 protected:
  Scanner& scanner() { return scanner_; }
  TypeParser* type_parser() { return type_parser_; }
  ir::Program* program() { return program_; }

 private:
  std::shared_ptr<ir::PointerConstant> ParsePointerConstant();
  std::shared_ptr<ir::FuncConstant> ParseFuncConstant();
  std::shared_ptr<ir::Constant> ParseBoolOrIntConstant(const ir::Type* expected_type);

  Scanner& scanner_;
  TypeParser* type_parser_;
  ir::Program* program_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_constant_parser_h */
