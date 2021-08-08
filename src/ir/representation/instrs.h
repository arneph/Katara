//
//  instrs.h
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_instrs_h
#define ir_instrs_h

#include <memory>
#include <string>
#include <vector>

#include "src/common/atomics.h"
#include "src/ir/representation/values.h"

namespace ir {

enum class InstrKind {
  kMov,
  kPhi,
  kConversion,
  kBoolNot,
  kBoolBinary,
  kIntUnary,
  kIntCompare,
  kIntBinary,
  kIntShift,
  kPointerOffset,
  kNilTest,

  kMalloc,
  kLoad,
  kStore,
  kFree,

  kJump,
  kJumpCond,
  kCall,
  kReturn,

  kLangPanic,
  kLangMakeSharedPointer,
  kLangCopySharedPointer,
  kLangDeleteSharedPointer,
  kLangStringIndex,
  kLangStringConcat,
};

class Instr {
 public:
  virtual ~Instr() {}

  virtual std::vector<std::shared_ptr<Computed>> DefinedValues() const = 0;
  virtual std::vector<std::shared_ptr<Value>> UsedValues() const = 0;

  virtual InstrKind instr_kind() const = 0;
  bool IsControlFlowInstr() const;

  virtual std::string ToString() const = 0;
};

class Computation : public Instr {
 public:
  std::shared_ptr<Computed> result() const { return result_; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override {
    return std::vector<std::shared_ptr<Computed>>{result_};
  }

 protected:
  Computation(std::shared_ptr<Computed> result) : result_(result) {}

 private:
  std::shared_ptr<Computed> result_;
};

class MovInstr : public Computation {
 public:
  MovInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> origin);

  std::shared_ptr<Value> origin() const { return origin_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override {
    return std::vector<std::shared_ptr<Value>>{origin_};
  }

  InstrKind instr_kind() const override { return InstrKind::kMov; }
  std::string ToString() const override {
    return result()->ToStringWithType() + " = mov " + origin()->ToString();
  }

 private:
  std::shared_ptr<Value> origin_;
};

class PhiInstr : public Computation {
 public:
  PhiInstr(std::shared_ptr<Computed> result, std::vector<std::shared_ptr<InheritedValue>> args);

  const std::vector<std::shared_ptr<InheritedValue>>& args() const { return args_; }

  std::shared_ptr<Value> ValueInheritedFromBlock(block_num_t bnum) const;

  std::vector<std::shared_ptr<Value>> UsedValues() const override;

  InstrKind instr_kind() const override { return InstrKind::kPhi; }
  std::string ToString() const override;

 private:
  std::vector<std::shared_ptr<InheritedValue>> args_;
};

class Conversion : public Computation {
 public:
  Conversion(std::shared_ptr<Computed> result, std::shared_ptr<Value> operand);

  std::shared_ptr<Value> operand() const { return operand_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {operand_}; }

  InstrKind instr_kind() const override { return InstrKind::kConversion; }
  std::string ToString() const override;

 private:
  std::shared_ptr<Value> operand_;
};

class BoolNotInstr : public Computation {
 public:
  BoolNotInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> operand);

  std::shared_ptr<Value> operand() const { return operand_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {operand_}; }

  InstrKind instr_kind() const override { return InstrKind::kBoolNot; }
  std::string ToString() const override;

 private:
  std::shared_ptr<Value> operand_;
};

class BoolBinaryInstr : public Computation {
 public:
  BoolBinaryInstr(std::shared_ptr<Computed> result, common::Bool::BinaryOp operation,
                  std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b);

  common::Bool::BinaryOp operation() const { return operation_; }
  std::shared_ptr<Value> operand_a() const { return operand_a_; }
  std::shared_ptr<Value> operand_b() const { return operand_b_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override {
    return {operand_a_, operand_b_};
  }

  InstrKind instr_kind() const override { return InstrKind::kBoolBinary; }
  std::string ToString() const override;

 private:
  common::Bool::BinaryOp operation_;
  std::shared_ptr<Value> operand_a_;
  std::shared_ptr<Value> operand_b_;
};

class IntUnaryInstr : public Computation {
 public:
  IntUnaryInstr(std::shared_ptr<Computed> result, common::Int::UnaryOp operation,
                std::shared_ptr<Value> operand);

