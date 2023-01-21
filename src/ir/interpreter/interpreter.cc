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

using ::common::atomics::Bool;
using ::common::atomics::Int;
using ::common::atomics::IntType;
using ::common::logging::fail;

Interpreter::Interpreter(ir::Program* program, bool sanitize) : heap_(sanitize), program_(program) {
  if (program_->entry_func_num() == ir::kNoFuncNum) {
    fail("program has no entry function");
  }
  ir::Func* entry_func = program_->entry_func();
  if (!entry_func->args().empty()) {
    // TODO: support argc, argv
    fail("entry function has arguments");
  } else if (entry_func->result_types().size() != 1) {
    fail("entry function does not have one result");
  }

  stack_.PushFrame(entry_func);
}

int64_t Interpreter::exit_code() const {
  if (!HasProgramCompleted()) {
    fail("program has not terminated");
  }
  return exit_code_.value();
}

void Interpreter::Run() {
  while (!HasProgramCompleted()) {
    ExecuteStep();
  }
}

void Interpreter::ExecuteStep() {
  if (stack_.current_frame()->exec_point().is_at_func_exit()) {
    ExecuteFuncExit();
  } else {
    ExecuteInstr(stack_.current_frame()->exec_point().next_instr());
  }
}

void Interpreter::ExecuteFuncExit() {
  std::vector<std::shared_ptr<ir::Constant>> results =
      stack_.current_frame()->exec_point().results();
  stack_.PopCurrentFrame();

  if (stack_.depth() == 0) {
    ir::IntConstant* result = static_cast<ir::IntConstant*>(results.front().get());
    exit_code_ = result->value().AsInt64();

  } else {
    auto call_instr =
        static_cast<ir::CallInstr*>(stack_.current_frame()->exec_point().next_instr());
    for (std::size_t i = 0; i < results.size(); i++) {
      ir::value_num_t result_num = call_instr->results().at(i)->number();
      std::shared_ptr<ir::Constant>& result_value = results.at(i);
      stack_.current_frame()->computed_values().insert_or_assign(result_num, result_value);
    }
    stack_.current_frame()->exec_point().AdvanceToNextInstr();
  }
}

void Interpreter::ExecuteInstr(ir::Instr* instr) {
  switch (instr->instr_kind()) {
    case ir::InstrKind::kMov:
      ExecuteMovInstr(static_cast<ir::MovInstr*>(instr));
      break;
    case ir::InstrKind::kPhi:
      ExecutePhiInstr(static_cast<ir::PhiInstr*>(instr));
      break;
    case ir::InstrKind::kConversion:
      ExecuteConversion(static_cast<ir::Conversion*>(instr));
      break;
    case ir::InstrKind::kIntBinary:
      ExecuteIntBinaryInstr(static_cast<ir::IntBinaryInstr*>(instr));
      break;
    case ir::InstrKind::kIntCompare:
      ExecuteIntCompareInstr(static_cast<ir::IntCompareInstr*>(instr));
      break;
    case ir::InstrKind::kIntShift:
      ExecuteIntShiftInstr(static_cast<ir::IntShiftInstr*>(instr));
      break;
    case ir::InstrKind::kPointerOffset:
      ExecutePointerOffsetInstr(static_cast<ir::PointerOffsetInstr*>(instr));
      break;
    case ir::InstrKind::kNilTest:
      ExecuteNilTestInstr(static_cast<ir::NilTestInstr*>(instr));
      break;
    case ir::InstrKind::kMalloc:
      ExecuteMallocInstr(static_cast<ir::MallocInstr*>(instr));
      break;
    case ir::InstrKind::kLoad:
      ExecuteLoadInstr(static_cast<ir::LoadInstr*>(instr));
      break;
    case ir::InstrKind::kStore:
      ExecuteStoreInstr(static_cast<ir::StoreInstr*>(instr));
      break;
    case ir::InstrKind::kFree:
      ExecuteFreeInstr(static_cast<ir::FreeInstr*>(instr));
      break;
    case ir::InstrKind::kJump:
      ExecuteJumpInstr(static_cast<ir::JumpInstr*>(instr));
      return;
    case ir::InstrKind::kJumpCond:
      ExecuteJumpCondInstr(static_cast<ir::JumpCondInstr*>(instr));
      return;
    case ir::InstrKind::kCall:
      ExecuteCallInstr(static_cast<ir::CallInstr*>(instr));
      return;
    case ir::InstrKind::kReturn:
      ExecuteReturnInstr(static_cast<ir::ReturnInstr*>(instr));
      return;
    default:
      fail("interpreter does not support instruction: " + instr->RefString());
  }
  stack_.current_frame()->exec_point().AdvanceToNextInstr();
}

