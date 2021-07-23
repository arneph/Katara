//
//  interpreter.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 6/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "interpreter.h"

#include "src/common/logging.h"

namespace ir_interpreter {

void Interpreter::run() {
  ir::Func* entry_func = program_->entry_func();
  if (!entry_func->args().empty()) {
    // TODO: support argc, argv
    common::fail("entry func has arguments");
  } else if (entry_func->result_types().size() != 1) {
    common::fail("entry func does not have one result");
  }

  state_ = ExecutionState::kRunning;
  std::vector<std::unique_ptr<ir::Constant>> results = CallFunc(entry_func, {});
  ir::IntConstant* result = static_cast<ir::IntConstant*>(results.front().get());
  exit_code_ = result->value().AsInt64();
  state_ = ExecutionState::kTerminated;
}

std::vector<std::unique_ptr<ir::Constant>> Interpreter::CallFunc(ir::Func* func,
                                                                 std::vector<ir::Constant*> args) {
  ir::Block* current_block = func->entry_block();
  ir::Block* previous_block = nullptr;

  FuncContext ctx;

  while (true) {
    for (size_t i = 0; i < current_block->instrs().size(); i++) {
      ir::Instr* instr = current_block->instrs().at(i).get();
      switch (instr->instr_kind()) {
        case ir::InstrKind::kConversion:
          ExecuteConversion(static_cast<ir::Conversion*>(instr), ctx);
          break;
        case ir::InstrKind::kIntBinary:
          ExecuteIntBinaryInstr(static_cast<ir::IntBinaryInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kIntShift:
          ExecuteIntShiftInstr(static_cast<ir::IntShiftInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kReturn: {
          ir::ReturnInstr* return_instr = static_cast<ir::ReturnInstr*>(instr);
          return EvaluateFuncResults(return_instr->args(), ctx);
        }
        default:
          common::fail("interpreter does not support instruction");
      }
    }
  }
}

void Interpreter::ExecuteConversion(ir::Conversion* instr, FuncContext& ctx) {
  ir::Constant* operand = Evaluate(instr->operand(), ctx);
  ctx.computed_values_.insert(
      {instr->result()->number(), ComputeConversion(instr->result()->type(), operand)});
}

std::unique_ptr<ir::Constant> Interpreter::ComputeConversion(const ir::Type* result_type,
                                                             ir::Constant* operand) {
  if (result_type->type_kind() == ir::TypeKind::kBool &&
      operand->type()->type_kind() == ir::TypeKind::kInt) {
    common::Int value = static_cast<ir::IntConstant*>(operand)->value();
    return std::make_unique<ir::BoolConstant>(value.ConvertToBool());

  } else if (result_type->type_kind() == ir::TypeKind::kInt &&
             operand->type()->type_kind() == ir::TypeKind::kBool) {
    common::IntType int_type = static_cast<const ir::IntType*>(result_type)->int_type();
    bool value = static_cast<ir::BoolConstant*>(operand)->value();
    return std::make_unique<ir::IntConstant>(common::Bool::ConvertTo(int_type, value));

  } else if (result_type->type_kind() == ir::TypeKind::kInt &&
             operand->type()->type_kind() == ir::TypeKind::kInt) {
    common::IntType int_type = static_cast<const ir::IntType*>(result_type)->int_type();
    common::Int value = static_cast<ir::IntConstant*>(operand)->value();
    if (!value.CanConvertTo(int_type)) {
      common::fail("can not handle conversion instr");
    }
    return std::make_unique<ir::IntConstant>(value.ConvertTo(int_type));
  } else {
    common::fail("interpreter does not support conversion");
  }
}

void Interpreter::ExecuteIntBinaryInstr(ir::IntBinaryInstr* instr, FuncContext& ctx) {
  common::Int a = EvaluateInt(instr->operand_a(), ctx);
  common::Int b = EvaluateInt(instr->operand_b(), ctx);
  if (!common::Int::CanCompute(a, b)) {
    common::fail("can not compute binary instr");
  }
  common::Int result = common::Int::Compute(a, instr->operation(), b);
  ctx.computed_values_.insert(
      {instr->result()->number(), std::make_unique<ir::IntConstant>(result)});
}

void Interpreter::ExecuteIntShiftInstr(ir::IntShiftInstr* instr, FuncContext& ctx) {
  common::Int shifted = EvaluateInt(instr->shifted(), ctx);
  common::Int offset = EvaluateInt(instr->offset(), ctx);
  common::Int result = common::Int::Shift(shifted, instr->operation(), offset);
  ctx.computed_values_.insert(
      {instr->result()->number(), std::make_unique<ir::IntConstant>(result)});
}

std::vector<std::unique_ptr<ir::Constant>> Interpreter::EvaluateFuncResults(
    const std::vector<std::shared_ptr<ir::Value>>& ir_values, FuncContext& ctx) {
  std::vector<std::unique_ptr<ir::Constant>> results;
  results.reserve(ir_values.size());
  for (auto ir_value : ir_values) {
    switch (ir_value->kind()) {
      case ir::Value::Kind::kConstant:
        switch (ir_value->type()->type_kind()) {
          case ir::TypeKind::kBool: {
            auto constant = static_cast<ir::BoolConstant*>(ir_value.get());
            results.push_back(std::make_unique<ir::BoolConstant>(constant->value()));
            break;
          }
          case ir::TypeKind::kInt: {
            auto constant = static_cast<ir::IntConstant*>(ir_value.get());
            results.push_back(std::make_unique<ir::IntConstant>(constant->value()));
            break;
          }
          case ir::TypeKind::kPointer: {
            auto constant = static_cast<ir::PointerConstant*>(ir_value.get());
            results.push_back(std::make_unique<ir::PointerConstant>(constant->value()));
            break;
          }
          case ir::TypeKind::kFunc: {
            auto constant = static_cast<ir::FuncConstant*>(ir_value.get());
            results.push_back(std::make_unique<ir::FuncConstant>(constant->value()));
            break;
          }
          default:
            common::fail("interpreter does not support constant");
        }
        break;
      case ir::Value::Kind::kComputed: {
        auto computed = static_cast<ir::Computed*>(ir_value.get());
        results.push_back(std::move(ctx.computed_values_.at(computed->number())));
        break;
      }
      case ir::Value::Kind::kInherited:
        common::fail("tried to evaluate inherited value");
    }
  }
  return results;
}

std::vector<ir::Constant*> Interpreter::Evaluate(
    const std::vector<std::shared_ptr<ir::Value>>& ir_values, FuncContext& ctx) {
  std::vector<ir::Constant*> values;
  values.reserve(ir_values.size());
  for (auto ir_value : ir_values) {
    values.push_back(Evaluate(ir_value, ctx));
  }
  return values;
}

bool Interpreter::EvaluateBool(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  return static_cast<ir::BoolConstant*>(Evaluate(ir_value, ctx))->value();
}

common::Int Interpreter::EvaluateInt(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  return static_cast<ir::IntConstant*>(Evaluate(ir_value, ctx))->value();
}

int64_t Interpreter::EvaluatePointer(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  return static_cast<ir::PointerConstant*>(Evaluate(ir_value, ctx))->value();
}

ir::func_num_t Interpreter::EvaluateFunc(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  return static_cast<ir::FuncConstant*>(Evaluate(ir_value, ctx))->value();
}

ir::Constant* Interpreter::Evaluate(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  switch (ir_value->kind()) {
    case ir::Value::Kind::kConstant:
      return static_cast<ir::Constant*>(ir_value.get());
    case ir::Value::Kind::kComputed: {
      auto computed = static_cast<ir::Computed*>(ir_value.get());
      return ctx.computed_values_.at(computed->number()).get();
    }
    case ir::Value::Kind::kInherited:
      common::fail("tried to evaluate inherited value");
  }
}

}  // namespace ir_interpreter
