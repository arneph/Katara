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
#include <string_view>
#include <vector>

#include "src/common/atomics/atomics.h"
#include "src/common/positions/positions.h"
#include "src/ir/representation/object.h"
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
  kSyscall,
  kCall,
  kReturn,

  kLangPanic,
  kLangMakeSharedPointer,
  kLangCopySharedPointer,
  kLangDeleteSharedPointer,
  kLangMakeUniquePointer,
  kLangDeleteUniquePointer,
  kLangStringIndex,
  kLangStringConcat,
};

class Instr : public Object {
 public:
  virtual ~Instr() = default;

  virtual std::vector<std::shared_ptr<Computed>> DefinedValues() const = 0;
  virtual std::vector<std::shared_ptr<Value>> UsedValues() const = 0;

  constexpr Object::Kind object_kind() const final { return Object::Kind::kInstr; }
  constexpr virtual InstrKind instr_kind() const = 0;
  bool IsControlFlowInstr() const;

  common::pos_t start() const { return start_; }
  common::pos_t end() const { return end_; }
  void SetPositions(common::pos_t start, common::pos_t end);
  void ClearPositions() { SetPositions(common::kNoPos, common::kNoPos); }

  virtual std::string OperationString() const = 0;
  virtual void WriteRefString(std::ostream& os) const override;

  constexpr virtual bool operator==(const Instr& that) const = 0;

 private:
  common::pos_t start_ = common::kNoPos;
  common::pos_t end_ = common::kNoPos;
};

constexpr bool IsEqual(const Instr* instr_a, const Instr* instr_b) {
  if (instr_a == instr_b) return true;
  if (instr_a == nullptr || instr_b == nullptr) return false;
  return *instr_a == *instr_b;
}

