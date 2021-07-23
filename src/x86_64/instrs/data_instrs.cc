//
//  data_instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "data_instrs.h"

#include "src/common/logging.h"
#include "src/x86_64/instrs/instr_encoder.h"

namespace x86_64 {

Mov::Mov(RM dst, Operand src) : dst_(dst), src_(src) {
  if (dst.is_reg()) {
    if (src.is_imm()) {
      if (dst.size() == src.size() || (dst.size() == Size::k64 && src.size() == Size::k32)) {
        mov_type_ = MovType::kREG_IMM;
      } else {
        common::fail("unsupported reg size, imm size combination");
      }

    } else if (src.is_reg() || src.is_mem()) {
      if (dst.size() != src.size()) common::fail("unsupported reg size, reg/mem size combination");
      mov_type_ = MovType::kREG_RM;

    } else {
      common::fail("unexpected src operand kind");
    }

  } else if (dst.is_mem()) {
    if (src.is_imm()) {
      if (src.size() == Size::k64) common::fail("unsupported imm size");
      if (dst.size() == src.size() || (dst.size() == Size::k64 && src.size() == Size::k32)) {
        mov_type_ = MovType::kRM_IMM;
      } else {
        common::fail("unsupported mem size, imm size combination");
      }

    } else if (src.is_reg()) {
      if (dst.size() != src.size()) common::fail("unsupported mem size, reg size combination");
      mov_type_ = MovType::kRM_REG;

    } else if (src.is_mem()) {
      common::fail("unsupported mov: mem to mem");

    } else {
      common::fail("unexpected src operand kind");
    }

  } else {
    common::fail("unexpected dst operand kind");
  }
}

int8_t Mov::Encode(Linker&, common::data code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(src_.size());
  if (dst_.RequiresREX() || src_.RequiresREX()) {
    encoder.EncodeREX();
  }

  if (mov_type_ == kRM_REG) {
    encoder.EncodeOpcode((src_.size() == Size::k8) ? 0x88 : 0x89);
  } else if (mov_type_ == kREG_RM) {
    encoder.EncodeOpcode((src_.size() == Size::k8) ? 0x8A : 0x8B);
  } else if (mov_type_ == kREG_IMM) {
    encoder.EncodeOpcode((src_.size() == Size::k8) ? 0xB0 : 0xB8);
  } else if (mov_type_ == kRM_IMM) {
    encoder.EncodeOpcode((src_.size() == Size::k8) ? 0xC6 : 0xC7);
    encoder.EncodeOpcodeExt(0);
  }

  if (mov_type_ == kRM_REG || mov_type_ == kRM_IMM) {
    encoder.EncodeRM(dst_.rm());

  } else if (mov_type_ == kREG_RM) {
    encoder.EncodeModRMReg(dst_.reg());

  } else if (mov_type_ == kREG_IMM) {
    encoder.EncodeOpcodeReg(dst_.reg());
  }

  if (mov_type_ == kRM_REG) {
    encoder.EncodeModRMReg(src_.reg());

  } else if (mov_type_ == kREG_RM) {
    encoder.EncodeRM(src_.rm());

  } else if (mov_type_ == kREG_IMM || mov_type_ == kRM_IMM) {
    encoder.EncodeImm(src_.imm());
  }

  return encoder.size();
}

std::string Mov::ToString() const { return "mov " + dst_.ToString() + "," + src_.ToString(); }

Xchg::Xchg(RM rm, Reg reg) : op_a_(rm), op_b_(reg) {
  if (rm.size() != reg.size()) common::fail("incompatible rm size, reg size combination");
}

bool Xchg::CanUseRegAShortcut() const {
  if (op_a_.size() == Size::k8) return false;
  if (!op_a_.is_reg()) return false;
  if (op_b_.reg() == 0) return true;
  return op_a_.reg().reg() == 0;
}

int8_t Xchg::Encode(Linker&, common::data code) const {
  InstrEncoder encoder(code);

  encoder.EncodeOperandSize(op_a_.size());
  if (op_a_.RequiresREX() || op_b_.RequiresREX()) {
    encoder.EncodeREX();
  }

  if (CanUseRegAShortcut()) {
    uint8_t r = (op_a_.reg().reg() + op_b_.reg()) & 0x07;

    encoder.EncodeOpcode(0x90 + r);
  } else {
    encoder.EncodeOpcode((op_a_.size() == Size::k8) ? 0x86 : 0x87);
    encoder.EncodeRM(op_a_);
    encoder.EncodeModRMReg(op_b_);
  }

  return encoder.size();
}

std::string Xchg::ToString() const { return "xchg " + op_a_.ToString() + "," + op_b_.ToString(); }

Push::Push(RM rm) : op_(rm) {
  if (rm.size() != Size::k16 && rm.size() != Size::k64) common::fail("unsupported rm size");
}

Push::Push(Imm imm) : op_(imm) {
  if (imm.size() == Size::k64) common::fail("unsupported imm size");
}

int8_t Push::Encode(Linker&, common::data code) const {
  InstrEncoder encoder(code);

  if (op_.size() != Size::k64) {
    encoder.EncodeOperandSize(op_.size());
  }
  if (op_.RequiresREX()) {
    encoder.EncodeREX();
  }
  if (op_.is_reg()) {
    encoder.EncodeOpcode(0x50);
    encoder.EncodeOpcodeReg(op_.reg());

  } else if (op_.is_mem()) {
    encoder.EncodeOpcode(0xff);
    encoder.EncodeOpcodeExt(6);
    encoder.EncodeRM(op_.mem());

  } else if (op_.is_imm()) {
    encoder.EncodeOpcode((op_.size() == Size::k8) ? 0x6a : 0x68);
    encoder.EncodeImm(op_.imm());
  }

  return encoder.size();
}

std::string Push::ToString() const { return "push " + op_.ToString(); }

Pop::Pop(RM rm) : op_(rm) {
  if (rm.size() != Size::k16 && rm.size() != Size::k64) common::fail("unsupported rm size");
}

int8_t Pop::Encode(Linker&, common::data code) const {
  InstrEncoder encoder(code);

  if (op_.size() != Size::k64) {
    encoder.EncodeOperandSize(op_.size());
  }
  if (op_.RequiresREX()) {
    encoder.EncodeREX();
  }
  if (op_.is_reg()) {
    encoder.EncodeOpcode(0x58);
    encoder.EncodeOpcodeReg(op_.reg());

  } else if (op_.is_mem()) {
    encoder.EncodeOpcode(0x8f);
    encoder.EncodeOpcodeExt(0);
    encoder.EncodeRM(op_.mem());
  }

  return encoder.size();
}

std::string Pop::ToString() const { return "pop " + op_.ToString(); }

Setcc::Setcc(InstrCond cond, RM op) : cond_(cond), op_(op) {
  if (op.size() != k8) common::fail("unsupported rm size");
}

int8_t Setcc::Encode(Linker&, common::data code) const {
  InstrEncoder encoder(code);

  if (op_.RequiresREX()) {
    encoder.EncodeREX();
  }

  encoder.EncodeOpcode(0x0f, 0x90 | cond_);
  encoder.EncodeRM(op_);

  return encoder.size();
}

std::string Setcc::ToString() const {
  return "set" + to_suffix_string(cond_) + " " + op_.ToString();
}

}  // namespace x86_64