void Interpreter::ExecuteMovInstr(ir::MovInstr* instr) {
  std::shared_ptr<ir::Constant> value = Evaluate(instr->origin());
  stack_.current_frame()->computed_values().insert_or_assign(instr->result()->number(), value);
}

void Interpreter::ExecutePhiInstr(ir::PhiInstr* instr) {
  ir::block_num_t previous_block_num =
      stack_.current_frame()->exec_point().previous_block()->number();
  for (const auto& arg : instr->args()) {
    if (arg->origin() == previous_block_num) {
      std::shared_ptr<ir::Constant> value = Evaluate(arg->value());
      stack_.current_frame()->computed_values().insert_or_assign(instr->result()->number(), value);
      return;
    }
  }
  fail("could not find inherited value for previous block");
}

void Interpreter::ExecuteConversion(ir::Conversion* instr) {
  ir::value_num_t result_num = instr->result()->number();
  const ir::Type* result_type = instr->result()->type();
  ir::TypeKind result_type_kind = result_type->type_kind();
  ir::TypeKind operand_type_kind = instr->operand()->type()->type_kind();

  if (result_type_kind == ir::TypeKind::kBool && operand_type_kind == ir::TypeKind::kInt) {
    Int operand = EvaluateInt(instr->operand());
    bool result = operand.ConvertToBool();
    stack_.current_frame()->computed_values().insert_or_assign(result_num,
                                                               ir::ToBoolConstant(result));
    return;

  } else if (result_type_kind == ir::TypeKind::kInt) {
    IntType result_int_type = static_cast<const ir::IntType*>(result_type)->int_type();

    if (operand_type_kind == ir::TypeKind::kBool) {
      bool operand = EvaluateBool(instr->operand());
      Int result = Bool::ConvertTo(result_int_type, operand);
      stack_.current_frame()->computed_values().insert_or_assign(result_num,
                                                                 ir::ToIntConstant(result));
      return;

    } else if (operand_type_kind == ir::TypeKind::kInt) {
      Int operand = EvaluateInt(instr->operand());
      if (!operand.CanConvertTo(result_int_type)) {
        fail("can not handle conversion instr");
      }
      Int result = operand.ConvertTo(result_int_type);
      stack_.current_frame()->computed_values().insert_or_assign(result_num,
                                                                 ir::ToIntConstant(result));
      return;
    }
  }

  fail("interpreter does not support conversion");
}

void Interpreter::ExecuteIntBinaryInstr(ir::IntBinaryInstr* instr) {
  Int a = EvaluateInt(instr->operand_a());
  Int b = EvaluateInt(instr->operand_b());
  if (!Int::CanCompute(a, b)) {
    fail("can not compute binary instr");
  }
  Int result = Int::Compute(a, instr->operation(), b);
  stack_.current_frame()->computed_values().insert_or_assign(instr->result()->number(),
                                                             ir::ToIntConstant(result));
}

void Interpreter::ExecuteIntCompareInstr(ir::IntCompareInstr* instr) {
  Int a = EvaluateInt(instr->operand_a());
  Int b = EvaluateInt(instr->operand_b());
  if (!Int::CanCompare(a, b)) {
    fail("can not compute compare instr");
  }
  bool result = Int::Compare(a, instr->operation(), b);
  stack_.current_frame()->computed_values().insert_or_assign(instr->result()->number(),
                                                             ir::ToBoolConstant(result));
}

void Interpreter::ExecuteIntShiftInstr(ir::IntShiftInstr* instr) {
  Int shifted = EvaluateInt(instr->shifted());
  Int offset = EvaluateInt(instr->offset());
  Int result = Int::Shift(shifted, instr->operation(), offset);
  stack_.current_frame()->computed_values().insert_or_assign(instr->result()->number(),
                                                             ir::ToIntConstant(result));
}

