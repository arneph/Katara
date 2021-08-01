//
//  block_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 8/1/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "block_builder.h"

namespace ir_builder {

std::shared_ptr<ir::Computed> BlockBuilder::MakeComputed(const ir::Type* type) {
  return func_builder_.MakeComputed(type);
}

template <class InstrType, class... Args>
void BlockBuilder::AddInstr(Args&&... args) {
  block_->instrs().push_back(std::make_unique<InstrType>(args...));
}

std::shared_ptr<ir::Value> BlockBuilder::ComputePhi(
    std::vector<std::shared_ptr<ir::InheritedValue>> args) {
  std::shared_ptr<ir::Computed> result = MakeComputed(args.front()->type());
  AddInstr<ir::PhiInstr>(result, args);
  return result;
}

std::shared_ptr<ir::Value> BlockBuilder::Convert(const ir::AtomicType* desired_type,
                                                 std::shared_ptr<ir::Value> operand) {
  if (operand->type() == desired_type) {
    return operand;
  } else {
    std::shared_ptr<ir::Computed> result = MakeComputed(desired_type);
    AddInstr<ir::Conversion>(result, operand);
    return result;
  }
}

std::shared_ptr<ir::Value> BlockBuilder::BoolNot(std::shared_ptr<ir::Value> operand) {
  if (operand == ir::False()) {
    return ir::True();
  } else if (operand == ir::True()) {
    return ir::False();
  }
  std::shared_ptr<ir::Computed> result = MakeComputed(ir::bool_type());
  AddInstr<ir::BoolNotInstr>(result, operand);
  return result;
}

std::shared_ptr<ir::Value> BlockBuilder::BoolBinaryOp(common::Bool::BinaryOp op,
                                                      std::shared_ptr<ir::Value> operand_a,
                                                      std::shared_ptr<ir::Value> operand_b) {
  if (operand_a->kind() == ir::Value::Kind::kConstant &&
      operand_b->kind() == ir::Value::Kind::kConstant) {
    bool a = static_cast<ir::BoolConstant*>(operand_a.get())->value();
    bool b = static_cast<ir::BoolConstant*>(operand_b.get())->value();
    return common::Bool::Compute(a, op, b) ? ir::True() : ir::False();
  }
  std::shared_ptr<ir::Computed> result = MakeComputed(ir::bool_type());
  AddInstr<ir::BoolBinaryInstr>(result, op, operand_a, operand_b);
  return result;
}

std::shared_ptr<ir::Value> BlockBuilder::IntUnaryOp(common::Int::UnaryOp op,
                                                    std::shared_ptr<ir::Value> operand) {
  if (operand->kind() == ir::Value::Kind::kConstant) {
    common::Int a = static_cast<ir::IntConstant*>(operand.get())->value();
    return std::make_shared<ir::IntConstant>(common::Int::Compute(op, a));
  }
  std::shared_ptr<ir::Computed> result = MakeComputed(operand->type());
  AddInstr<ir::IntUnaryInstr>(result, op, operand);
  return result;
}

std::shared_ptr<ir::Value> BlockBuilder::IntCompareOp(common::Int::CompareOp op,
                                                      std::shared_ptr<ir::Value> operand_a,
                                                      std::shared_ptr<ir::Value> operand_b) {
  if (operand_a->kind() == ir::Value::Kind::kConstant &&
      operand_b->kind() == ir::Value::Kind::kConstant) {
    common::Int a = static_cast<ir::IntConstant*>(operand_a.get())->value();
    common::Int b = static_cast<ir::IntConstant*>(operand_b.get())->value();
    return common::Int::Compare(a, op, b) ? ir::True() : ir::False();
  }
  std::shared_ptr<ir::Computed> result = MakeComputed(ir::bool_type());
  AddInstr<ir::IntCompareInstr>(result, op, operand_a, operand_b);
  return result;
}

std::shared_ptr<ir::Value> BlockBuilder::IntBinaryOp(common::Int::BinaryOp op,
                                                     std::shared_ptr<ir::Value> operand_a,
                                                     std::shared_ptr<ir::Value> operand_b) {
  if (operand_a->kind() == ir::Value::Kind::kConstant &&
      operand_b->kind() == ir::Value::Kind::kConstant) {
    common::Int a = static_cast<ir::IntConstant*>(operand_a.get())->value();
    common::Int b = static_cast<ir::IntConstant*>(operand_b.get())->value();
    return std::make_shared<ir::IntConstant>(common::Int::Compute(a, op, b));
  }
  std::shared_ptr<ir::Computed> result = MakeComputed(operand_a->type());
  AddInstr<ir::IntBinaryInstr>(result, op, operand_a, operand_b);
  return result;
}

std::shared_ptr<ir::Value> BlockBuilder::IntShift(common::Int::ShiftOp op,
                                                  std::shared_ptr<ir::Value> operand_a,
                                                  std::shared_ptr<ir::Value> operand_b) {
  if (operand_a->kind() == ir::Value::Kind::kConstant &&
      operand_b->kind() == ir::Value::Kind::kConstant) {
    common::Int a = static_cast<ir::IntConstant*>(operand_a.get())->value();
    common::Int b = static_cast<ir::IntConstant*>(operand_b.get())->value();
    return std::make_shared<ir::IntConstant>(common::Int::Shift(a, op, b));
  }
  std::shared_ptr<ir::Computed> result = MakeComputed(operand_a->type());
  AddInstr<ir::IntShiftInstr>(result, op, operand_a, operand_b);
  return result;
}

std::shared_ptr<ir::Computed> BlockBuilder::OffsetPointer(std::shared_ptr<ir::Computed> pointer,
                                                          std::shared_ptr<ir::Value> offset) {
  if (offset == ir::I64Zero()) {
    return pointer;
  }
  std::shared_ptr<ir::Computed> result = MakeComputed(ir::pointer_type());
  AddInstr<ir::PointerOffsetInstr>(result, pointer, offset);
  return result;
}

std::shared_ptr<ir::Value> BlockBuilder::IsNil(std::shared_ptr<ir::Value> operand) {
  if (operand == ir::NilPointer() || operand == ir::NilFunc()) {
    return ir::True();
  }
  std::shared_ptr<ir::Computed> result = MakeComputed(ir::bool_type());
  AddInstr<ir::NilTestInstr>(result, operand);
  return result;
}

std::shared_ptr<ir::Computed> BlockBuilder::Malloc(std::shared_ptr<ir::Value> size) {
  std::shared_ptr<ir::Computed> result = MakeComputed(ir::pointer_type());
  AddInstr<ir::MallocInstr>(result, size);
  return result;
}

std::shared_ptr<ir::Computed> BlockBuilder::Load(const ir::Type* loaded_type,
                                                 std::shared_ptr<ir::Value> address) {
  std::shared_ptr<ir::Computed> result = MakeComputed(loaded_type);
  AddInstr<ir::LoadInstr>(result, address);
  return result;
}

void BlockBuilder::Store(std::shared_ptr<ir::Value> address, std::shared_ptr<ir::Value> value) {
  AddInstr<ir::StoreInstr>(address, value);
}

void BlockBuilder::Free(std::shared_ptr<ir::Value> address) { AddInstr<ir::FreeInstr>(address); }

void BlockBuilder::Jump(ir::block_num_t destination) {
  AddInstr<ir::JumpInstr>(destination);
  func_builder_.func()->AddControlFlow(block_number(), destination);
}

void BlockBuilder::JumpCond(std::shared_ptr<ir::Value> condition, ir::block_num_t destination_true,
                            ir::block_num_t destination_false) {
  if (condition == ir::False()) {
    Jump(destination_false);
    return;
  } else if (condition == ir::True() || destination_true == destination_false) {
    Jump(destination_true);
    return;
  }
  AddInstr<ir::JumpCondInstr>(condition, destination_true, destination_false);
  func_builder_.func()->AddControlFlow(block_number(), destination_true);
  func_builder_.func()->AddControlFlow(block_number(), destination_false);
}

std::vector<std::shared_ptr<ir::Computed>> BlockBuilder::Call(
    ir::func_num_t called_func_num, std::vector<std::shared_ptr<ir::Value>> args) {
  ir::Func* called_func = func_builder_.program_->GetFunc(called_func_num);
  std::vector<std::shared_ptr<ir::Computed>> results;
  results.reserve(called_func->result_types().size());
  for (const ir::Type* result_type : called_func->result_types()) {
    results.push_back(MakeComputed(result_type));
  }
  AddInstr<ir::CallInstr>(std::make_shared<ir::FuncConstant>(called_func_num), results, args);
  return results;
}

std::vector<std::shared_ptr<ir::Computed>> BlockBuilder::Call(
    std::shared_ptr<ir::Value> func, std::vector<const ir::Type*> result_types,
    std::vector<std::shared_ptr<ir::Value>> args) {
  std::vector<std::shared_ptr<ir::Computed>> results;
  results.reserve(result_types.size());
  for (const ir::Type* result_type : result_types) {
    results.push_back(MakeComputed(result_type));
  }
  AddInstr<ir::CallInstr>(func, results, args);
  return results;
}

void BlockBuilder::Return(std::vector<std::shared_ptr<ir::Value>> args) {
  AddInstr<ir::ReturnInstr>(args);
}

}  // namespace ir_builder
