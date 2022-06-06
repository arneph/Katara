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
  ConstantParser(::ir_serialization::Scanner& scanner, ::ir_serialization::TypeParser* type_parser,
                 ir::Program* program)
      : ::ir_serialization::ConstantParser(scanner, type_parser, program) {}

 private:
  std::shared_ptr<ir::Constant> ParseConstant(const ir::Type* expected_type) override;
  std::shared_ptr<ir_ext::StringConstant> ParseStringConstant();
};

}  // namespace ir_serialization
}  // namespace lang

#endif /* lang_ir_serialization_constant_parser_h */
