//
//  interpreter.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 6/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "interpreter.h"

#include "src/common/logging/logging.h"

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
  std::vector<RuntimeConstant> results = CallFunc(entry_func, {});
  ir::IntConstant* result = static_cast<ir::IntConstant*>(results.front().get());
  exit_code_ = result->value().AsInt64();
  state_ = ExecutionState::kTerminated;
}

std::vector<Interpreter::RuntimeConstant> Interpreter::CallFunc(ir::Func* func,
                                                                std::vector<RuntimeConstant> args) {
  ir::Block* current_block = func->entry_block();
  ir::Block* previous_block = nullptr;
  FuncContext ctx;
  for (std::size_t i = 0; i < func->args().size(); i++) {
    ir::value_num_t number = func->args().at(i)->number();
    RuntimeConstant value = args.at(i);
    ctx.computed_values_.insert({number, value});
  }
  while (true) {
    for (size_t i = 0; i < current_block->instrs().size(); i++) {
      ir::Instr* instr = current_block->instrs().at(i).get();
      bool exit_instr_loop = false;
      switch (instr->instr_kind()) {
        case ir::InstrKind::kConversion:
          ExecuteConversion(static_cast<ir::Conversion*>(instr), ctx);
          break;
        case ir::InstrKind::kIntBinary:
          ExecuteIntBinaryInstr(static_cast<ir::IntBinaryInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kIntCompare:
          ExecuteIntCompareInstr(static_cast<ir::IntCompareInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kIntShift:
          ExecuteIntShiftInstr(static_cast<ir::IntShiftInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kPointerOffset:
          ExecutePointerOffsetInstr(static_cast<ir::PointerOffsetInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kNilTest:
          ExecuteNilTestInstr(static_cast<ir::NilTestInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kMalloc:
          ExecuteMallocInstr(static_cast<ir::MallocInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kLoad:
          ExecuteLoadInstr(static_cast<ir::LoadInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kStore:
          ExecuteStoreInstr(static_cast<ir::StoreInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kFree:
          ExecuteFreeInstr(static_cast<ir::FreeInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kJump:
          previous_block = current_block;
          current_block = func->GetBlock(static_cast<ir::JumpInstr*>(instr)->destination());
          exit_instr_loop = true;
          break;
        case ir::InstrKind::kJumpCond: {
          auto jump_cond_instr = static_cast<ir::JumpCondInstr*>(instr);
          bool cond = EvaluateBool(jump_cond_instr->condition(), ctx);
          previous_block = current_block;
          current_block = func->GetBlock(cond ? jump_cond_instr->destination_true()
                                              : jump_cond_instr->destination_false());
          exit_instr_loop = true;
          break;
        }
        case ir::InstrKind::kCall:
          ExecuteCallInstr(static_cast<ir::CallInstr*>(instr), ctx);
          break;
        case ir::InstrKind::kReturn: {
          ir::ReturnInstr* return_instr = static_cast<ir::ReturnInstr*>(instr);
          return Evaluate(return_instr->args(), ctx);
        }
        default:
          common::fail("interpreter does not support instruction: " + instr->ToString());
      }
      if (exit_instr_loop) {
        break;
      }
    }
  }
}

void Interpreter::ExecuteConversion(ir::Conversion* instr, FuncContext& ctx) {
  ir::value_num_t result_num = instr->result()->number();
  const ir::Type* result_type = instr->result()->type();
  ir::TypeKind result_type_kind = result_type->type_kind();
  ir::TypeKind operand_type_kind = instr->operand()->type()->type_kind();

  if (result_type_kind == ir::TypeKind::kBool && operand_type_kind == ir::TypeKind::kInt) {
    common::Int operand = EvaluateInt(instr->operand(), ctx);
    bool result = operand.ConvertToBool();
    ctx.computed_values_.insert_or_assign(result_num, ir::ToBoolConstant(result));
    return;

  } else if (result_type_kind == ir::TypeKind::kInt) {
    common::IntType result_int_type = static_cast<const ir::IntType*>(result_type)->int_type();

    if (operand_type_kind == ir::TypeKind::kBool) {
      bool operand = EvaluateBool(instr->operand(), ctx);
      common::Int result = common::Bool::ConvertTo(result_int_type, operand);
      ctx.computed_values_.insert_or_assign(result_num, ir::ToIntConstant(result));
      return;

    } else if (operand_type_kind == ir::TypeKind::kInt) {
      common::Int operand = EvaluateInt(instr->operand(), ctx);
      if (!operand.CanConvertTo(result_int_type)) {
        common::fail("can not handle conversion instr");
      }
      common::Int result = operand.ConvertTo(result_int_type);
      ctx.computed_values_.insert_or_assign(result_num, ir::ToIntConstant(result));
      return;
    }
  }

  common::fail("interpreter does not support conversion");
}

void Interpreter::ExecuteIntBinaryInstr(ir::IntBinaryInstr* instr, FuncContext& ctx) {
  common::Int a = EvaluateInt(instr->operand_a(), ctx);
  common::Int b = EvaluateInt(instr->operand_b(), ctx);
  if (!common::Int::CanCompute(a, b)) {
    common::fail("can not compute binary instr");
  }
  common::Int result = common::Int::Compute(a, instr->operation(), b);
  ctx.computed_values_.insert_or_assign(instr->result()->number(), ir::ToIntConstant(result));
}

void Interpreter::ExecuteIntCompareInstr(ir::IntCompareInstr* instr, FuncContext& ctx) {
  common::Int a = EvaluateInt(instr->operand_a(), ctx);
  common::Int b = EvaluateInt(instr->operand_b(), ctx);
  if (!common::Int::CanCompare(a, b)) {
    common::fail("can not compute compare instr");
  }
  bool result = common::Int::Compare(a, instr->operation(), b);
  ctx.computed_values_.insert_or_assign(instr->result()->number(), ir::ToBoolConstant(result));
}

void Interpreter::ExecuteIntShiftInstr(ir::IntShiftInstr* instr, FuncContext& ctx) {
  common::Int shifted = EvaluateInt(instr->shifted(), ctx);
  common::Int offset = EvaluateInt(instr->offset(), ctx);
  common::Int result = common::Int::Shift(shifted, instr->operation(), offset);
  ctx.computed_values_.insert_or_assign(instr->result()->number(), ir::ToIntConstant(result));
}

void Interpreter::ExecutePointerOffsetInstr(ir::PointerOffsetInstr* instr, FuncContext& ctx) {
  int64_t pointer = EvaluatePointer(instr->pointer(), ctx);
  int64_t offset = EvaluateInt(instr->offset(), ctx).AsInt64();
  int64_t result = pointer + offset;
  ctx.computed_values_.insert_or_assign(instr->result()->number(), ir::ToPointerConstant(result));
}

void Interpreter::ExecuteNilTestInstr(ir::NilTestInstr* instr, FuncContext& ctx) {
  bool result = [this, instr, &ctx]() {
    switch (instr->tested()->type()->type_kind()) {
      case ir::TypeKind::kPointer:
        return EvaluatePointer(instr->tested(), ctx) == 0;
      case ir::TypeKind::kFunc:
        return EvaluateFunc(instr->tested(), ctx) == ir::kNoFuncNum;
      default:
        common::fail("unexpected type for niltest");
    }
  }();
  ctx.computed_values_.insert_or_assign(instr->result()->number(), ir::ToBoolConstant(result));
}

void Interpreter::ExecuteMallocInstr(ir::MallocInstr* instr, FuncContext& ctx) {
  uint64_t size = EvaluateInt(instr->size(), ctx).AsUint64();
  int64_t address = int64_t(malloc(size));
  ctx.computed_values_.insert_or_assign(instr->result()->number(), ir::ToPointerConstant(address));
}

namespace {

template <typename T>
T LoadFromHeap(int64_t address) {
  return *((T*)(address));
}

template <typename T>
void StoreOnHeap(int64_t address, T value) {
  *((T*)(address)) = value;
}

}  // namespace

void Interpreter::ExecuteLoadInstr(ir::LoadInstr* instr, FuncContext& ctx) {
  int64_t address = EvaluatePointer(instr->address(), ctx);
  const ir::Type* result_type = instr->result()->type();
  RuntimeConstant result_value = [address, result_type]() -> RuntimeConstant {
    switch (result_type->type_kind()) {
      case ir::TypeKind::kBool:
        return ir::ToBoolConstant(LoadFromHeap<bool>(address));
      case ir::TypeKind::kInt: {
        common::Int result = [address](common::IntType int_type) {
          switch (int_type) {
            case common::IntType::kI8:
              return common::Int(LoadFromHeap<int8_t>(address));
            case common::IntType::kI16:
              return common::Int(LoadFromHeap<int16_t>(address));
            case common::IntType::kI32:
              return common::Int(LoadFromHeap<int32_t>(address));
            case common::IntType::kI64:
              return common::Int(LoadFromHeap<int64_t>(address));
            case common::IntType::kU8:
              return common::Int(LoadFromHeap<uint8_t>(address));
            case common::IntType::kU16:
              return common::Int(LoadFromHeap<uint16_t>(address));
            case common::IntType::kU32:
              return common::Int(LoadFromHeap<uint32_t>(address));
            case common::IntType::kU64:
              return common::Int(LoadFromHeap<uint64_t>(address));
          }
        }(static_cast<const ir::IntType*>(result_type)->int_type());
        return ir::ToIntConstant(result);
      }
      case ir::TypeKind::kPointer:
        return ir::ToPointerConstant(LoadFromHeap<int64_t>(address));
      case ir::TypeKind::kFunc:
        return ir::ToFuncConstant(LoadFromHeap<ir::func_num_t>(address));
      default:
        common::fail("can not handle type");
    }
  }();
  ctx.computed_values_.insert_or_assign(instr->result()->number(), result_value);
}

void Interpreter::ExecuteStoreInstr(ir::StoreInstr* instr, FuncContext& ctx) {
  int64_t address = EvaluatePointer(instr->address(), ctx);
  const ir::Type* value_type = instr->value()->type();
  switch (value_type->type_kind()) {
    case ir::TypeKind::kBool: {
      bool value = EvaluateBool(instr->value(), ctx);
      StoreOnHeap(address, value);
      return;
    }
    case ir::TypeKind::kInt: {
      common::Int value = EvaluateInt(instr->value(), ctx);
      switch (value.type()) {
        case common::IntType::kI8:
          StoreOnHeap(address, int8_t(value.AsInt64()));
          return;
        case common::IntType::kI16:
          StoreOnHeap(address, int16_t(value.AsInt64()));
          return;
        case common::IntType::kI32:
          StoreOnHeap(address, int32_t(value.AsInt64()));
          return;
        case common::IntType::kI64:
          StoreOnHeap(address, value.AsInt64());
          return;
        case common::IntType::kU8:
          StoreOnHeap(address, uint8_t(value.AsUint64()));
          return;
        case common::IntType::kU16:
          StoreOnHeap(address, uint16_t(value.AsUint64()));
          return;
        case common::IntType::kU32:
          StoreOnHeap(address, uint32_t(value.AsUint64()));
          return;
        case common::IntType::kU64:
          StoreOnHeap(address, value.AsUint64());
          return;
      }
      break;
    }
    case ir::TypeKind::kPointer: {
      int64_t value = EvaluatePointer(instr->value(), ctx);
      StoreOnHeap(address, value);
      return;
    }
    case ir::TypeKind::kFunc: {
      ir::func_num_t value = EvaluateFunc(instr->value(), ctx);
      StoreOnHeap(address, value);
      return;
    }
    default:
      break;
  }
  common::fail("can not handle type");
}

void Interpreter::ExecuteFreeInstr(ir::FreeInstr* instr, FuncContext& ctx) {
  int64_t address = EvaluatePointer(instr->address(), ctx);
  free((void*)(address));
}

void Interpreter::ExecuteCallInstr(ir::CallInstr* instr, FuncContext& ctx) {
  ir::func_num_t func_num = EvaluateFunc(instr->func(), ctx);
  ir::Func* func = program_->GetFunc(func_num);
  std::vector<RuntimeConstant> args = Evaluate(instr->args(), ctx);
  std::vector<RuntimeConstant> results = CallFunc(func, args);
  for (std::size_t i = 0; i < instr->results().size(); i++) {
    ir::value_num_t result_num = instr->results().at(i)->number();
    RuntimeConstant result_value = results.at(i);
    ctx.computed_values_.insert_or_assign(result_num, result_value);
  }
}

bool Interpreter::EvaluateBool(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  return static_cast<ir::BoolConstant*>(Evaluate(ir_value, ctx).get())->value();
}

common::Int Interpreter::EvaluateInt(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  return static_cast<ir::IntConstant*>(Evaluate(ir_value, ctx).get())->value();
}

int64_t Interpreter::EvaluatePointer(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  return static_cast<ir::PointerConstant*>(Evaluate(ir_value, ctx).get())->value();
}

ir::func_num_t Interpreter::EvaluateFunc(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx) {
  return static_cast<ir::FuncConstant*>(Evaluate(ir_value, ctx).get())->value();
}

std::vector<Interpreter::RuntimeConstant> Interpreter::Evaluate(
    const std::vector<std::shared_ptr<ir::Value>>& ir_values, FuncContext& ctx) {
  std::vector<RuntimeConstant> values;
  values.reserve(ir_values.size());
  for (auto ir_value : ir_values) {
    values.push_back(Evaluate(ir_value, ctx));
  }
  return values;
}

Interpreter::RuntimeConstant Interpreter::Evaluate(std::shared_ptr<ir::Value> ir_value,
                                                   FuncContext& ctx) {
  switch (ir_value->kind()) {
    case ir::Value::Kind::kConstant:
      return std::static_pointer_cast<ir::Constant>(ir_value);
    case ir::Value::Kind::kComputed: {
      auto computed = static_cast<ir::Computed*>(ir_value.get());
      return ctx.computed_values_.at(computed->number());
    }
    case ir::Value::Kind::kInherited:
      common::fail("tried to evaluate inherited value");
  }
}

}  // namespace ir_interpreter
