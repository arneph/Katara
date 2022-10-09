//
//  execution_point.h
//  Katara
//
//  Created by Arne Philipeit on 8/7/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include <memory>
#include <vector>

#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/values.h"

#ifndef ir_interpreter_execution_point_h
#define ir_interpreter_execution_point_h

namespace ir_interpreter {

class ExecutionPoint {
 public:
  static ExecutionPoint AtFuncEntry(ir::Func* func);

  bool is_at_block_entry() const { return current_block_->instrs().front().get() == next_instr_; }
  bool is_at_func_exit() const { return next_instr_ == nullptr; }
  ir::Block* previous_block() const { return previous_block_; }
  ir::Block* current_block() const { return current_block_; }
  ir::Instr* next_instr() const { return next_instr_; }
  std::size_t next_instr_index() const;
  const std::vector<std::shared_ptr<ir::Constant>>& results() const;

  void AdvanceToNextInstr();
  void AdvanceToNextBlock(ir::Block* next_block);
  void AdvanceToFuncExit(std::vector<std::shared_ptr<ir::Constant>> results);

 private:
  ExecutionPoint(ir::Block* previous_block, ir::Block* current_block, ir::Instr* next_instr,
                 std::vector<std::shared_ptr<ir::Constant>> results)
      : previous_block_(previous_block),
        current_block_(current_block),
        next_instr_(next_instr),
        results_(results) {}

  ir::Block* previous_block_;
  ir::Block* current_block_;
  ir::Instr* next_instr_;
  std::vector<std::shared_ptr<ir::Constant>> results_;
};

}  // namespace ir_interpreter

#endif /* ir_interpreter_execution_point_h */
