//
//  execution_point.cc
//  Katara
//
//  Created by Arne Philipeit on 8/7/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "execution_point.h"

#include <algorithm>

#include "src/common/logging/logging.h"

namespace ir_interpreter {

ExecutionPoint ExecutionPoint::AtFuncEntry(ir::Func* func) {
  return ExecutionPoint(/*previous_block=*/nullptr,
                        /*current_block=*/func->entry_block(),
                        /*next_instr=*/func->entry_block()->instrs().front().get(),
                        /*results=*/{});
}

void ExecutionPoint::AdvanceToNextInstr() {
  auto it = std::find_if(current_block_->instrs().begin(), current_block_->instrs().end(),
                         [this](auto& instr) { return instr.get() == next_instr_; });
  ++it;
  next_instr_ = it->get();
}

void ExecutionPoint::AdvanceToNextBlock(ir::Block* next_block) {
  previous_block_ = current_block_;
  current_block_ = next_block;
  next_instr_ = current_block_->instrs().front().get();
}

void ExecutionPoint::AdvanceToFuncExit(std::vector<std::shared_ptr<ir::Constant>> results) {
  next_instr_ = nullptr;
  results_ = results;
}

const std::vector<std::shared_ptr<ir::Constant>>& ExecutionPoint::results() const {
  if (!is_at_func_exit()) {
    common::fail("results are not defined at current execution point");
  }
  return results_;
}

}  // namespace ir_interpreter
