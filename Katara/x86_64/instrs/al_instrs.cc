//
//  al_instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "al_instrs.h"

#include "x86_64/coding/instr_encoder.h"

namespace x64 {

UnaryALInstr::UnaryALInstr(std::shared_ptr<RM8> rm)
    : op_size_(8), op_(rm) {}
UnaryALInstr::UnaryALInstr(std::shared_ptr<RM16> rm)
    : op_size_(16), op_(rm) {}
UnaryALInstr::UnaryALInstr(std::shared_ptr<RM32> rm)
    : op_size_(32), op_(rm) {}
UnaryALInstr::UnaryALInstr(std::shared_ptr<RM64> rm)
    : op_size_(64), op_(rm) {}

uint8_t UnaryALInstr::op_size() const {
    return op_size_;
}

std::shared_ptr<RM> UnaryALInstr::op() const {
    return op_;
}

int8_t UnaryALInstr::Encode(Linker *linker,
                            common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    if (op_->RequiresREX()) {
        encoder.EncodeREX();
    }
    
    encoder.EncodeOpcode(Opcode());
    encoder.EncodeOpcodeExt(OpcodeExt());
    encoder.EncodeRM(op_.get());
    
    return encoder.size();
}

BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem8> mem,
                             std::shared_ptr<Imm8> imm)
    : op_encoding_(OpEncoding::kRM_IMM),
      op_size_(8), op_a_(mem), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem16> mem,
                             std::shared_ptr<Imm16> imm)
    : op_encoding_(OpEncoding::kRM_IMM),
      op_size_(16), op_a_(mem), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem32> mem,
                             std::shared_ptr<Imm32> imm)
    : op_encoding_(OpEncoding::kRM_IMM),
      op_size_(32), op_a_(mem), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem64> mem,
                             std::shared_ptr<Imm32> imm)
    : op_encoding_(OpEncoding::kRM_IMM),
      op_size_(64), op_a_(mem), op_b_(imm) {}

BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem16> mem,
                             std::shared_ptr<Imm8> imm)
    : op_encoding_(OpEncoding::kRM_IMM8),
      op_size_(16), op_a_(mem), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem32> mem,
                             std::shared_ptr<Imm8> imm)
    : op_encoding_(OpEncoding::kRM_IMM8),
      op_size_(32), op_a_(mem), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem64> mem,
                             std::shared_ptr<Imm8> imm)
    : op_encoding_(OpEncoding::kRM_IMM8),
      op_size_(64), op_a_(mem), op_b_(imm) {}

BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg8> reg,
                             std::shared_ptr<Imm8> imm)
    : op_encoding_(OpEncoding::kRM_IMM),
      op_size_(8),  op_a_(reg), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg16> reg,
                             std::shared_ptr<Imm16> imm)
    : op_encoding_(OpEncoding::kRM_IMM),
      op_size_(16), op_a_(reg), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg32> reg,
                             std::shared_ptr<Imm32> imm)
    : op_encoding_(OpEncoding::kRM_IMM),
      op_size_(32), op_a_(reg), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg64> reg,
                             std::shared_ptr<Imm32> imm)
    : op_encoding_(OpEncoding::kRM_IMM),
      op_size_(64), op_a_(reg), op_b_(imm) {}

BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg16> reg,
                             std::shared_ptr<Imm8> imm)
    : op_encoding_(OpEncoding::kRM_IMM8),
      op_size_(16), op_a_(reg), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg32> reg,
                             std::shared_ptr<Imm8> imm)
    : op_encoding_(OpEncoding::kRM_IMM8),
      op_size_(32), op_a_(reg), op_b_(imm) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg64> reg,
                             std::shared_ptr<Imm8> imm)
    : op_encoding_(OpEncoding::kRM_IMM8),
      op_size_(64), op_a_(reg), op_b_(imm) {}

BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem8> mem,
                             std::shared_ptr<Reg8> reg)
    : op_encoding_(OpEncoding::kRM_REG),
      op_size_(8),  op_a_(mem), op_b_(reg) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem16> mem,
                             std::shared_ptr<Reg16> reg)
    : op_encoding_(OpEncoding::kRM_REG),
      op_size_(16), op_a_(mem), op_b_(reg) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem32> mem,
                             std::shared_ptr<Reg32> reg)
    : op_encoding_(OpEncoding::kRM_REG),
      op_size_(32), op_a_(mem), op_b_(reg) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Mem64> mem,
                             std::shared_ptr<Reg64> reg)
    : op_encoding_(OpEncoding::kRM_REG),
      op_size_(64), op_a_(mem), op_b_(reg) {}

BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg8> regA,
                             std::shared_ptr<Reg8> regB)
    : op_encoding_(OpEncoding::kRM_REG),
      op_size_(8),  op_a_(regA), op_b_(regB) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg16>  regA,
                             std::shared_ptr<Reg16>  regB)
    : op_encoding_(OpEncoding::kRM_REG),
      op_size_(16), op_a_(regA), op_b_(regB) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg32>  regA,
                             std::shared_ptr<Reg32>  regB)
    : op_encoding_(OpEncoding::kRM_REG),
      op_size_(32), op_a_(regA), op_b_(regB) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg64>  regA,
                             std::shared_ptr<Reg64>  regB)
    : op_encoding_(OpEncoding::kRM_REG),
      op_size_(64), op_a_(regA), op_b_(regB) {}

BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg8>   reg,
                             std::shared_ptr<Mem8>   mem)
    : op_encoding_(OpEncoding::kREG_RM),
      op_size_(8),  op_a_(reg), op_b_(mem) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg16>  reg,
                             std::shared_ptr<Mem16>  mem)
    : op_encoding_(OpEncoding::kREG_RM),
      op_size_(16), op_a_(reg), op_b_(mem) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg32>  reg,
                             std::shared_ptr<Mem32>  mem)
    : op_encoding_(OpEncoding::kREG_RM),
      op_size_(32), op_a_(reg), op_b_(mem) {}
BinaryALInstr::BinaryALInstr(std::shared_ptr<Reg64>  reg,
                             std::shared_ptr<Mem64>  mem)
    : op_encoding_(OpEncoding::kREG_RM),
      op_size_(64), op_a_(reg), op_b_(mem) {}

BinaryALInstr::OpEncoding
BinaryALInstr::op_encoding() const {
    return op_encoding_;
}

uint8_t BinaryALInstr::op_size() const {
    return op_size_;
}

std::shared_ptr<Operand> BinaryALInstr::op_a() const {
    return op_a_;
}

std::shared_ptr<Operand> BinaryALInstr::op_b() const {
    return op_b_;
}

bool BinaryALInstr::CanUseRegAShortcut() const {
    if (op_encoding_ != OpEncoding::kRM_IMM) return false;
    if (Reg *reg = dynamic_cast<Reg *>(op_a_.get())) {
        return reg->reg() == 0;
    } else {
        return false;
    }
}

int8_t BinaryALInstr::Encode(Linker *linker,
                             common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    if (op_a_->RequiresREX() || op_b_->RequiresREX()) {
        encoder.EncodeREX();
    }
    
    encoder.EncodeOpcode(Opcode());
    if (!CanUseRegAShortcut() &&
        (op_encoding_ == OpEncoding::kRM_IMM ||
         op_encoding_ == OpEncoding::kRM_IMM8)) {
        encoder.EncodeOpcodeExt(OpcodeExt());
    }
    
    if (CanUseRegAShortcut()) {
    } else if (op_encoding_ == OpEncoding::kRM_IMM ||
               op_encoding_ == OpEncoding::kRM_IMM8 ||
               op_encoding_ == OpEncoding::kRM_REG) {
        RM *rm = dynamic_cast<RM *>(op_a_.get());
        encoder.EncodeRM(rm);
        
    } else if (op_encoding_ == OpEncoding::kREG_RM) {
        Reg *reg = dynamic_cast<Reg *>(op_a_.get());
        encoder.EncodeModRMReg(reg);
    }
    
    if (op_encoding_ == OpEncoding::kRM_IMM ||
        op_encoding_ == OpEncoding::kRM_IMM8) {
        Imm *imm = dynamic_cast<Imm *>(op_b_.get());
        encoder.EncodeImm(imm);
        
    } else if (op_encoding_ == OpEncoding::kRM_REG) {
        Reg *reg = dynamic_cast<Reg *>(op_b_.get());
        encoder.EncodeModRMReg(reg);
        
    } else if (op_encoding_ == OpEncoding::kREG_RM) {
        RM *rm = dynamic_cast<RM *>(op_b_.get());
        encoder.EncodeRM(rm);
    }
    
    return encoder.size();
}

