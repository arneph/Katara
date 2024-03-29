//
//  func_builder.h
//  Katara
//
//  Created by Arne Philipeit on 8/1/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_func_builder_h
#define ir_func_builder_h

#include <memory>

#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace ir_builder {

class FuncBuilder {
 public:
  static FuncBuilder ForNewFuncInProgram(ir::Program* program);

  ir::Func* func() const { return func_; }
  ir::func_num_t func_number() const { return func_->number(); }

  void SetName(std::string name) { func_->set_name(name); }

  std::shared_ptr<ir::Computed> AddArg(const ir::Type* type);
  void AddResultType(const ir::Type* type);

  class BlockBuilder AddEntryBlock();
  class BlockBuilder AddBlock();

  std::shared_ptr<ir::Computed> MakeComputed(const ir::Type* type);

 private:
  FuncBuilder(ir::Program* program, ir::Func* func) : program_(program), func_(func) {}

  ir::Program* program_;
  ir::Func* func_;

  friend class BlockBuilder;
};

}  // namespace ir_builder

#endif /* ir_func_builder_h */