class Computation : public Instr {
 public:
  std::shared_ptr<Computed> result() const { return result_; }
  void set_result(std::shared_ptr<Computed> result) { result_ = result; }

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
  MovInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> origin)
      : Computation(result), origin_(origin) {}

  std::shared_ptr<Value> origin() const { return origin_; }
  void set_origin(std::shared_ptr<Value> origin) { origin_ = origin; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override {
    return std::vector<std::shared_ptr<Value>>{origin_};
  }

  InstrKind instr_kind() const override { return InstrKind::kMov; }
  std::string OperationString() const override { return "mov"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> origin_;
};

class PhiInstr : public Computation {
 public:
  PhiInstr(std::shared_ptr<Computed> result, std::vector<std::shared_ptr<InheritedValue>> args)
      : Computation(result), args_(args) {}

  const std::vector<std::shared_ptr<InheritedValue>>& args() const { return args_; }
  std::vector<std::shared_ptr<InheritedValue>>& args() { return args_; }

  std::shared_ptr<Value> ValueInheritedFromBlock(block_num_t bnum) const;

  std::vector<std::shared_ptr<Value>> UsedValues() const override;

  InstrKind instr_kind() const override { return InstrKind::kPhi; }
  std::string OperationString() const override { return "phi"; }
  void WriteRefString(std::ostream& os) const override;

  bool operator==(const Instr& that) const override;

 private:
  std::vector<std::shared_ptr<InheritedValue>> args_;
};

class Conversion : public Computation {
 public:
  Conversion(std::shared_ptr<Computed> result, std::shared_ptr<Value> operand)
      : Computation(result), operand_(operand) {}

  std::shared_ptr<Value> operand() const { return operand_; }
  void set_operand(std::shared_ptr<Value> operand) { operand_ = operand; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {operand_}; }

  InstrKind instr_kind() const override { return InstrKind::kConversion; }
  std::string OperationString() const override { return "conv"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> operand_;
};

class BoolNotInstr : public Computation {
 public:
  BoolNotInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> operand)
      : Computation(result), operand_(operand) {}

  std::shared_ptr<Value> operand() const { return operand_; }
  void set_operand(std::shared_ptr<Value> operand) { operand_ = operand; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {operand_}; }

  InstrKind instr_kind() const override { return InstrKind::kBoolNot; }
  std::string OperationString() const override { return "bnot"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> operand_;
};

class BoolBinaryInstr : public Computation {
 public:
  BoolBinaryInstr(std::shared_ptr<Computed> result, common::Bool::BinaryOp operation,
                  std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b)
      : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {}

  common::Bool::BinaryOp operation() const { return operation_; }
  void set_operation(common::Bool::BinaryOp operation) { operation_ = operation; }

  std::shared_ptr<Value> operand_a() const { return operand_a_; }
  void set_operand_a(std::shared_ptr<Value> operand_a) { operand_a_ = operand_a; }

  std::shared_ptr<Value> operand_b() const { return operand_b_; }
  void set_operand_b(std::shared_ptr<Value> operand_b) { operand_b_ = operand_b; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override {
    return {operand_a_, operand_b_};
  }

  InstrKind instr_kind() const override { return InstrKind::kBoolBinary; }
  std::string OperationString() const override { return common::ToString(operation_); }

  bool operator==(const Instr& that) const override;

 private:
  common::Bool::BinaryOp operation_;
  std::shared_ptr<Value> operand_a_;
  std::shared_ptr<Value> operand_b_;
};

class IntUnaryInstr : public Computation {
 public:
  IntUnaryInstr(std::shared_ptr<Computed> result, common::Int::UnaryOp operation,
                std::shared_ptr<Value> operand)
      : Computation(result), operation_(operation), operand_(operand) {}

  common::Int::UnaryOp operation() const { return operation_; }
  void set_operation(common::Int::UnaryOp operation) { operation_ = operation; }

  std::shared_ptr<Value> operand() const { return operand_; }
  void set_operand(std::shared_ptr<Value> operand) { operand_ = operand; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {operand_}; }

  InstrKind instr_kind() const override { return InstrKind::kIntUnary; }
  std::string OperationString() const override { return common::ToString(operation_); }

  bool operator==(const Instr& that) const override;

 private:
  common::Int::UnaryOp operation_;
  std::shared_ptr<Value> operand_;
};

class IntCompareInstr : public Computation {
 public:
  IntCompareInstr(std::shared_ptr<Computed> result, common::Int::CompareOp operation,
                  std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b)
      : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {}

  common::Int::CompareOp operation() const { return operation_; }
  void set_operation(common::Int::CompareOp operation) { operation_ = operation; }

  std::shared_ptr<Value> operand_a() const { return operand_a_; }
  void set_operand_a(std::shared_ptr<Value> operand_a) { operand_a_ = operand_a; }

  std::shared_ptr<Value> operand_b() const { return operand_b_; }
  void set_operand_b(std::shared_ptr<Value> operand_b) { operand_b_ = operand_b; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override {
    return {operand_a_, operand_b_};
  }

  InstrKind instr_kind() const override { return InstrKind::kIntCompare; }
  std::string OperationString() const override { return common::ToString(operation_); }

  bool operator==(const Instr& that) const override;

 private:
  common::Int::CompareOp operation_;
  std::shared_ptr<Value> operand_a_;
  std::shared_ptr<Value> operand_b_;
};

class IntBinaryInstr : public Computation {
 public:
  IntBinaryInstr(std::shared_ptr<Computed> result, common::Int::BinaryOp operation,
                 std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b)
      : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {}

  common::Int::BinaryOp operation() const { return operation_; }
  void set_operation(common::Int::BinaryOp operation) { operation_ = operation; }

  std::shared_ptr<Value> operand_a() const { return operand_a_; }
  void set_operand_a(std::shared_ptr<Value> operand_a) { operand_a_ = operand_a; }

  std::shared_ptr<Value> operand_b() const { return operand_b_; }
  void set_operand_b(std::shared_ptr<Value> operand_b) { operand_b_ = operand_b; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override {
    return {operand_a_, operand_b_};
  }

  InstrKind instr_kind() const override { return InstrKind::kIntBinary; }
  std::string OperationString() const override { return common::ToString(operation_); }

  bool operator==(const Instr& that) const override;

 private:
  common::Int::BinaryOp operation_;
  std::shared_ptr<Value> operand_a_;
  std::shared_ptr<Value> operand_b_;
};

class IntShiftInstr : public Computation {
 public:
  IntShiftInstr(std::shared_ptr<Computed> result, common::Int::ShiftOp operation,
                std::shared_ptr<Value> shifted, std::shared_ptr<Value> offset)
      : Computation(result), operation_(operation), shifted_(shifted), offset_(offset) {}

  common::Int::ShiftOp operation() const { return operation_; }
  void set_operation(common::Int::ShiftOp operation) { operation_ = operation; }

  std::shared_ptr<Value> shifted() const { return shifted_; }
  void set_shifted(std::shared_ptr<Value> shifted) { shifted_ = shifted; }

  std::shared_ptr<Value> offset() const { return offset_; }
  void set_offset(std::shared_ptr<Value> offset) { offset_ = offset; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {shifted_, offset_}; }

  InstrKind instr_kind() const override { return InstrKind::kIntShift; }
  std::string OperationString() const override { return common::ToString(operation_); }

  bool operator==(const Instr& that) const override;

 private:
  common::Int::ShiftOp operation_;
  std::shared_ptr<Value> shifted_;
  std::shared_ptr<Value> offset_;
};

class PointerOffsetInstr : public Computation {
 public:
  PointerOffsetInstr(std::shared_ptr<Computed> result, std::shared_ptr<Computed> pointer,
                     std::shared_ptr<Value> offset)
      : Computation(result), pointer_(pointer), offset_(offset) {}

  std::shared_ptr<Computed> pointer() const { return pointer_; }
  void set_pointer(std::shared_ptr<Computed> pointer) { pointer_ = pointer; }

  std::shared_ptr<Value> offset() const { return offset_; }
  void set_offset(std::shared_ptr<Value> offset) { offset_ = offset; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {pointer_, offset_}; }

  InstrKind instr_kind() const override { return InstrKind::kPointerOffset; }
  std::string OperationString() const override { return "poff"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Computed> pointer_;
  std::shared_ptr<Value> offset_;
};

class NilTestInstr : public Computation {
 public:
  NilTestInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> tested)
      : Computation(result), tested_(tested) {}

  std::shared_ptr<Value> tested() const { return tested_; }
  void set_tested(std::shared_ptr<Value> tested) { tested_ = tested; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {tested_}; }

  InstrKind instr_kind() const override { return InstrKind::kNilTest; }
  std::string OperationString() const override { return "niltest"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> tested_;
};

class MallocInstr : public Computation {
 public:
  MallocInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> size)
      : Computation(result), size_(size) {}

  std::shared_ptr<Value> size() const { return size_; }
  void set_size(std::shared_ptr<Value> size) { size_ = size; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {size_}; }

  InstrKind instr_kind() const override { return InstrKind::kMalloc; }
  std::string OperationString() const override { return "malloc"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> size_;
};

class LoadInstr : public Computation {
 public:
  LoadInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> address)
      : Computation(result), address_(address) {}

  std::shared_ptr<Value> address() const { return address_; }
  void set_address(std::shared_ptr<Value> address) { address_ = address; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {address_}; }

  InstrKind instr_kind() const override { return InstrKind::kLoad; }
  std::string OperationString() const override { return "load"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> address_;
};

class StoreInstr : public Instr {
 public:
  StoreInstr(std::shared_ptr<Value> address, std::shared_ptr<Value> value)
      : address_(address), value_(value) {}

  std::shared_ptr<Value> address() const { return address_; }
  void set_address(std::shared_ptr<Value> address) { address_ = address; }

  std::shared_ptr<Value> value() const { return value_; }
  void set_value(std::shared_ptr<Value> value) { value_ = value; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {address_, value_}; }

  InstrKind instr_kind() const override { return InstrKind::kStore; }
  std::string OperationString() const override { return "store"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> address_;
  std::shared_ptr<Value> value_;
};

class FreeInstr : public Instr {
 public:
  FreeInstr(std::shared_ptr<Value> address) : address_(address) {}

  std::shared_ptr<Value> address() const { return address_; }
  void set_address(std::shared_ptr<Value> address) { address_ = address; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {address_}; }

  InstrKind instr_kind() const override { return InstrKind::kFree; }
  std::string OperationString() const override { return "free"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> address_;
};

class JumpInstr : public Instr {
 public:
  JumpInstr(block_num_t destination) : destination_(destination) {}

  block_num_t destination() const { return destination_; }
  void set_destination(block_num_t destination) { destination_ = destination; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {}; }

  InstrKind instr_kind() const override { return InstrKind::kJump; }
  std::string OperationString() const override { return "jmp"; }
  void WriteRefString(std::ostream& os) const override;

  bool operator==(const Instr& that) const override;

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
  void set_condition(std::shared_ptr<Value> condition) { condition_ = condition; }

  block_num_t destination_true() const { return destination_true_; }
  void set_destination_true(block_num_t destination_true) { destination_true_ = destination_true; }

  block_num_t destination_false() const { return destination_false_; }
  void set_destination_false(block_num_t destination_false) {
    destination_false_ = destination_false;
  }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return {condition_}; }

  InstrKind instr_kind() const override { return InstrKind::kJumpCond; }
  std::string OperationString() const override { return "jcc"; }
  void WriteRefString(std::ostream& os) const override;

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> condition_;
  block_num_t destination_true_;
  block_num_t destination_false_;
};

class SyscallInstr : public Computation {
 public:
  SyscallInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> syscall_num,
               std::vector<std::shared_ptr<Value>> args)
      : Computation(result), syscall_num_(syscall_num), args_(args) {}

  std::shared_ptr<Value> syscall_num() const { return syscall_num_; }
  void set_syscall_num(std::shared_ptr<Value> syscall_num) { syscall_num_ = syscall_num; }

  const std::vector<std::shared_ptr<Value>>& args() const { return args_; }
  std::vector<std::shared_ptr<Value>>& args() { return args_; }

  std::vector<std::shared_ptr<Value>> UsedValues() const override;

  InstrKind instr_kind() const override { return InstrKind::kSyscall; }
  std::string OperationString() const override { return "syscall"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> syscall_num_;
  std::vector<std::shared_ptr<Value>> args_;
};

class CallInstr : public Instr {
 public:
  CallInstr(std::shared_ptr<Value> func, std::vector<std::shared_ptr<Computed>> results,
            std::vector<std::shared_ptr<Value>> args)
      : func_(func), results_(results), args_(args) {}

  std::shared_ptr<Value> func() const { return func_; }
  void set_func(std::shared_ptr<Value> func) { func_ = func; }

  const std::vector<std::shared_ptr<Computed>>& results() const { return results_; }
  std::vector<std::shared_ptr<Computed>>& results() { return results_; }

  const std::vector<std::shared_ptr<Value>>& args() const { return args_; }
  std::vector<std::shared_ptr<Value>>& args() { return args_; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return results_; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override;

  InstrKind instr_kind() const override { return InstrKind::kCall; }
  std::string OperationString() const override { return "call"; }

  bool operator==(const Instr& that) const override;

 private:
  std::shared_ptr<Value> func_;
  std::vector<std::shared_ptr<Computed>> results_;
  std::vector<std::shared_ptr<Value>> args_;
};

class ReturnInstr : public Instr {
 public:
  ReturnInstr(std::vector<std::shared_ptr<Value>> args = {}) : args_(args) {}

  const std::vector<std::shared_ptr<Value>>& args() const { return args_; }
  std::vector<std::shared_ptr<Value>>& args() { return args_; }

  std::vector<std::shared_ptr<Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<Value>> UsedValues() const override { return args_; }

  InstrKind instr_kind() const override { return InstrKind::kReturn; }
  std::string OperationString() const override { return "ret"; }

  bool operator==(const Instr& that) const override;

 private:
  std::vector<std::shared_ptr<Value>> args_;
};

}  // namespace ir

#endif /* ir_instrs_h */