Not::~Not() {}

uint8_t Not::Opcode() const {
    return (op_size() == 8) ? 0xf6 : 0xf7;
}

uint8_t Not::OpcodeExt() const {
    return 2;
}

std::string Not::ToString() const {
    return "not " + op()->ToString();
}

And::~And() {}

uint8_t And::Opcode() const {
    if (CanUseRegAShortcut()) {
        return (op_size() == 8) ? 0x24 : 0x25;
    }
    switch (op_encoding()) {
        case OpEncoding::kRM_IMM:
            return (op_size() == 8) ? 0x80 : 0x81;
        case OpEncoding::kRM_IMM8:
            return 0x83;
        case OpEncoding::kRM_REG:
            return (op_size() == 8) ? 0x20 : 0x21;
        case OpEncoding::kREG_RM:
            return (op_size() == 8) ? 0x22 : 0x23;
    }
}

uint8_t And::OpcodeExt() const {
    return 4;
}

std::string And::ToString() const {
    return "and " + op_a()->ToString() + ","
                  + op_b()->ToString();
}

Or::~Or() {}
uint8_t Or::Opcode() const {
    if (CanUseRegAShortcut()) {
        return (op_size() == 8) ? 0x0c : 0x0d;
    }
    switch (op_encoding()) {
        case OpEncoding::kRM_IMM:
            return (op_size() == 8) ? 0x80 : 0x81;
        case OpEncoding::kRM_IMM8:
            return 0x83;
        case OpEncoding::kRM_REG:
            return (op_size() == 8) ? 0x08 : 0x09;
        case OpEncoding::kREG_RM:
            return (op_size() == 8) ? 0x0a : 0x0b;
    }
}

uint8_t Or::OpcodeExt() const {
    return 1;
}

std::string Or::ToString() const {
    return "or " + op_a()->ToString() + ","
                 + op_b()->ToString();
}

Xor::~Xor() {}

uint8_t Xor::Opcode() const {
    if (CanUseRegAShortcut()) {
        return (op_size() == 8) ? 0x34 : 0x35;
    }
    switch (op_encoding()) {
        case OpEncoding::kRM_IMM:
            return (op_size() == 8) ? 0x80 : 0x81;
        case OpEncoding::kRM_IMM8:
            return 0x83;
        case OpEncoding::kRM_REG:
            return (op_size() == 8) ? 0x30 : 0x31;
        case OpEncoding::kREG_RM:
            return (op_size() == 8) ? 0x32 : 0x33;
    }
}

uint8_t Xor::OpcodeExt() const {
    return 6;
}

std::string Xor::ToString() const {
    return "xor " + op_a()->ToString() + ","
                  + op_b()->ToString();
}

Neg::~Neg() {}

uint8_t Neg::Opcode() const {
    return (op_size() == 8) ? 0xf6 : 0xf7;
}

uint8_t Neg::OpcodeExt() const {
    return 3;
}

std::string Neg::ToString() const {
    return "neg " + op()->ToString();
}

Add::~Add() {}

uint8_t Add::Opcode() const {
    if (CanUseRegAShortcut()) {
        return (op_size() == 8) ? 0x04 : 0x05;
    }
    switch (op_encoding()) {
        case OpEncoding::kRM_IMM:
            return (op_size() == 8) ? 0x80 : 0x81;
        case OpEncoding::kRM_IMM8:
            return 0x83;
        case OpEncoding::kRM_REG:
            return (op_size() == 8) ? 0x00 : 0x01;
        case OpEncoding::kREG_RM:
            return (op_size() == 8) ? 0x02 : 0x03;
    }
}

