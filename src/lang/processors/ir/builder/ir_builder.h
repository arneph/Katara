//
//  ir_builder.h
//  Katara
//
//  Created by Arne Philipeit on 4/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_builder_h
#define lang_ir_builder_h

#include <memory>
#include <unordered_map>

#include "src/common/atomics/atomics.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/lang/processors/ir/builder/context.h"
#include "src/lang/processors/ir/builder/expr_builder.h"
#include "src/lang/processors/ir/builder/stmt_builder.h"
#include "src/lang/processors/ir/builder/type_builder.h"
#include "src/lang/processors/ir/builder/value_builder.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/representation/types/expr_info.h"
#include "src/lang/representation/types/info.h"
#include "src/lang/representation/types/initializer.h"
#include "src/lang/representation/types/objects.h"
#include "src/lang/representation/types/package.h"
#include "src/lang/representation/types/scope.h"
#include "src/lang/representation/types/selection.h"
#include "src/lang/representation/types/types.h"
#include "src/lang/representation/types/types_util.h"
#include "src/lang/runtime/runtime.h"

namespace lang {
namespace ir_builder {

struct ProgramWithRuntime {
  std::unique_ptr<ir::Program> program;
  runtime::RuntimeFuncs runtime;
};

class IRBuilder {
 public:
  static ProgramWithRuntime TranslateProgram(packages::Package* main_package,
                                             types::Info* type_info);

 private:
  IRBuilder(types::Info* type_info, ir::Program* program, runtime::RuntimeFuncs& runtime);

  void PrepareDeclsInFile(ast::File* file);
  void PrepareFuncDecl(ast::FuncDecl* func_decl);

  void BuildDeclsInFile(ast::File* file);
  void BuildFuncDecl(ast::FuncDecl* func_decl);
  void BuildFuncParameters(types::Tuple* parameters, ASTContext& ast_ctx, IRContext& ir_ctx);
  void BuildFuncResults(types::Tuple* results, ASTContext& ast_ctx, IRContext& ir_ctx);

  types::Info* type_info_;
  TypeBuilder type_builder_;
  ValueBuilder value_builder_;
  ExprBuilder expr_builder_;
  StmtBuilder stmt_builder_;
  ir::Program* program_;
  runtime::RuntimeFuncs& runtime_;
  std::unordered_map<types::Func*, ir::Func*> funcs_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_h */
