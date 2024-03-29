//
//  arithmetic_logic_instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "arithmetic_logic_instrs.h"

#include "src/common/logging/logging.h"
#include "src/x86_64/instrs/instr_encoder.h"

namespace x86_64 {

using ::common::data::DataView;
using ::common::logging::fail;

int8_t UnaryALInstr::Encode(Linker&, DataView code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(op_.size());
  if (op_.RequiresREX()) {
    encoder.EncodeREX();
  }

  encoder.EncodeOpcode(Opcode());
  encoder.EncodeOpcodeExt(OpcodeExt());
  encoder.EncodeRM(op_);

  return encoder.size();
}

BinaryALInstr::BinaryALInstr(RM op_a, Operand op_b) : op_a_(op_a), op_b_(op_b) {
  if (op_b.is_imm()) {
    if (op_b.size() == Size::k64) fail("unsupported imm size");

    if (op_a.size() == op_b.size() || (op_a.size() == Size::k64 && op_b.size() == Size::k32)) {
      op_encoding_ = OpEncoding::kRM_IMM;

    } else if (op_b.size() == Size::k8) {
      op_encoding_ = OpEncoding::kRM_IMM8;

    } else {
      fail("unsupported rm size, imm size combination");
    }

  } else {
    if (op_a.size() != op_b.size()) fail("unsupported rm size, reg size combination");

    if (op_a.is_reg()) {
      op_encoding_ = OpEncoding::kREG_RM;

    } else if (op_a.is_mem()) {
      if (op_b.is_reg()) {
        op_encoding_ = OpEncoding::kRM_REG;

      } else if (op_b.is_mem()) {
        fail("unsupported binary al instr: mem with mem");
      } else {
        fail("unexpected operand kind");
      }

    } else {
      fail("unexpected operand kind");
    }
  }
}

bool BinaryALInstr::CanUseRegAShortcut() const {
  if (op_encoding_ != OpEncoding::kRM_IMM) return false;
  if (!op_a_.is_reg()) return false;

  return op_a_.reg().reg() == 0;
}

int8_t BinaryALInstr::Encode(Linker&, DataView code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(op_a_.size());
  if (op_a_.RequiresREX() || op_b_.RequiresREX()) {
    encoder.EncodeREX();
  }

  encoder.EncodeOpcode(Opcode());
  if (!CanUseRegAShortcut() &&
      (op_encoding_ == OpEncoding::kRM_IMM || op_encoding_ == OpEncoding::kRM_IMM8)) {
    encoder.EncodeOpcodeExt(OpcodeExt());
  }

  if (CanUseRegAShortcut()) {
  } else if (op_encoding_ == OpEncoding::kRM_IMM || op_encoding_ == OpEncoding::kRM_IMM8 ||
             op_encoding_ == OpEncoding::kRM_REG) {
    encoder.EncodeRM(op_a_);

  } else if (op_encoding_ == OpEncoding::kREG_RM) {
    encoder.EncodeModRMReg(op_a_.reg());
  }

  if (op_encoding_ == OpEncoding::kRM_IMM || op_encoding_ == OpEncoding::kRM_IMM8) {
    encoder.EncodeImm(op_b_.imm());

  } else if (op_encoding_ == OpEncoding::kRM_REG) {
    encoder.EncodeModRMReg(op_b_.reg());

  } else if (op_encoding_ == OpEncoding::kREG_RM) {
    encoder.EncodeRM(op_b_.rm());
  }

  return encoder.size();
}

uint8_t Not::Opcode() const { return (op().size() == Size::k8) ? 0xf6 : 0xf7; }

uint8_t Not::OpcodeExt() const { return 2; }

std::string Not::ToString() const { return "not " + op().ToString(); }

uint8_t And::Opcode() const {
  if (CanUseRegAShortcut()) {
    return (op_a().size() == Size::k8) ? 0x24 : 0x25;
  }
  switch (op_encoding()) {
    case OpEncoding::kRM_IMM:
      return (op_a().size() == Size::k8) ? 0x80 : 0x81;
    case OpEncoding::kRM_IMM8:
      return 0x83;
    case OpEncoding::kRM_REG:
      return (op_a().size() == Size::k8) ? 0x20 : 0x21;
    case OpEncoding::kREG_RM:
      return (op_a().size() == Size::k8) ? 0x22 : 0x23;
  }
}

uint8_t And::OpcodeExt() const { return 4; }

std::string And::ToString() const { return "and " + op_a().ToString() + "," + op_b().ToString(); }

uint8_t Or::Opcode() const {
  if (CanUseRegAShortcut()) {
    return (op_a().size() == Size::k8) ? 0x0c : 0x0d;
  }
  switch (op_encoding()) {
    case OpEncoding::kRM_IMM:
      return (op_a().size() == Size::k8) ? 0x80 : 0x81;
    case OpEncoding::kRM_IMM8:
      return 0x83;
    case OpEncoding::kRM_REG:
      return (op_a().size() == Size::k8) ? 0x08 : 0x09;
    case OpEncoding::kREG_RM:
      return (op_a().size() == Size::k8) ? 0x0a : 0x0b;
  }
}

uint8_t Or::OpcodeExt() const { return 1; }

std::string Or::ToString() const { return "or " + op_a().ToString() + "," + op_b().ToString(); }

uint8_t Xor::Opcode() const {
  if (CanUseRegAShortcut()) {
    return (op_a().size() == Size::k8) ? 0x34 : 0x35;
  }
  switch (op_encoding()) {
    case OpEncoding::kRM_IMM:
      return (op_a().size() == Size::k8) ? 0x80 : 0x81;
    case OpEncoding::kRM_IMM8:
      return 0x83;
    case OpEncoding::kRM_REG:
      return (op_a().size() == Size::k8) ? 0x30 : 0x31;
    case OpEncoding::kREG_RM:
      return (op_a().size() == Size::k8) ? 0x32 : 0x33;
  }
}

uint8_t Xor::OpcodeExt() const { return 6; }

std::string Xor::ToString() const { return "xor " + op_a().ToString() + "," + op_b().ToString(); }

uint8_t Neg::Opcode() const { return (op().size() == Size::k8) ? 0xf6 : 0xf7; }

uint8_t Neg::OpcodeExt() const { return 3; }

std::string Neg::ToString() const { return "neg " + op().ToString(); }

uint8_t Add::Opcode() const {
  if (CanUseRegAShortcut()) {
    return (op_a().size() == Size::k8) ? 0x04 : 0x05;
  }
  switch (op_encoding()) {
    case OpEncoding::kRM_IMM:
      return (op_a().size() == Size::k8) ? 0x80 : 0x81;
    case OpEncoding::kRM_IMM8:
      return 0x83;
    case OpEncoding::kRM_REG:
      return (op_a().size() == Size::k8) ? 0x00 : 0x01;
    case OpEncoding::kREG_RM:
      return (op_a().size() == Size::k8) ? 0x02 : 0x03;
  }
}

uint8_t Add::OpcodeExt() const { return 0; }

std::string Add::ToString() const { return "add " + op_a().ToString() + "," + op_b().ToString(); }

uint8_t Adc::Opcode() const {
  if (CanUseRegAShortcut()) {
    return (op_a().size() == Size::k8) ? 0x14 : 0x15;
  }
  switch (op_encoding()) {
    case OpEncoding::kRM_IMM:
      return (op_a().size() == Size::k8) ? 0x80 : 0x81;
    case OpEncoding::kRM_IMM8:
      return 0x83;
    case OpEncoding::kRM_REG:
      return (op_a().size() == Size::k8) ? 0x10 : 0x11;
    case OpEncoding::kREG_RM:
      return (op_a().size() == Size::k8) ? 0x12 : 0x13;
  }
}

uint8_t Adc::OpcodeExt() const { return 2; }

std::string Adc::ToString() const { return "adc " + op_a().ToString() + "," + op_b().ToString(); }

uint8_t Sub::Opcode() const {
  if (CanUseRegAShortcut()) {
    return (op_a().size() == Size::k8) ? 0x2c : 0x2d;
  }
  switch (op_encoding()) {
    case OpEncoding::kRM_IMM:
      return (op_a().size() == Size::k8) ? 0x80 : 0x81;
    case OpEncoding::kRM_IMM8:
      return 0x83;
    case OpEncoding::kRM_REG:
      return (op_a().size() == Size::k8) ? 0x28 : 0x29;
    case OpEncoding::kREG_RM:
      return (op_a().size() == Size::k8) ? 0x2a : 0x2b;
  }
}

uint8_t Sub::OpcodeExt() const { return 5; }

std::string Sub::ToString() const { return "sub " + op_a().ToString() + "," + op_b().ToString(); }

uint8_t Sbb::Opcode() const {
  if (CanUseRegAShortcut()) {
    return (op_a().size() == Size::k8) ? 0x1c : 0x1d;
  }
  switch (op_encoding()) {
    case OpEncoding::kRM_IMM:
      return (op_a().size() == Size::k8) ? 0x80 : 0x81;
    case OpEncoding::kRM_IMM8:
      return 0x83;
    case OpEncoding::kRM_REG:
      return (op_a().size() == Size::k8) ? 0x18 : 0x19;
    case OpEncoding::kREG_RM:
      return (op_a().size() == Size::k8) ? 0x1a : 0x1b;
  }
}

uint8_t Sbb::OpcodeExt() const { return 3; }

std::string Sbb::ToString() const { return "sbb " + op_a().ToString() + "," + op_b().ToString(); }

uint8_t Cmp::Opcode() const {
  if (CanUseRegAShortcut()) {
    return (op_a().size() == Size::k8) ? 0x3c : 0x3d;
  }
  switch (op_encoding()) {
    case OpEncoding::kRM_IMM:
      return (op_a().size() == Size::k8) ? 0x80 : 0x81;
    case OpEncoding::kRM_IMM8:
      return 0x83;
    case OpEncoding::kRM_REG:
      return (op_a().size() == Size::k8) ? 0x38 : 0x39;
    case OpEncoding::kREG_RM:
      return (op_a().size() == Size::k8) ? 0x3a : 0x3b;
  }
}

uint8_t Cmp::OpcodeExt() const { return 7; }

std::string Cmp::ToString() const { return "cmp " + op_a().ToString() + "," + op_b().ToString(); }

int8_t Mul::Encode(Linker&, DataView code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(factor_.size());
  if (factor_.RequiresREX()) {
    encoder.EncodeREX();
  }
  encoder.EncodeOpcode((factor_.size() == Size::k8) ? 0xf6 : 0xf7);
  encoder.EncodeOpcodeExt(4);
  encoder.EncodeRM(factor_);

  return encoder.size();
}

std::string Mul::ToString() const { return "mul " + factor_.ToString(); }

Imul::Imul(RM rm) : factor_a_(Size::k8, 0), factor_b_(rm), factor_c_(0) {
  imul_type_ = ImulType::kRegAD_RM;
}

Imul::Imul(Reg reg, RM rm) : factor_a_(reg), factor_b_(rm), factor_c_(0) {
  if (reg.size() != rm.size()) fail("unsupported reg size, rm size combination");
  if (reg.size() == Size::k8) fail("unsupported reg or rm size");

  imul_type_ = ImulType::kReg_RM;
}

Imul::Imul(Reg reg, RM rm, Imm imm) : factor_a_(reg), factor_b_(rm), factor_c_(imm) {
  if (reg.size() != rm.size()) fail("unsupported reg size, rm size combination");
  if (reg.size() == Size::k8) fail("unsupported reg and rm size");
  if (imm.size() == Size::k64) fail("unsupported imm size");
  if (reg.size() == imm.size() || (reg.size() == Size::k64 && imm.size() == Size::k32)) {
    imul_type_ = ImulType::kReg_RM_IMM;
  } else if (imm.size() == Size::k8) {
    imul_type_ = ImulType::kReg_RM_IMM8;
  } else {
    fail("unsupported reg size, rm size, imm size combination");
  }
}

bool Imul::CanSkipImm() const {
  if (imul_type_ != ImulType::kReg_RM_IMM && imul_type_ != ImulType::kReg_RM_IMM8) {
    return true;
  }
  return factor_c_.value() == 1;
}

int8_t Imul::Encode(Linker&, DataView code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(factor_b_.size());
  if ((imul_type_ != ImulType::kRegAD_RM && factor_a_.RequiresREX()) || factor_b_.RequiresREX() ||
      (!CanSkipImm() && factor_c_.RequiresREX())) {
    encoder.EncodeREX();
  }

  if (imul_type_ == ImulType::kRegAD_RM) {
    encoder.EncodeOpcode((factor_b_.size() == Size::k8) ? 0xf6 : 0xf7);
    encoder.EncodeOpcodeExt(5);
    encoder.EncodeRM(factor_b_);
  } else if (imul_type_ == ImulType::kReg_RM || imul_type_ == ImulType::kReg_RM_IMM ||
             imul_type_ == ImulType::kReg_RM_IMM8) {
    if (CanSkipImm()) {
      encoder.EncodeOpcode(0x0f, 0xaf);
    } else if (imul_type_ == ImulType::kReg_RM_IMM) {
      encoder.EncodeOpcode(0x69);
    } else if (imul_type_ == ImulType::kReg_RM_IMM8) {
      encoder.EncodeOpcode(0x6b);
    }
    encoder.EncodeModRMReg(factor_a_);
    encoder.EncodeRM(factor_b_);
    if (!CanSkipImm()) {
      encoder.EncodeImm(factor_c_);
    }
  }

  return encoder.size();
}

std::string Imul::ToString() const {
  if (imul_type_ == ImulType::kRegAD_RM) {
    return "imul " + factor_b_.ToString();
  } else if (CanSkipImm()) {
    return "imul " + factor_a_.ToString() + "," + factor_b_.ToString();
  } else {
    return "imul " + factor_a_.ToString() + "," + factor_b_.ToString() + "," + factor_c_.ToString();
  }
}

int8_t Div::Encode(Linker&, DataView code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(divisor_.size());
  if (divisor_.RequiresREX()) {
    encoder.EncodeREX();
  }
  encoder.EncodeOpcode((divisor_.size() == Size::k8) ? 0xf6 : 0xf7);
  encoder.EncodeOpcodeExt(6);
  encoder.EncodeRM(divisor_);

  return encoder.size();
}

std::string Div::ToString() const { return "div " + divisor_.ToString(); }

int8_t Idiv::Encode(Linker&, DataView code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(divisor_.size());
  if (divisor_.RequiresREX()) {
    encoder.EncodeREX();
  }
  encoder.EncodeOpcode((divisor_.size() == Size::k8) ? 0xf6 : 0xf7);
  encoder.EncodeOpcodeExt(7);
  encoder.EncodeRM(divisor_);

  return encoder.size();
}

std::string Idiv::ToString() const { return "idiv " + divisor_.ToString(); }

SignExtendRegA::SignExtendRegA(Size op_size) : op_size_(op_size) {
  if (op_size != Size::k16 && op_size != Size::k32 && op_size != Size::k64) {
    fail("expected op_size 16, 32, or 64");
  }
}

int8_t SignExtendRegA::Encode(Linker&, DataView code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(op_size_);
  encoder.EncodeOpcode(0x98);

  return encoder.size();
}

std::string SignExtendRegA::ToString() const {
  if (op_size_ == Size::k16) {
    return "cbw";
  } else if (op_size_ == Size::k32) {
    return "cwde";
  } else {
    return "cdqe";
  }
}
SignExtendRegAD::SignExtendRegAD(Size op_size) : op_size_(op_size) {
  if (op_size != Size::k16 && op_size != Size::k32 && op_size != Size::k64) {
    fail("expected op_size 16, 32, or 64");
  }
}

int8_t SignExtendRegAD::Encode(Linker&, DataView code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(op_size_);
  encoder.EncodeOpcode(0x99);

  return encoder.size();
}

std::string SignExtendRegAD::ToString() const {
  if (op_size_ == Size::k16) {
    return "cwd";
  } else if (op_size_ == Size::k32) {
    return "cdq";
  } else {
    return "cqo";
  }
}

Test::Test(RM rm, Imm imm) : op_a_(rm), op_b_(imm) {
  if (imm.size() == Size::k64) fail("unsupported imm size");
  if (rm.size() == imm.size() || (rm.size() == Size::k64 && imm.size() == Size::k32)) {
    test_type_ = TestType::kRM_IMM;
  } else {
    fail("unsupported rm size, imm size combination");
  }
}

Test::Test(RM rm, Reg reg) : op_a_(rm), op_b_(reg) {
  if (rm.size() != reg.size()) fail("unsupported rm size, reg size combination");

  test_type_ = TestType::kRM_REG;
}

bool Test::CanUseRegAShortcut() const {
  if (test_type_ != TestType::kRM_IMM) return false;
  if (!op_a_.is_reg()) return false;
  return op_a_.reg().reg() == 0;
}

int8_t Test::Encode(Linker&, DataView code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(op_a_.size());
  if (op_a_.RequiresREX() || op_b_.RequiresREX()) {
    encoder.EncodeREX();
  }

  if (CanUseRegAShortcut()) {
    encoder.EncodeOpcode((op_a_.size() == Size::k8) ? 0xa8 : 0xa9);
  } else if (test_type_ == TestType::kRM_IMM) {
    encoder.EncodeOpcode((op_a_.size() == Size::k8) ? 0xf6 : 0xf7);
    encoder.EncodeOpcodeExt(0);
  } else if (test_type_ == TestType::kRM_REG) {
    encoder.EncodeOpcode((op_a_.size() == Size::k8) ? 0x84 : 0x85);
  }

  if (!CanUseRegAShortcut()) {
    encoder.EncodeRM(op_a_);
  }
  if (test_type_ == TestType::kRM_IMM) {
    encoder.EncodeImm(op_b_.imm());

  } else if (test_type_ == TestType::kRM_REG) {
    encoder.EncodeModRMReg(op_b_.reg());
  }

  return encoder.size();
}

std::string Test::ToString() const { return "test " + op_a_.ToString() + "," + op_b_.ToString(); }

}  // namespace x86_64