uint8_t Add::OpcodeExt() const {
    return 0;
}

std::string Add::ToString() const {
    return "add " + op_a()->ToString() + ","
                  + op_b()->ToString();
}

Adc::~Adc() {}

uint8_t Adc::Opcode() const {
    if (CanUseRegAShortcut()) {
        return (op_size() == 8) ? 0x14 : 0x15;
    }
    switch (op_encoding()) {
        case OpEncoding::kRM_IMM:
            return (op_size() == 8) ? 0x80 : 0x81;
        case OpEncoding::kRM_IMM8:
            return 0x83;
        case OpEncoding::kRM_REG:
            return (op_size() == 8) ? 0x10 : 0x11;
        case OpEncoding::kREG_RM:
            return (op_size() == 8) ? 0x12 : 0x13;
    }
}

uint8_t Adc::OpcodeExt() const {
    return 2;
}

std::string Adc::ToString() const {
    return "adc " + op_a()->ToString() + ","
                  + op_b()->ToString();
}

Sub::~Sub() {}

uint8_t Sub::Opcode() const {
    if (CanUseRegAShortcut()) {
        return (op_size() == 8) ? 0x2c : 0x2d;
    }
    switch (op_encoding()) {
        case OpEncoding::kRM_IMM:
            return (op_size() == 8) ? 0x80 : 0x81;
        case OpEncoding::kRM_IMM8:
            return 0x83;
        case OpEncoding::kRM_REG:
            return (op_size() == 8) ? 0x28 : 0x29;
        case OpEncoding::kREG_RM:
            return (op_size() == 8) ? 0x2a : 0x2b;
    }
}

uint8_t Sub::OpcodeExt() const {
    return 5;
}

std::string Sub::ToString() const {
    return "sub " + op_a()->ToString() + ","
                  + op_b()->ToString();
}

Sbb::~Sbb() {}

uint8_t Sbb::Opcode() const {
    if (CanUseRegAShortcut()) {
        return (op_size() == 8) ? 0x1c : 0x1d;
    }
    switch (op_encoding()) {
        case OpEncoding::kRM_IMM:
            return (op_size() == 8) ? 0x80 : 0x81;
        case OpEncoding::kRM_IMM8:
            return 0x83;
        case OpEncoding::kRM_REG:
            return (op_size() == 8) ? 0x18 : 0x19;
        case OpEncoding::kREG_RM:
            return (op_size() == 8) ? 0x1a : 0x1b;
    }
}

uint8_t Sbb::OpcodeExt() const {
    return 3;
}

std::string Sbb::ToString() const {
    return "sbb " + op_a()->ToString() + ","
                  + op_b()->ToString();
}

Cmp::~Cmp() {}

uint8_t Cmp::Opcode() const {
    if (CanUseRegAShortcut()) {
        return (op_size() == 8) ? 0x3c : 0x3d;
    }
    switch (op_encoding()) {
        case OpEncoding::kRM_IMM:
            return (op_size() == 8) ? 0x80 : 0x81;
        case OpEncoding::kRM_IMM8:
            return 0x83;
        case OpEncoding::kRM_REG:
            return (op_size() == 8) ? 0x38 : 0x39;
        case OpEncoding::kREG_RM:
            return (op_size() == 8) ? 0x3a : 0x3b;
    }
}

uint8_t Cmp::OpcodeExt() const {
    return 7;
}

std::string Cmp::ToString() const {
    return "cmp " + op_a()->ToString() + ","
                  + op_b()->ToString();
}

Mul::Mul(std::shared_ptr<RM8> rm)
    : op_size_(8), factor_(rm) {}
Mul::Mul(std::shared_ptr<RM16> rm)
    : op_size_(16), factor_(rm) {}
Mul::Mul(std::shared_ptr<RM32> rm)
    : op_size_(32), factor_(rm) {}
Mul::Mul(std::shared_ptr<RM64> rm)
    : op_size_(64), factor_(rm) {}
Mul::~Mul() {}

