//
//  data_instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "data_instrs.h"

#include "x86_64/coding/instr_encoder.h"

namespace x64 {

Mov::Mov(std::shared_ptr<Reg8>  d,
         std::shared_ptr<Reg8>   s)
    : mov_type_(MovType::kRM_REG),
      op_size_(8),  dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg16>  d,
         std::shared_ptr<Reg16>  s)
    : mov_type_(MovType::kRM_REG),
      op_size_(16), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg32>  d,
         std::shared_ptr<Reg32>  s)
    : mov_type_(MovType::kRM_REG),
      op_size_(32), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg64>  d,
         std::shared_ptr<Reg64>  s)
    : mov_type_(MovType::kRM_REG),
      op_size_(64), dst_(d), src_(s) {}

Mov::Mov(std::shared_ptr<Mem8>  d,
         std::shared_ptr<Reg8>   s)
    : mov_type_(MovType::kRM_REG),
      op_size_(8),  dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Mem16>  d,
         std::shared_ptr<Reg16>  s)
    : mov_type_(MovType::kRM_REG),
      op_size_(16), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Mem32>  d,
         std::shared_ptr<Reg32>  s)
    : mov_type_(MovType::kRM_REG),
      op_size_(32), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Mem64>  d,
         std::shared_ptr<Reg64>  s)
    : mov_type_(MovType::kRM_REG),
      op_size_(64), dst_(d), src_(s) {}

Mov::Mov(std::shared_ptr<Reg8>  d,
         std::shared_ptr<Mem8>   s)
    : mov_type_(MovType::kREG_RM),
      op_size_(8),  dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg16>  d,
         std::shared_ptr<Mem16>  s)
    : mov_type_(MovType::kREG_RM),
      op_size_(16), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg32>  d,
         std::shared_ptr<Mem32>  s)
    : mov_type_(MovType::kREG_RM),
      op_size_(32), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg64>  d,
         std::shared_ptr<Mem64>  s)
    : mov_type_(MovType::kREG_RM),
      op_size_(64), dst_(d), src_(s) {}

Mov::Mov(std::shared_ptr<Reg8>  d,
         std::shared_ptr<Imm8>  s)
    : mov_type_(MovType::kREG_IMM),
      op_size_(8),  dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg16>  d,
         std::shared_ptr<Imm16> s)
    : mov_type_(MovType::kREG_IMM),
      op_size_(16), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg32>  d,
         std::shared_ptr<Imm32> s)
    : mov_type_(MovType::kREG_IMM),
      op_size_(32), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg64>  d,
         std::shared_ptr<Imm32> s)
    : mov_type_(MovType::kRM_IMM),
      op_size_(64), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Reg64>  d,
         std::shared_ptr<Imm64> s)
    : mov_type_(MovType::kREG_IMM),
      op_size_(64), dst_(d), src_(s) {}

Mov::Mov(std::shared_ptr<Mem8>  d,
         std::shared_ptr<Imm8>  s)
    : mov_type_(MovType::kRM_IMM),
      op_size_(8),  dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Mem16>  d,
         std::shared_ptr<Imm16> s)
    : mov_type_(MovType::kRM_IMM),
      op_size_(16), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Mem32>  d,
         std::shared_ptr<Imm32> s)
    : mov_type_(MovType::kRM_IMM),
      op_size_(32), dst_(d), src_(s) {}
Mov::Mov(std::shared_ptr<Mem64>  d,
         std::shared_ptr<Imm32> s)
    : mov_type_(MovType::kRM_IMM),
      op_size_(64), dst_(d), src_(s) {}
Mov::~Mov() {}

int8_t Mov::Encode(Linker *linker,
                   common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    if (dst_->RequiresREX() || src_->RequiresREX()) {
        encoder.EncodeREX();
    }
    
    if (mov_type_ == kRM_REG) {
        encoder.EncodeOpcode((op_size_ == 8) ? 0x88 : 0x89);
    } else if (mov_type_ == kREG_RM) {
        encoder.EncodeOpcode((op_size_ == 8) ? 0x8A : 0x8B);
    } else if (mov_type_ == kREG_IMM) {
        encoder.EncodeOpcode((op_size_ == 8) ? 0xB0 : 0xB8);
    } else if (mov_type_ == kRM_IMM) {
        encoder.EncodeOpcode((op_size_ == 8) ? 0xC6 : 0xC7);
        encoder.EncodeOpcodeExt(0);
    }
    
    if (mov_type_ == kRM_REG || mov_type_ == kRM_IMM) {
        RM *drm = dynamic_cast<RM *>(dst_.get());
        encoder.EncodeRM(drm);
        
    } else if (mov_type_ == kREG_RM) {
        Reg *dreg = dynamic_cast<Reg *>(dst_.get());
        encoder.EncodeModRMReg(dreg);
        
    } else if (mov_type_ == kREG_IMM) {
        Reg *dreg = dynamic_cast<Reg *>(dst_.get());
        encoder.EncodeOpcodeReg(dreg);
    }
    
    if (mov_type_ == kRM_REG) {
        Reg *sreg = dynamic_cast<Reg *>(src_.get());
        encoder.EncodeModRMReg(sreg);
        
    } else if (mov_type_ == kREG_RM) {
        RM *srm = dynamic_cast<RM *>(src_.get());
        encoder.EncodeRM(srm);
        
    } else if (mov_type_ == kREG_IMM || mov_type_ == kRM_IMM) {
        Imm *simm = dynamic_cast<Imm *>(src_.get());
        encoder.EncodeImm(simm);
    }
    
    return encoder.size();
}

