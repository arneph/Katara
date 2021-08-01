//
//  block_builder.h
//  Katara
//
//  Created by Arne Philipeit on 8/1/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_block_builder_h
#define ir_block_builder_h

#include "src/common/atomics.h"
#include "src/ir/builder/func_builder.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace ir_builder {

class BlockBuilder {
 public:
  ir::Block* block() const { return block_; }
  ir::block_num_t block_number() const { return block_->number(); }

  std::shared_ptr<ir::Value> ComputePhi(std::vector<std::shared_ptr<ir::InheritedValue>> args);

  std::shared_ptr<ir::Value> Convert(const ir::AtomicType* desired_type,
                                     std::shared_ptr<ir::Value> operand);

  std::shared_ptr<ir::Value> BoolNot(std::shared_ptr<ir::Value> operand);
  std::shared_ptr<ir::Value> BoolBinaryOp(common::Bool::BinaryOp op,
                                          std::shared_ptr<ir::Value> operand_a,
                                          std::shared_ptr<ir::Value> operand_b);
  std::shared_ptr<ir::Value> BoolEq(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return BoolBinaryOp(common::Bool::BinaryOp::kEq, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> BoolNeq(std::shared_ptr<ir::Value> operand_a,
                                     std::shared_ptr<ir::Value> operand_b) {
    return BoolBinaryOp(common::Bool::BinaryOp::kNeq, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> BoolAnd(std::shared_ptr<ir::Value> operand_a,
                                     std::shared_ptr<ir::Value> operand_b) {
    return BoolBinaryOp(common::Bool::BinaryOp::kAnd, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> BoolOr(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return BoolBinaryOp(common::Bool::BinaryOp::kOr, operand_a, operand_b);
  }

  std::shared_ptr<ir::Value> IntUnaryOp(common::Int::UnaryOp op,
                                        std::shared_ptr<ir::Value> operand);
  std::shared_ptr<ir::Value> IntNeg(std::shared_ptr<ir::Value> operand) {
    return IntUnaryOp(common::Int::UnaryOp::kNeg, operand);
  }
  std::shared_ptr<ir::Value> IntNot(std::shared_ptr<ir::Value> operand) {
    return IntUnaryOp(common::Int::UnaryOp::kNot, operand);
  }
  std::shared_ptr<ir::Value> IntCompareOp(common::Int::CompareOp op,
                                          std::shared_ptr<ir::Value> operand_a,
                                          std::shared_ptr<ir::Value> operand_b);
  std::shared_ptr<ir::Value> IntEq(std::shared_ptr<ir::Value> operand_a,
                                   std::shared_ptr<ir::Value> operand_b) {
    return IntCompareOp(common::Int::CompareOp::kEq, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntNeq(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntCompareOp(common::Int::CompareOp::kNeq, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntLss(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntCompareOp(common::Int::CompareOp::kLss, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntLeq(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntCompareOp(common::Int::CompareOp::kLeq, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntGeq(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntCompareOp(common::Int::CompareOp::kGeq, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntGtr(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntCompareOp(common::Int::CompareOp::kGtr, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntBinaryOp(common::Int::BinaryOp op,
                                         std::shared_ptr<ir::Value> operand_a,
                                         std::shared_ptr<ir::Value> operand_b);
  std::shared_ptr<ir::Value> IntAdd(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntBinaryOp(common::Int::BinaryOp::kAdd, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntSub(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntBinaryOp(common::Int::BinaryOp::kSub, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntMul(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntBinaryOp(common::Int::BinaryOp::kMul, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntDiv(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntBinaryOp(common::Int::BinaryOp::kDiv, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntRem(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntBinaryOp(common::Int::BinaryOp::kRem, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntAnd(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntBinaryOp(common::Int::BinaryOp::kAnd, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntOr(std::shared_ptr<ir::Value> operand_a,
                                   std::shared_ptr<ir::Value> operand_b) {
    return IntBinaryOp(common::Int::BinaryOp::kOr, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntXor(std::shared_ptr<ir::Value> operand_a,
                                    std::shared_ptr<ir::Value> operand_b) {
    return IntBinaryOp(common::Int::BinaryOp::kXor, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntAndNot(std::shared_ptr<ir::Value> operand_a,
                                       std::shared_ptr<ir::Value> operand_b) {
    return IntBinaryOp(common::Int::BinaryOp::kAndNot, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntShift(common::Int::ShiftOp op, std::shared_ptr<ir::Value> operand_a,
                                      std::shared_ptr<ir::Value> operand_b);
  std::shared_ptr<ir::Value> IntShiftLeft(std::shared_ptr<ir::Value> operand_a,
                                          std::shared_ptr<ir::Value> operand_b) {
    return IntShift(common::Int::ShiftOp::kLeft, operand_a, operand_b);
  }
  std::shared_ptr<ir::Value> IntShiftRight(std::shared_ptr<ir::Value> operand_a,
                                           std::shared_ptr<ir::Value> operand_b) {
    return IntShift(common::Int::ShiftOp::kRight, operand_a, operand_b);
  }

  std::shared_ptr<ir::Computed> OffsetPointer(std::shared_ptr<ir::Computed> pointer,
                                              std::shared_ptr<ir::Value> offset);
  std::shared_ptr<ir::Value> IsNil(std::shared_ptr<ir::Value> operand);

  std::shared_ptr<ir::Computed> Malloc(std::shared_ptr<ir::Value> size);
  std::shared_ptr<ir::Computed> Load(const ir::Type* loaded_type,
                                     std::shared_ptr<ir::Value> address);
  void Store(std::shared_ptr<ir::Value> address, std::shared_ptr<ir::Value> value);
  void Free(std::shared_ptr<ir::Value> address);

  void Jump(ir::block_num_t destination);
  void JumpCond(std::shared_ptr<ir::Value> condition, ir::block_num_t destination_true,
                ir::block_num_t destination_false);

  std::vector<std::shared_ptr<ir::Computed>> Call(
      ir::func_num_t func, std::vector<std::shared_ptr<ir::Value>> args = {});
  std::vector<std::shared_ptr<ir::Computed>> Call(std::shared_ptr<ir::Value> func,
                                                  std::vector<const ir::Type*> result_types,
                                                  std::vector<std::shared_ptr<ir::Value>> args);

  void Return(std::vector<std::shared_ptr<ir::Value>> args = {});

 protected:
  BlockBuilder(FuncBuilder& func_builder, ir::Block* block)
      : func_builder_(func_builder), block_(block) {}

  FuncBuilder& func_builder() { return func_builder_; }

  std::shared_ptr<ir::Computed> MakeComputed(const ir::Type* type);

  template <class InstrType, class... Args>
  void AddInstr(Args&&... args);

 private:
  FuncBuilder& func_builder_;
  ir::Block* block_;

  friend FuncBuilder;
};

}  // namespace ir_builder

#endif /* ir_block_builder_h */