int8_t Mul::Encode(Linker *linker,
                   common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    if (factor_->RequiresREX()) {
        encoder.EncodeREX();
    }
    encoder.EncodeOpcode((op_size_ == 8) ? 0xf6 : 0xf7);
    encoder.EncodeOpcodeExt(4);
    encoder.EncodeRM(factor_.get());
    
    return encoder.size();
}

std::string Mul::ToString() const {
    return "mul " + factor_->ToString();
}

Imul::Imul(std::shared_ptr<RM8> rm)
    : imul_type_(ImulType::kRegAD_RM),
      op_size_(8),
      factor_a_(nullptr),
      factor_b_(rm),
      factor_c_(nullptr) {}
Imul::Imul(std::shared_ptr<RM16> rm)
    : imul_type_(ImulType::kRegAD_RM),
      op_size_(8),
      factor_a_(nullptr),
      factor_b_(rm),
      factor_c_(nullptr) {}
Imul::Imul(std::shared_ptr<RM32> rm)
    : imul_type_(ImulType::kRegAD_RM),
      op_size_(8),
      factor_a_(nullptr),
      factor_b_(rm),
      factor_c_(nullptr) {}
Imul::Imul(std::shared_ptr<RM64> rm)
    : imul_type_(ImulType::kRegAD_RM),
      op_size_(8),
      factor_a_(nullptr),
      factor_b_(rm),
      factor_c_(nullptr) {}

Imul::Imul(std::shared_ptr<Reg16> reg,
           std::shared_ptr<RM16> rm,
           std::shared_ptr<Imm8> imm)
    : imul_type_(ImulType::kReg_RM_IMM8),
      op_size_(16),
      factor_a_(reg),
      factor_b_(rm),
      factor_c_(imm) {}
Imul::Imul(std::shared_ptr<Reg32> reg,
           std::shared_ptr<RM32> rm,
           std::shared_ptr<Imm8> imm)
    : imul_type_(ImulType::kReg_RM_IMM8),
      op_size_(32),
      factor_a_(reg),
      factor_b_(rm),
      factor_c_(imm) {}
Imul::Imul(std::shared_ptr<Reg64> reg,
           std::shared_ptr<RM64> rm,
           std::shared_ptr<Imm8> imm)
    : imul_type_(ImulType::kReg_RM_IMM8),
      op_size_(64),
      factor_a_(reg),
      factor_b_(rm),
      factor_c_(imm) {}

Imul::Imul(std::shared_ptr<Reg16> reg,
           std::shared_ptr<RM16> rm,
           std::shared_ptr<Imm16> imm)
    : imul_type_(ImulType::kReg_RM_IMM),
      op_size_(16),
      factor_a_(reg),
      factor_b_(rm),
      factor_c_(imm) {}
Imul::Imul(std::shared_ptr<Reg32> reg,
           std::shared_ptr<RM32> rm,
           std::shared_ptr<Imm32> imm)
    : imul_type_(ImulType::kReg_RM_IMM),
      op_size_(32),
      factor_a_(reg),
      factor_b_(rm),
      factor_c_(imm) {}
Imul::Imul(std::shared_ptr<Reg64> reg,
           std::shared_ptr<RM64> rm,
           std::shared_ptr<Imm32> imm)
    : imul_type_(ImulType::kReg_RM_IMM),
      op_size_(64),
      factor_a_(reg),
      factor_b_(rm),
      factor_c_(imm) {}
Imul::~Imul() {}

bool Imul::CanSkipImm() const {
    if (factor_c_.get() == nullptr) {
        return true;
    }
    if (Imm8 *imm = dynamic_cast<Imm8 *>(factor_c_.get())) {
        return imm->value() == 1;
    } else if (Imm16 *imm = dynamic_cast<Imm16 *>(factor_c_.get())) {
        return imm->value() == 1;
    } else if (Imm32 *imm = dynamic_cast<Imm32 *>(factor_c_.get())) {
        return imm->value() == 1;
    }
    return true;
}