std::string Mov::ToString() const {
    return "mov " + dst_->ToString() + ","
                  + src_->ToString();
}

Xchg::Xchg(std::shared_ptr<RM8> rm,
           std::shared_ptr<Reg8> reg)
    : op_size_(8), op_a_(rm), op_b_(reg) {}
Xchg::Xchg(std::shared_ptr<RM16> rm,
           std::shared_ptr<Reg16> reg)
    : op_size_(16), op_a_(rm), op_b_(reg) {}
Xchg::Xchg(std::shared_ptr<RM32> rm,
           std::shared_ptr<Reg32> reg)
    : op_size_(32), op_a_(rm), op_b_(reg) {}
Xchg::Xchg(std::shared_ptr<RM64> rm,
           std::shared_ptr<Reg64> reg)
    : op_size_(64), op_a_(rm), op_b_(reg) {}
Xchg::~Xchg() {}

bool Xchg::CanUseRegAShortcut() const {
    if (op_size_ == 8) return false;
    if (op_b_->reg() == 0) return true;
    if (Reg *reg = dynamic_cast<Reg *>(op_a_.get())) {
        return reg->reg() == 0;
    }
    return false;
}

int8_t Xchg::Encode(Linker *linker,
                    common::data code) const {
    coding::InstrEncoder encoder(code);
    
    encoder.EncodeOperandSize(op_size_);
    if (op_a_->RequiresREX() || op_b_->RequiresREX()) {
        encoder.EncodeREX();
    }
    
    if (CanUseRegAShortcut()) {
        Reg *reg = dynamic_cast<Reg *>(op_a_.get());
        
        uint8_t r = (reg->reg() + op_b_->reg()) & 0x07;
        
        encoder.EncodeOpcode(0x90 + r);
    } else {
        encoder.EncodeOpcode((op_size_ == 8) ? 0x86 : 0x87);
        encoder.EncodeRM(op_a_.get());
        encoder.EncodeModRMReg(op_b_.get());
    }
    
    return encoder.size();
}

std::string Xchg::ToString() const {
    return "xchg " + op_a_->ToString() + ","
                   + op_b_->ToString();
}

Push::Push(std::shared_ptr<Mem16>  mem)
    : op_size_(16), op_(mem) {}
Push::Push(std::shared_ptr<Mem64>  mem)
    : op_size_(64), op_(mem) {}
Push::Push(std::shared_ptr<Reg16>  reg)
    : op_size_(16), op_(reg) {}
Push::Push(std::shared_ptr<Reg64>  reg)
    : op_size_(64), op_(reg) {}
Push::Push(std::shared_ptr<Imm8>  imm)
    : op_size_(8),  op_(imm) {}
Push::Push(std::shared_ptr<Imm16> imm)
    : op_size_(16), op_(imm) {}
Push::Push(std::shared_ptr<Imm32> imm)
    : op_size_(32), op_(imm) {}
Push::~Push() {}

int8_t Push::Encode(Linker *linker,
                    common::data code) const {
    coding::InstrEncoder encoder(code);
    
    if (op_size_ != 64) {
        encoder.EncodeOperandSize(op_size_);
    }
    if (op_->RequiresREX()) {
        encoder.EncodeREX();
    }
    if (Reg *reg = dynamic_cast<Reg *>(op_.get())) {
        encoder.EncodeOpcode(0x50);
        encoder.EncodeOpcodeReg(reg);
        
    } else if (Mem *mem = dynamic_cast<Mem *>(op_.get())) {
        encoder.EncodeOpcode(0xff);
        encoder.EncodeOpcodeExt(6);
        encoder.EncodeRM(mem);
        
    } else if (Imm *imm = dynamic_cast<Imm *>(op_.get())) {
        encoder.EncodeOpcode((op_size_ == 8) ? 0x6a : 0x68);
        encoder.EncodeImm(imm);
    }
    
    return encoder.size();
}

std::string Push::ToString() const {
    return "push " + op_->ToString();
}

Pop::Pop(std::shared_ptr<Mem16>  mem)
    : op_size_(16), op_(mem) {}
Pop::Pop(std::shared_ptr<Mem64>  mem)
    : op_size_(64), op_(mem) {}
Pop::Pop(std::shared_ptr<Reg16>  reg)
    : op_size_(16), op_(reg) {}
Pop::Pop(std::shared_ptr<Reg64>  reg)
    : op_size_(64), op_(reg) {}
Pop::~Pop() {}

int8_t Pop::Encode(Linker *linker,
                   common::data code) const {
    coding::InstrEncoder encoder(code);
    
    if (op_size_ != 64) {
        encoder.EncodeOperandSize(op_size_);
    }
    if (op_->RequiresREX()) {
        encoder.EncodeREX();
    }
    if (Reg *reg = dynamic_cast<Reg *>(op_.get())) {
        encoder.EncodeOpcode(0x58);
        encoder.EncodeOpcodeReg(reg);
        
    } else if (Mem *mem = dynamic_cast<Mem *>(op_.get())) {
        encoder.EncodeOpcode(0x8f);
        encoder.EncodeOpcodeExt(0);
        encoder.EncodeRM(mem);
    }
    
    return encoder.size();
}

std::string Pop::ToString() const {
    return "pop " + op_->ToString();
}

}
