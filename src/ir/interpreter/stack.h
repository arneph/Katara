//
//  stack.h
//  Katara
//
//  Created by Arne Philipeit on 8/7/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include <memory>
#include <unordered_map>
#include <vector>

#include "src/ir/interpreter/execution_point.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/values.h"

#ifndef ir_interpreter_stack_h
#define ir_interpreter_stack_h

namespace ir_interpreter {

class StackFrame {
 public:
  const StackFrame* parent() const { return parent_; }
  StackFrame* parent() { return parent_; }
  ir::Func* func() const { return func_; }

  const ExecutionPoint& exec_point() const { return exec_point_; }
  ExecutionPoint& exec_point() { return exec_point_; }
  void set_exec_point(ExecutionPoint exec_point) { exec_point_ = exec_point; }
  const std::unordered_map<ir::value_num_t, std::shared_ptr<ir::Constant>>& computed_values()
      const {
    return computed_values_;
  }
  std::unordered_map<ir::value_num_t, std::shared_ptr<ir::Constant>>& computed_values() {
    return computed_values_;
  }

 private:
  StackFrame(StackFrame* parent, ir::Func* func)
      : parent_(parent), func_(func), exec_point_(ExecutionPoint::AtFuncEntry(func)) {}

  StackFrame* parent_;
  ir::Func* func_;

  ExecutionPoint exec_point_;
  std::unordered_map<ir::value_num_t, std::shared_ptr<ir::Constant>> computed_values_;

  friend class Stack;
};

class Stack {
 public:
  std::size_t depth() const { return frames_.size(); }
  const std::vector<const StackFrame*> frames() const;
  const StackFrame* current_frame() const;
  StackFrame* current_frame();

  StackFrame* PushFrame(ir::Func* func);
  void PopCurrentFrame();

 private:
  std::vector<std::unique_ptr<StackFrame>> frames_;
};

}  // namespace ir_interpreter

#endif /* ir_interpreter_stack_h */
