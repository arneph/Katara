//
//  func_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 8/1/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "func_builder.h"

#include "src/ir/builder/block_builder.h"

namespace ir_builder {

FuncBuilder FuncBuilder::ForNewFuncInProgram(ir::Program* program) {
  ir::Func* func = program->AddFunc();
  return FuncBuilder(program, func);
}

std::shared_ptr<ir::Computed> FuncBuilder::MakeComputed(const ir::Type* type) {
  return std::make_shared<ir::Computed>(type, func_->next_computed_number());
}

std::shared_ptr<ir::Computed> FuncBuilder::AddArg(const ir::Type* type) {
  std::shared_ptr<ir::Computed> arg = MakeComputed(type);
  func_->args().push_back(arg);
  return arg;
}

void FuncBuilder::AddResultType(const ir::Type* type) { func_->result_types().push_back(type); }

}  // namespace ir_builder