void Interpreter::ExecutePointerOffsetInstr(ir::PointerOffsetInstr* instr) {
  int64_t pointer = EvaluatePointer(instr->pointer());
  int64_t offset = EvaluateInt(instr->offset()).AsInt64();
  int64_t result = pointer + offset;
  stack_.current_frame()->computed_values().insert_or_assign(instr->result()->number(),
                                                             ir::ToPointerConstant(result));
}

void Interpreter::ExecuteNilTestInstr(ir::NilTestInstr* instr) {
  bool result = [this, instr]() {
    switch (instr->tested()->type()->type_kind()) {
      case ir::TypeKind::kPointer:
        return EvaluatePointer(instr->tested()) == 0;
      case ir::TypeKind::kFunc:
        return EvaluateFunc(instr->tested()) == ir::kNoFuncNum;
      default:
        fail("unexpected type for niltest");
    }
  }();
  stack_.current_frame()->computed_values().insert_or_assign(instr->result()->number(),
                                                             ir::ToBoolConstant(result));
}

void Interpreter::ExecuteMallocInstr(ir::MallocInstr* instr) {
  int64_t size = EvaluateInt(instr->size()).AsInt64();
  int64_t address = heap_.Malloc(size);
  stack_.current_frame()->computed_values().insert_or_assign(instr->result()->number(),
                                                             ir::ToPointerConstant(address));
}

void Interpreter::ExecuteLoadInstr(ir::LoadInstr* instr) {
  int64_t address = EvaluatePointer(instr->address());
  const ir::Type* result_type = instr->result()->type();
  std::shared_ptr<ir::Constant> result_value = [this, address,
                                                result_type]() -> std::shared_ptr<ir::Constant> {
    switch (result_type->type_kind()) {
      case ir::TypeKind::kBool:
        return ir::ToBoolConstant(heap_.Load<bool>(address));
      case ir::TypeKind::kInt: {
        Int result = [this, address](IntType int_type) {
          switch (int_type) {
            case IntType::kI8:
              return Int(heap_.Load<int8_t>(address));
            case IntType::kI16:
              return Int(heap_.Load<int16_t>(address));
            case IntType::kI32:
              return Int(heap_.Load<int32_t>(address));
            case IntType::kI64:
              return Int(heap_.Load<int64_t>(address));
            case IntType::kU8:
              return Int(heap_.Load<uint8_t>(address));
            case IntType::kU16:
              return Int(heap_.Load<uint16_t>(address));
            case IntType::kU32:
              return Int(heap_.Load<uint32_t>(address));
            case IntType::kU64:
              return Int(heap_.Load<uint64_t>(address));
          }
        }(static_cast<const ir::IntType*>(result_type)->int_type());
        return ir::ToIntConstant(result);
      }
      case ir::TypeKind::kPointer:
        return ir::ToPointerConstant(heap_.Load<int64_t>(address));
      case ir::TypeKind::kFunc:
        return ir::ToFuncConstant(heap_.Load<ir::func_num_t>(address));
      default:
        fail("can not handle type");
    }
  }();
  stack_.current_frame()->computed_values().insert_or_assign(instr->result()->number(),
                                                             result_value);
}

void Interpreter::ExecuteStoreInstr(ir::StoreInstr* instr) {
  int64_t address = EvaluatePointer(instr->address());
  const ir::Type* value_type = instr->value()->type();
  switch (value_type->type_kind()) {
    case ir::TypeKind::kBool: {
      bool value = EvaluateBool(instr->value());
      heap_.Store(address, value);
      return;
    }
    case ir::TypeKind::kInt: {
      Int value = EvaluateInt(instr->value());
      switch (value.type()) {
        case IntType::kI8:
          heap_.Store(address, int8_t(value.AsInt64()));
          return;
        case IntType::kI16:
          heap_.Store(address, int16_t(value.AsInt64()));
          return;
        case IntType::kI32:
          heap_.Store(address, int32_t(value.AsInt64()));
          return;
        case IntType::kI64:
          heap_.Store(address, value.AsInt64());
          return;
        case IntType::kU8:
          heap_.Store(address, uint8_t(value.AsUint64()));
          return;
        case IntType::kU16:
          heap_.Store(address, uint16_t(value.AsUint64()));
          return;
        case IntType::kU32:
          heap_.Store(address, uint32_t(value.AsUint64()));
          return;
        case IntType::kU64:
          heap_.Store(address, value.AsUint64());
          return;
      }
      break;
    }
    case ir::TypeKind::kPointer: {
      int64_t value = EvaluatePointer(instr->value());
      heap_.Store(address, value);
      return;
    }
    case ir::TypeKind::kFunc: {
      ir::func_num_t value = EvaluateFunc(instr->value());
      heap_.Store(address, value);
      return;
    }
    default:
      break;
  }
  fail("can not handle type");
}