int8_t Imul::Encode(Linker *linker,
                    common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    if ((factor_a_.get() != nullptr &&
         factor_a_->RequiresREX()) ||
        factor_b_->RequiresREX() ||
        (!CanSkipImm() &&
         factor_c_->RequiresREX())) {
        encoder.EncodeREX();
    }
    
    if (imul_type_ == ImulType::kRegAD_RM) {
        encoder.EncodeOpcode((op_size_ == 8) ? 0xf6 : 0xf7);
        encoder.EncodeOpcodeExt(5);
        encoder.EncodeRM(factor_b_.get());
    } else if (imul_type_ == ImulType::kReg_RM_IMM8 ||
               imul_type_ == ImulType::kReg_RM_IMM) {
        if (CanSkipImm()) {
            encoder.EncodeOpcode(0x0f, 0xaf);
        } else if (imul_type_ == ImulType::kReg_RM_IMM8) {
            encoder.EncodeOpcode(0x6b);
        } else if (imul_type_ == ImulType::kReg_RM_IMM) {
            encoder.EncodeOpcode(0x69);
        }
        encoder.EncodeModRMReg(factor_a_.get());
        encoder.EncodeRM(factor_b_.get());
        if (!CanSkipImm()) {
            encoder.EncodeImm(factor_c_.get());
        }
    }
    
    return encoder.size();
}

std::string Imul::ToString() const {
    if (imul_type_ == ImulType::kRegAD_RM) {
        return "imul " + factor_b_->ToString();
    } else if (CanSkipImm()) {
        return "imul " + factor_a_->ToString() + ","
                       + factor_b_->ToString();
    } else {
        return "imul " + factor_a_->ToString() + ","
                       + factor_b_->ToString() + ","
                       + factor_c_->ToString();
    }
}

Div::Div(std::shared_ptr<RM8> rm)
    : op_size_(8), factor_(rm) {}
Div::Div(std::shared_ptr<RM16> rm)
    : op_size_(16), factor_(rm) {}
Div::Div(std::shared_ptr<RM32> rm)
    : op_size_(32), factor_(rm) {}
Div::Div(std::shared_ptr<RM64> rm)
    : op_size_(64), factor_(rm) {}
Div::~Div() {}

int8_t Div::Encode(Linker *linker,
                   common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    if (factor_->RequiresREX()) {
        encoder.EncodeREX();
    }
    encoder.EncodeOpcode((op_size_ == 8) ? 0xf6 : 0xf7);
    encoder.EncodeOpcodeExt(6);
    encoder.EncodeRM(factor_.get());
    
    return encoder.size();
}

std::string Div::ToString() const {
    return "div " + factor_->ToString();
}

Idiv::Idiv(std::shared_ptr<RM8> rm)
    : op_size_(8), factor_(rm) {}
Idiv::Idiv(std::shared_ptr<RM16> rm)
    : op_size_(16), factor_(rm) {}
Idiv::Idiv(std::shared_ptr<RM32> rm)
    : op_size_(32), factor_(rm) {}
Idiv::Idiv(std::shared_ptr<RM64> rm)
    : op_size_(64), factor_(rm) {}
Idiv::~Idiv() {}

int8_t Idiv::Encode(Linker *linker,
                    common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    if (factor_->RequiresREX()) {
        encoder.EncodeREX();
    }
    encoder.EncodeOpcode((op_size_ == 8) ? 0xf6 : 0xf7);
    encoder.EncodeOpcodeExt(7);
    encoder.EncodeRM(factor_.get());
    
    return encoder.size();
}

std::string Idiv::ToString() const {
    return "idiv " + factor_->ToString();
}

SignExtendRegA::SignExtendRegA(uint8_t op_size) : op_size_(op_size) {
    if (op_size != 16 && op_size != 32 && op_size != 64) {
        throw "expected op_size 16, 32, or 64, got: "
            + std::to_string(op_size);
    }
}

SignExtendRegA::~SignExtendRegA() {}

int8_t SignExtendRegA::Encode(Linker *linker,
                              common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    encoder.EncodeOpcode(0x98);
    
    return encoder.size();
}

