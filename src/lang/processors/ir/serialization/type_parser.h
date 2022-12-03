//
//  type_parser.h
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_serialization_type_parser_h
#define lang_ir_serialization_type_parser_h

#include "src/ir/issues/issues.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"
#include "src/lang/representation/ir_extension/types.h"

namespace lang {
namespace ir_serialization {

class TypeParser : public ::ir_serialization::TypeParser {
 public:
  TypeParser(::ir_serialization::Scanner& scanner, ir_issues::IssueTracker& issue_tracker,
             ir::Program* program)
      : ::ir_serialization::TypeParser(scanner, issue_tracker, program) {}

 private:
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

#endif /* lang_ir_serialization_type_parser_h */
