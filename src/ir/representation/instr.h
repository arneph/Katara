//
//  instr.h
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_instr_h
#define ir_instr_h

#include <memory>
#include <string>
#include <vector>

#include "ir/representation/value.h"

namespace ir {

class Block;

class Instr {
 public:
  Instr();
  virtual ~Instr() {}

  int64_t number() const;
  Block* block() const;

  virtual std::vector<Computed> DefinedValues() const = 0;
  virtual std::vector<Computed> UsedValues() const = 0;

  virtual std::string ToString() const = 0;

  friend Block;

 private:
  int64_t number_;
  Block* block_;
};

class Computation : public Instr {
 public:
  ~Computation() override;

  Computed result() const;

  std::vector<Computed> DefinedValues() const override;

 protected:
  Computation(Computed result);

 private:
  Computed result_;
};

class MovInstr : public Computation {
 public:
  MovInstr(Computed result, Value origin);
  ~MovInstr() override;

  Value origin() const;

  std::vector<Computed> UsedValues() const override;

  std::string ToString() const override;

 private:
  Value origin_;
};

class PhiInstr : public Computation {
 public:
  PhiInstr(Computed result, std::vector<InheritedValue> args);
  ~PhiInstr() override;

  const std::vector<InheritedValue>& args() const;

  Value ValueInheritedFromBlock(int64_t bnum) const;

  std::vector<Computed> UsedValues() const override;

  std::string ToString() const override;

 private:
  std::vector<InheritedValue> args_;
};

enum class UnaryALOperation : uint8_t { kNot, kNeg };

extern bool is_unary_al_operation_string(std::string op_str);
extern UnaryALOperation to_unary_al_operation(std::string op_str);
extern std::string to_string(UnaryALOperation op);

class UnaryALInstr : public Computation {
 public:
  UnaryALInstr(UnaryALOperation operation, Computed result, Value operand);
  ~UnaryALInstr() override;

  UnaryALOperation operation() const;
  Value operand() const;

  std::vector<Computed> UsedValues() const override;

  std::string ToString() const override;

 private:
  UnaryALOperation operation_;
  Value operand_;
};

enum class BinaryALOperation : uint8_t { kAnd, kOr, kXor, kAdd, kSub, kMul, kDiv, kRem };

extern bool is_binary_al_operation_string(std::string op_str);
extern BinaryALOperation to_binary_al_operation(std::string op_str);
extern std::string to_string(BinaryALOperation op);

class BinaryALInstr : public Computation {
 public:
  BinaryALInstr(BinaryALOperation operation, Computed result, Value operand_a, Value operand_b);
  ~BinaryALInstr() override;

  BinaryALOperation operation() const;
  Value operand_a() const;
  Value operand_b() const;

  std::vector<Computed> UsedValues() const override;

  std::string ToString() const override;

 private:
  BinaryALOperation operation_;
  Value operand_a_;
  Value operand_b_;
};

enum class CompareOperation : uint8_t {
  kEqual,
  kNotEqual,
  kGreater,
  kGreaterOrEqual,
  kLessOrEqual,
  kLess
};

extern CompareOperation comuted(CompareOperation op);
extern CompareOperation negated(CompareOperation op);
extern bool is_compare_operation_string(std::string op_str);
extern CompareOperation to_compare_operation(std::string op_str);
extern std::string to_string(CompareOperation op);

class CompareInstr : public Computation {
 public:
  CompareInstr(CompareOperation operation, Computed result, Value operand_a, Value operand_b);
  ~CompareInstr() override;

  CompareOperation operation() const;
  Value operand_a() const;
  Value operand_b() const;

  std::vector<Computed> UsedValues() const override;

  std::string ToString() const override;

 private:
  CompareOperation operation_;
  Value operand_a_;
  Value operand_b_;
};

class JumpInstr : public Instr {
 public:
  JumpInstr(BlockValue destination);
  ~JumpInstr() override;

  BlockValue destination() const;

  std::vector<Computed> DefinedValues() const override;
  std::vector<Computed> UsedValues() const override;

  std::string ToString() const override;

 private:
  BlockValue destination_;
};

class JumpCondInstr : public Instr {
 public:
  JumpCondInstr(Value condition, BlockValue destination_true, BlockValue destination_false);
  ~JumpCondInstr() override;

  Value condition() const;
  BlockValue destination_true() const;
  BlockValue destination_false() const;

  std::vector<Computed> DefinedValues() const override;
  std::vector<Computed> UsedValues() const override;

  std::string ToString() const override;

 private:
  Value condition_;
  BlockValue destination_true_;
  BlockValue destination_false_;
};

class CallInstr : public Instr {
 public:
  CallInstr(Value func, std::vector<Computed> results, std::vector<Value> args);
  ~CallInstr() override;

  Value func() const;
  const std::vector<Computed>& results() const;
  const std::vector<Value>& args() const;

  std::vector<Computed> DefinedValues() const override;
  std::vector<Computed> UsedValues() const override;

  std::string ToString() const override;

 private:
  Value func_;
  std::vector<Computed> results_;
  std::vector<Value> args_;
};

class ReturnInstr : public Instr {
 public:
  ReturnInstr(std::vector<Value> args);
  ~ReturnInstr() override;

  const std::vector<Value>& args() const;

  std::vector<Computed> DefinedValues() const override;
  std::vector<Computed> UsedValues() const override;

  std::string ToString() const override;

 private:
  std::vector<Value> args_;
};

}  // namespace ir

#endif /* ir_instr_h */
