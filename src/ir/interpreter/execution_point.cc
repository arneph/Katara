//
//  execution_point.cc
//  Katara
//
//  Created by Arne Philipeit on 8/7/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "execution_point.h"

#include <sstream>

#include "src/common/logging/logging.h"

namespace ir_interpreter {

using ::common::logging::fail;

ExecutionPoint ExecutionPoint::AtFuncEntry(ir::Func* func) {
  return ExecutionPoint(/*previous_block=*/nullptr,
                        /*current_block=*/func->entry_block(),
                        /*next_instr=*/0,
                        /*results=*/{});
}

ir::Instr* ExecutionPoint::next_instr() const {
  if (next_instr_index_ < current_block_->instrs().size()) {
    return current_block_->instrs().at(next_instr_index_).get();
  } else {
    return nullptr;
  }
}

void ExecutionPoint::AdvanceToNextInstr() { next_instr_index_++; }

void ExecutionPoint::AdvanceToNextBlock(ir::Block* next_block) {
  previous_block_ = current_block_;
  current_block_ = next_block;
  next_instr_index_ = 0;
}

void ExecutionPoint::AdvanceToFuncExit(std::vector<std::shared_ptr<ir::Constant>> results) {
  next_instr_index_ = current_block_->instrs().size();
  results_ = results;
}

const std::vector<std::shared_ptr<ir::Constant>>& ExecutionPoint::results() const {
  if (!is_at_func_exit()) {
    fail("results are not defined at current execution point");
  }
  return results_;
}

}  // namespace ir_interpreter
