//
//  constant_handler.h
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_constant_handler_h
#define lang_type_checker_constant_handler_h

#include <vector>

#include "lang/processors/issues/issues.h"
#include "lang/processors/type_checker/base_handler.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/types/info.h"
#include "lang/representation/types/info_builder.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/types.h"

namespace lang {
namespace type_checker {

class ConstantHandler final : public BaseHandler {
 public:
  bool ProcessConstant(types::Constant* constant, types::Type* type, ast::Expr* value,
                       int64_t iota);

 private:
  ConstantHandler(class TypeResolver& type_resolver, types::InfoBuilder& info_builder,
                  issues::IssueTracker& issues)
      : BaseHandler(type_resolver, info_builder, issues) {}

  constants::Value ConvertUntypedInt(constants::Value value, types::Basic::Kind kind);

  friend class TypeResolver;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_constant_handler_h */
