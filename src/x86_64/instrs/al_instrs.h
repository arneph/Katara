//
//  al_instrs.h
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_al_instrs_h
#define x86_64_al_instrs_h

#include <memory>
#include <string>

#include "src/common/data.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/machine_code/linker.h"
#include "src/x86_64/ops.h"

namespace x86_64 {

class UnaryALInstr : public Instr {
 public:
  UnaryALInstr(RM op) : op_(op) {}
  virtual ~UnaryALInstr() override {}

  RM op() const { return op_; }

  int8_t Encode(Linker& linker, common::data code) const override;

 protected:
  virtual uint8_t Opcode() const = 0;
  virtual uint8_t OpcodeExt() const = 0;

 private:
  RM op_;
};

class BinaryALInstr : public Instr {
 public:
  BinaryALInstr(RM op_a, Operand op_b);
  virtual ~BinaryALInstr() override {}

  RM op_a() const { return op_a_; }
  Operand op_b() const { return op_b_; }

  int8_t Encode(Linker& linker, common::data code) const override;

 protected:
  enum class OpEncoding : uint8_t {
    kRM_IMM,
    kRM_IMM8,
    kRM_REG,
    kREG_RM,
  };

  OpEncoding op_encoding() const { return op_encoding_; }

  bool CanUseRegAShortcut() const;

  virtual uint8_t Opcode() const = 0;
  virtual uint8_t OpcodeExt() const = 0;

 private:
  OpEncoding op_encoding_;
  RM op_a_;
  Operand op_b_;
};

class Not final : public UnaryALInstr {
 public:
  using UnaryALInstr::UnaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class And final : public BinaryALInstr {
 public:
  using BinaryALInstr::BinaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class Or final : public BinaryALInstr {
 public:
  using BinaryALInstr::BinaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class Xor final : public BinaryALInstr {
 public:
  using BinaryALInstr::BinaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class Neg final : public UnaryALInstr {
 public:
  using UnaryALInstr::UnaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class Add final : public BinaryALInstr {
 public:
  using BinaryALInstr::BinaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class Adc final : public BinaryALInstr {
 public:
  using BinaryALInstr::BinaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class Sub final : public BinaryALInstr {
 public:
  using BinaryALInstr::BinaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class Sbb final : public BinaryALInstr {
 public:
  using BinaryALInstr::BinaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class Cmp final : public BinaryALInstr {
 public:
  using BinaryALInstr::BinaryALInstr;

  std::string ToString() const override;

 private:
  uint8_t Opcode() const override;
  uint8_t OpcodeExt() const override;
};

class Mul final : public Instr {
 public:
  Mul(RM rm) : factor_(rm) {}

  RM factor() const { return factor_; }

  int8_t Encode(Linker& linker, common::data code) const override;
  std::string ToString() const override;

 private:
  RM factor_;
};

class Imul final : public Instr {
 public:
  Imul(RM rm);
  Imul(Reg reg, RM rm);
  Imul(Reg reg, RM rm, Imm imm);

  Reg factor_a() const { return factor_a_; }
  RM factor_b() const { return factor_b_; }
  Imm factor_c() const { return factor_c_; }

  int8_t Encode(Linker& linker, common::data code) const override;
  std::string ToString() const override;

 private:
  typedef enum : uint8_t {
    kRegAD_RM,
    kReg_RM,
    kReg_RM_IMM,
    kReg_RM_IMM8,
  } ImulType;

  bool CanSkipImm() const;

  ImulType imul_type_;
  Reg factor_a_;
  RM factor_b_;
  Imm factor_c_;
};

class Div final : public Instr {
 public:
  Div(RM rm) : divisor_(rm) {}

  RM divisor() const { return divisor_; }

  int8_t Encode(Linker& linker, common::data code) const override;
  std::string ToString() const override;

 private:
  RM divisor_;
};

class Idiv final : public Instr {
 public:
  Idiv(RM rm) : divisor_(rm) {}

  RM divisor() const { return divisor_; }

  int8_t Encode(Linker& linker, common::data code) const override;
  std::string ToString() const override;

 private:
  RM divisor_;
};

class SignExtendRegA final : public Instr {
 public:
  SignExtendRegA(Size op_size);

  Size op_size() const { return op_size_; }

  int8_t Encode(Linker& linker, common::data code) const override;
  std::string ToString() const override;

 private:
  Size op_size_;
};

class SignExtendRegAD final : public Instr {
  SignExtendRegAD(Size op_size);

  Size op_size() const { return op_size_; }

  int8_t Encode(Linker& linker, common::data code) const override;
  std::string ToString() const override;

 private:
  Size op_size_;
};

class Test final : public Instr {
 public:
  Test(RM rm, Imm imm);
  Test(RM rm, Reg reg);

  RM op_a() const { return op_a_; }
  Operand op_b() const { return op_b_; }

  int8_t Encode(Linker& linker, common::data code) const override;
  std::string ToString() const override;

 private:
  typedef enum : uint8_t { kRM_IMM, kRM_REG } TestType;

  bool CanUseRegAShortcut() const;

  TestType test_type_;
  RM op_a_;
  Operand op_b_;
};

}  // namespace x86_64

#endif /* x86_64_al_instrs_h */
