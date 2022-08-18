//
//  stack.cc
//  Katara
//
//  Created by Arne Philipeit on 8/7/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "stack.h"

namespace ir_interpreter {

const std::vector<const StackFrame*> Stack::frames() const {
  std::vector<const StackFrame*> frames;
  for (auto& frame : frames_) {
    frames.push_back(frame.get());
  }
  return frames;
}

const StackFrame* Stack::current_frame() const {
  return depth() > 0 ? frames_.back().get() : nullptr;
}

StackFrame* Stack::current_frame() { return depth() > 0 ? frames_.back().get() : nullptr; }

StackFrame* Stack::PushFrame(ir::Func* func) {
  frames_.push_back(std::unique_ptr<StackFrame>(new StackFrame(current_frame(), func)));
  return frames_.back().get();
}

void Stack::PopCurrentFrame() { frames_.pop_back(); }

}  // namespace ir_interpreter