void Interpreter::ExecuteFreeInstr(ir::FreeInstr* instr) {
  int64_t address = EvaluatePointer(instr->address());
  heap_.Free(address);
}

void Interpreter::ExecuteJumpInstr(ir::JumpInstr* instr) {
  ir::func_num_t next_block_num = instr->destination();
  ir::Block* next_block = stack_.current_frame()->func()->GetBlock(next_block_num);
  stack_.current_frame()->exec_point().AdvanceToNextBlock(next_block);
}

void Interpreter::ExecuteJumpCondInstr(ir::JumpCondInstr* instr) {
  bool cond = EvaluateBool(instr->condition());
  ir::func_num_t next_block_num = cond ? instr->destination_true() : instr->destination_false();
  ir::Block* next_block = stack_.current_frame()->func()->GetBlock(next_block_num);
  stack_.current_frame()->exec_point().AdvanceToNextBlock(next_block);
}

void Interpreter::ExecuteCallInstr(ir::CallInstr* instr) {
  ir::func_num_t func_num = EvaluateFunc(instr->func());
  ir::Func* func = program_->GetFunc(func_num);
  std::vector<std::shared_ptr<ir::Constant>> args = Evaluate(instr->args());

  stack_.PushFrame(func);
  for (std::size_t i = 0; i < args.size(); i++) {
    ir::value_num_t arg_num = func->args().at(i)->number();
    std::shared_ptr<ir::Constant>& arg_value = args.at(i);
    stack_.current_frame()->computed_values().insert({arg_num, arg_value});
  }
}

void Interpreter::ExecuteReturnInstr(ir::ReturnInstr* instr) {
  std::vector<std::shared_ptr<ir::Constant>> results = Evaluate(instr->args());
  stack_.current_frame()->exec_point().AdvanceToFuncExit(results);
}

bool Interpreter::EvaluateBool(std::shared_ptr<ir::Value> ir_value) {
  return static_cast<ir::BoolConstant*>(Evaluate(ir_value).get())->value();
}

Int Interpreter::EvaluateInt(std::shared_ptr<ir::Value> ir_value) {
  return static_cast<ir::IntConstant*>(Evaluate(ir_value).get())->value();
}

int64_t Interpreter::EvaluatePointer(std::shared_ptr<ir::Value> ir_value) {
  return static_cast<ir::PointerConstant*>(Evaluate(ir_value).get())->value();
}

ir::func_num_t Interpreter::EvaluateFunc(std::shared_ptr<ir::Value> ir_value) {
  return static_cast<ir::FuncConstant*>(Evaluate(ir_value).get())->value();
}

std::vector<std::shared_ptr<ir::Constant>> Interpreter::Evaluate(
    const std::vector<std::shared_ptr<ir::Value>>& ir_values) {
  std::vector<std::shared_ptr<ir::Constant>> values;
  values.reserve(ir_values.size());
  for (auto ir_value : ir_values) {
    values.push_back(Evaluate(ir_value));
  }
  return values;
}

std::shared_ptr<ir::Constant> Interpreter::Evaluate(std::shared_ptr<ir::Value> ir_value) {
  switch (ir_value->kind()) {
    case ir::Value::Kind::kConstant:
      return std::static_pointer_cast<ir::Constant>(ir_value);
    case ir::Value::Kind::kComputed: {
      auto computed = static_cast<ir::Computed*>(ir_value.get());
      return stack_.current_frame()->computed_values().at(computed->number());
    }
    case ir::Value::Kind::kInherited:
      fail("tried to evaluate inherited value");
  }
}

}  // namespace ir_interpreter
