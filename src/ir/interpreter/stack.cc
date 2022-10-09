//
//  stack.cc
//  Katara
//
//  Created by Arne Philipeit on 8/7/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "stack.h"

#include <iomanip>

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

std::string Stack::ToDebuggerString() const {
  if (frames_.empty()) {
    return "Stack is empty.\n";
  }
  std::stringstream ss;
  for (std::size_t frame_index = 0; frame_index < frames_.size(); frame_index++) {
    ss << ToDebuggerString(frame_index, /*include_computed_values=*/false);
  }
  return ss.str();
}

std::string Stack::ToDebuggerString(std::size_t frame_index, bool include_computed_values) const {
  std::stringstream ss;
  WriteFrameFunc(frame_index, ss);
  ss << "\n  ";
  WriteFrameInstr(frame_index, ss);
  ss << "\n";
  if (include_computed_values) {
    WriteFrameValues(frame_index, ss);
  }
  return ss.str();
}

void Stack::WriteFrameFunc(std::size_t frame_index, std::stringstream& ss) const {
  const StackFrame* frame = frames_.at(frame_index).get();
  ss << "<" << std::setw(4) << std::setfill('0') << (frame_index + 1) << "> ";
  ss << frame->func()->RefString();
  if (frame->func()->args().empty()) {
    return;
  }
  ss << " (";
  bool first = true;
  for (auto& arg : frame->func()->args()) {
    std::shared_ptr<ir::Constant> arg_value = frame->computed_values().at(arg->number());
    if (first) {
      first = false;
    } else {
      ss << ", ";
    }
    ss << "%" << arg->number() << " = " << arg_value->RefStringWithType();
  }
  ss << ")";
}

void Stack::WriteFrameInstr(std::size_t frame_index, std::stringstream& ss) const {
  const StackFrame* frame = frames_.at(frame_index).get();
  ss << frame->exec_point().current_block()->RefString() << "";
  if (frame->exec_point().is_at_func_exit()) {
    ss << " exiting function";
    if (!frame->func()->result_types().empty()) {
      ss << " with (";
      bool first = true;
      for (const auto& result : frame->exec_point().results()) {
        if (first) {
          first = false;
        } else {
          ss << ", ";
        }
        ss << result->RefStringWithType();
      }
      ss << ")";
    }
  } else {
    ss << "[" << std::setw(3) << std::setfill('0') << frame->exec_point().next_instr_index()
       << "] ";
    ss << frame->exec_point().next_instr()->RefString();
  }
}

void Stack::WriteFrameValues(std::size_t frame_index, std::stringstream& ss) const {
  const StackFrame* frame = frames_.at(frame_index).get();
  for (const auto& [value_num, value] : frame->computed_values()) {
    ss << "  "
       << "%" << std::left << std::setw(3) << std::setfill(' ') << value_num << " = "
       << value->RefStringWithType() << "\n";
  }
}

}  // namespace ir_interpreter