std::string SignExtendRegA::ToString() const {
    if (op_size_ == 16) {
        return "cbw";
    } else if (op_size_ == 32) {
        return "cwde";
    } else {
        return "cdqe";
    }
}
SignExtendRegAD::SignExtendRegAD(uint8_t op_size) : op_size_(op_size) {
    if (op_size != 16 && op_size != 32 && op_size != 64) {
        throw "expected op_size 16, 32, or 64, got: "
            + std::to_string(op_size);
    }
}

SignExtendRegAD::~SignExtendRegAD() {}

int8_t SignExtendRegAD::Encode(Linker *linker,
                               common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    encoder.EncodeOpcode(0x99);
    
    return encoder.size();
}

std::string SignExtendRegAD::ToString() const {
    if (op_size_ == 16) {
        return "cwd";
    } else if (op_size_ == 32) {
        return "cdq";
    } else {
        return "cqo";
    }
}

Test::Test(std::shared_ptr<RM8> rm,
           std::shared_ptr<Imm8> imm)
    : test_type_(TestType::kRM_IMM),
      op_size_(8), op_a_(rm), op_b_(imm) {}
Test::Test(std::shared_ptr<RM16> rm,
           std::shared_ptr<Imm16> imm)
    : test_type_(TestType::kRM_IMM),
      op_size_(16), op_a_(rm), op_b_(imm) {}
Test::Test(std::shared_ptr<RM32> rm,
           std::shared_ptr<Imm32> imm)
    : test_type_(TestType::kRM_IMM),
      op_size_(32), op_a_(rm), op_b_(imm) {}
Test::Test(std::shared_ptr<RM64> rm,
           std::shared_ptr<Imm32> imm)
    : test_type_(TestType::kRM_IMM),
      op_size_(64), op_a_(rm), op_b_(imm) {}

Test::Test(std::shared_ptr<RM8> rm,
           std::shared_ptr<Reg8> reg)
    : test_type_(TestType::kRM_REG),
      op_size_(8), op_a_(rm), op_b_(reg) {}
Test::Test(std::shared_ptr<RM16> rm,
           std::shared_ptr<Reg16> reg)
    : test_type_(TestType::kRM_REG),
      op_size_(16), op_a_(rm), op_b_(reg) {}
Test::Test(std::shared_ptr<RM32> rm,
           std::shared_ptr<Reg32> reg)
    : test_type_(TestType::kRM_REG),
      op_size_(32), op_a_(rm), op_b_(reg) {}
Test::Test(std::shared_ptr<RM64> rm,
           std::shared_ptr<Reg64> reg)
    : test_type_(TestType::kRM_REG),
      op_size_(64), op_a_(rm), op_b_(reg) {}

Test::~Test() {}

bool Test::CanUseRegAShortcut() const {
    if (test_type_ != TestType::kRM_IMM) return false;
    if (Reg *reg = dynamic_cast<Reg *>(op_a_.get())) {
        return reg->reg() == 0;
    }
    return false;
}

int8_t Test::Encode(Linker *linker,
                    common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    if (op_a_->RequiresREX() || op_b_->RequiresREX()) {
        encoder.EncodeREX();
    }
    
    if (CanUseRegAShortcut()) {
        encoder.EncodeOpcode((op_size_ == 8) ? 0xa8 : 0xa9);
    } else if (test_type_ == TestType::kRM_IMM) {
        encoder.EncodeOpcode((op_size_ == 8) ? 0xf6 : 0xf7);
        encoder.EncodeOpcodeExt(0);
    } else if (test_type_ == TestType::kRM_REG) {
        encoder.EncodeOpcode((op_size_ == 8) ? 0x84 : 0x85);
    }
    
    if (!CanUseRegAShortcut()) {
        encoder.EncodeRM(op_a_.get());
    }
    if (test_type_ == TestType::kRM_IMM) {
        Imm *imm = dynamic_cast<Imm *>(op_b_.get());
        
        encoder.EncodeImm(imm);
    } else if (test_type_ == TestType::kRM_REG) {
        Reg *reg = dynamic_cast<Reg *>(op_b_.get());
        
        encoder.EncodeModRMReg(reg);
    }
    
    return encoder.size();
}

std::string Test::ToString() const {
    return "test " + op_a_->ToString() + ","
                   + op_b_->ToString();
}

}