  common::Int::UnaryOp operation() const { return operation_; }
  std::shared_ptr<Value> operand() const { return operand_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {operand_}; }

  InstrKind instr_kind() const override { return InstrKind::kIntUnary; }
  std::string ToString() const override;

 private:
  common::Int::UnaryOp operation_;
  std::shared_ptr<Value> operand_;
};

class IntCompareInstr : public Computation {
 public:
  IntCompareInstr(std::shared_ptr<Computed> result, common::Int::CompareOp operation,
                  std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b);

  common::Int::CompareOp operation() const { return operation_; }
  std::shared_ptr<Value> operand_a() const { return operand_a_; }
  std::shared_ptr<Value> operand_b() const { return operand_b_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override {
    return {operand_a_, operand_b_};
  }

  InstrKind instr_kind() const override { return InstrKind::kIntCompare; }
  std::string ToString() const override;

 private:
  common::Int::CompareOp operation_;
  std::shared_ptr<Value> operand_a_;
  std::shared_ptr<Value> operand_b_;
};

class IntBinaryInstr : public Computation {
 public:
  IntBinaryInstr(std::shared_ptr<Computed> result, common::Int::BinaryOp operation,
                 std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b);

  common::Int::BinaryOp operation() const { return operation_; }
  std::shared_ptr<Value> operand_a() const { return operand_a_; }
  std::shared_ptr<Value> operand_b() const { return operand_b_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override {
    return {operand_a_, operand_b_};
  }

  InstrKind instr_kind() const override { return InstrKind::kIntBinary; }
  std::string ToString() const override;

 private:
  common::Int::BinaryOp operation_;
  std::shared_ptr<Value> operand_a_;
  std::shared_ptr<Value> operand_b_;
};

class IntShiftInstr : public Computation {
 public:
  IntShiftInstr(std::shared_ptr<Computed> result, common::Int::ShiftOp operation,
                std::shared_ptr<Value> shifted, std::shared_ptr<Value> offset);

  common::Int::ShiftOp operation() const { return operation_; }
  std::shared_ptr<Value> shifted() const { return shifted_; }
  std::shared_ptr<Value> offset() const { return offset_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {shifted_, offset_}; }

  InstrKind instr_kind() const override { return InstrKind::kIntShift; }
  std::string ToString() const override;

 private:
  common::Int::ShiftOp operation_;
  std::shared_ptr<Value> shifted_;
  std::shared_ptr<Value> offset_;
};

class PointerOffsetInstr : public Computation {
 public:
  PointerOffsetInstr(std::shared_ptr<Computed> result, std::shared_ptr<Computed> pointer,
                     std::shared_ptr<Value> offset);

  std::shared_ptr<Computed> pointer() const { return pointer_; }
  std::shared_ptr<Value> offset() const { return offset_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {pointer_, offset_}; }

  InstrKind instr_kind() const override { return InstrKind::kPointerOffset; }
  std::string ToString() const override;

 private:
  std::shared_ptr<Computed> pointer_;
  std::shared_ptr<Value> offset_;
};

class NilTestInstr : public Computation {
 public:
  NilTestInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> tested);

  std::shared_ptr<Value> tested() const { return tested_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {tested_}; }

  InstrKind instr_kind() const override { return InstrKind::kNilTest; }
  std::string ToString() const override {
    return result()->ToStringWithType() + " = niltest " + tested_->ToString();
  }

 private:
  std::shared_ptr<Value> tested_;
};

class MallocInstr : public Computation {
 public:
  MallocInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> size)
      : Computation(result), size_(size) {}

  std::shared_ptr<Value> size() const { return size_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {size_}; }

