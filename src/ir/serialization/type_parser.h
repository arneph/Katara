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

#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/serialization/scanner.h"

namespace ir_serialization {

class TypeParser {
 public:
  TypeParser(Scanner& scanner, ir::Program* program) : scanner_(scanner), program_(program) {}
  virtual ~TypeParser() = default;

  std::vector<const ir::Type*> ParseTypes();
  virtual const ir::Type* ParseType();

 protected:
  Scanner& scanner() { return scanner_; }
  ir::Program* program() { return program_; }

 private:
  Scanner& scanner_;
  ir::Program* program_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_type_parser_h */
