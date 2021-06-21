//
//  interpreter.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 6/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "interpreter.h"

namespace ir_interpreter {

void Interpreter::run() {
  ir::Func* entry_func = program_->entry_func();
  if (!entry_func->args().empty()) {
    // TODO: support argc, argv
    throw "internal error: entry func has arguments";
  } else if (entry_func->result_types().size() != 1) {
    throw "internal error: entry func does not have one result";
  }

  state_ = ExecutionState::kRunning;
  Value result = CallFunc(entry_func, {}).front();
  exit_code_ = int(result.value());
  state_ = ExecutionState::kTerminated;
}

std::vector<Value> Interpreter::CallFunc(ir::Func* func, std::vector<Value> args) {
  ir::Block* current_block = func->entry_block();
  ir::Block* previous_block = nullptr;

  FuncContext ctx;

  while (true) {
    for (size_t i = 0; i < current_block->instrs().size(); i++) {
      ir::Instr* instr = current_block->instrs().at(i).get();
      switch (instr->instr_kind()) {
        case ir::InstrKind::kBinaryAL:
          ExecuteBinaryALInstr(static_cast<ir::BinaryALInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kReturn: {
          ir::ReturnInstr* return_instr = static_cast<ir::ReturnInstr*>(instr);
          return Evaluate(return_instr->args(), ctx);
        }
        default:
          throw "internal error: interpreter does not support instruction";
      }
    }
  }
}

void Interpreter::ExecuteBinaryALInstr(ir::BinaryALInstr* instr, FuncContext& ctx) {
  Value a = Evaluate(instr->operand_a(), ctx);
  Value b = Evaluate(instr->operand_b(), ctx);
  Value result = ComputBinaryOp(a, instr->operation(), b);
  ctx.computed_values_.insert({instr->result()->number(), result});
}

Value Interpreter::ComputBinaryOp(Value a, ir::BinaryALOperation op, Value b) {
  switch (op) {
    case ir::BinaryALOperation::kAdd:
      return Value{a.value() + b.value()};
    case ir::BinaryALOperation::kSub:
      return Value{a.value() - b.value()};
    case ir::BinaryALOperation::kMul:
      return Value{a.value() * b.value()};
    case ir::BinaryALOperation::kDiv:
      return Value{a.value() / b.value()};
    case ir::BinaryALOperation::kRem:
      return Value{a.value() % b.value()};
    case ir::BinaryALOperation::kOr:
      return Value{a.value() | b.value()};
    case ir::BinaryALOperation::kAnd:
      return Value{a.value() & b.value()};
    case ir::BinaryALOperation::kXor:
      return Value{a.value() ^ b.value()};
    case ir::BinaryALOperation::kAndNot:
      return Value{a.value() & ~b.value()};
  }
}

std::vector<Value> Interpreter::Evaluate(const std::vector<std::shared_ptr<ir::Value>>& ir_values,
                                         FuncContext& ctx) {
  std::vector<Value> values;
  values.reserve(ir_values.size());
  for (auto ir_value : ir_values) {
    values.push_back(Evaluate(ir_value, ctx));
  }
  return values;
}

Value Interpreter::Evaluate(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  switch (ir_value->value_kind()) {
    case ir::ValueKind::kConstant:
      return Value(static_cast<ir::Constant*>(ir_value.get())->value());
    case ir::ValueKind::kComputed:
      return ctx.computed_values_.at(static_cast<ir::Computed*>(ir_value.get())->number());
    case ir::ValueKind::kInherited:
      throw "internal error: tried to evaluate inherited value";
    default:
      throw "internal error: tried to evaluate value of unknown kind";
  }
}

}  // namespace ir_interpreter