  InstrKind instr_kind() const override { return InstrKind::kMalloc; }
  std::string ToString() const override {
    return result()->ToStringWithType() + " = malloc " + size_->ToString();
  }

 private:
  std::shared_ptr<Value> size_;
};

class LoadInstr : public Computation {
 public:
  LoadInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> address)
      : Computation(result), address_(address) {}

  std::shared_ptr<Value> address() const { return address_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {address_}; }

  InstrKind instr_kind() const override { return InstrKind::kLoad; }
  std::string ToString() const override {
    return result()->ToStringWithType() + " = load " + address_->ToString();
  }

 private:
  std::shared_ptr<Value> address_;
};

class StoreInstr : public Instr {
 public:
  StoreInstr(std::shared_ptr<Value> address, std::shared_ptr<Value> value)
      : address_(address), value_(value) {}

  std::shared_ptr<Value> address() const { return address_; }
  std::shared_ptr<Value> value() const { return value_; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {address_, value_}; }

  InstrKind instr_kind() const override { return InstrKind::kStore; }
  std::string ToString() const override {
    return "store " + address_->ToString() + ", " + value_->ToString();
  }

 private:
  std::shared_ptr<Value> address_;
  std::shared_ptr<Value> value_;
};

class FreeInstr : public Instr {
 public:
  FreeInstr(std::shared_ptr<Value> address) : address_(address) {}

  std::shared_ptr<Value> address() const { return address_; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {address_}; }

  InstrKind instr_kind() const override { return InstrKind::kFree; }
  std::string ToString() const override { return "free " + address_->ToString(); }

 private:
  std::shared_ptr<Value> address_;
};

class JumpInstr : public Instr {
 public:
  JumpInstr(block_num_t destination) : destination_(destination) {}

  block_num_t destination() const { return destination_; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {}; }

  InstrKind instr_kind() const override { return InstrKind::kJump; }
  std::string ToString() const override { return "jmp {" + std::to_string(destination_) + "}"; }

 private:
  block_num_t destination_;
};

class JumpCondInstr : public Instr {
 public:
  JumpCondInstr(std::shared_ptr<Value> condition, block_num_t destination_true,
                block_num_t destination_false)
      : condition_(condition),
        destination_true_(destination_true),
        destination_false_(destination_false) {}

  std::shared_ptr<Value> condition() const { return condition_; }
  block_num_t destination_true() const { return destination_true_; }
  block_num_t destination_false() const { return destination_false_; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {condition_}; }

  InstrKind instr_kind() const override { return InstrKind::kJumpCond; }
  std::string ToString() const override;

 private:
  std::shared_ptr<Value> condition_;
  block_num_t destination_true_;
  block_num_t destination_false_;
};

class CallInstr : public Instr {
 public:
  CallInstr(std::shared_ptr<Value> func, std::vector<std::shared_ptr<Computed>> results,
            std::vector<std::shared_ptr<Value>> args)
      : func_(func), results_(results), args_(args) {}

  std::shared_ptr<Value> func() const { return func_; }
  const std::vector<std::shared_ptr<Computed>>& results() const { return results_; }
  const std::vector<std::shared_ptr<Value>>& args() const { return args_; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return results_; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override;

  InstrKind instr_kind() const override { return InstrKind::kCall; }
  std::string ToString() const override;

 private:
  std::shared_ptr<Value> func_;
  std::vector<std::shared_ptr<Computed>> results_;
  std::vector<std::shared_ptr<Value>> args_;
};

class ReturnInstr : public Instr {
 public:
  ReturnInstr(std::vector<std::shared_ptr<Value>> args = {}) : args_(args) {}

  const std::vector<std::shared_ptr<Value>>& args() const { return args_; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return args_; }

  InstrKind instr_kind() const override { return InstrKind::kReturn; }
  std::string ToString() const override;

 private:
  std::vector<std::shared_ptr<Value>> args_;
};

}  // namespace ir

#endif /* ir_instrs_h */
