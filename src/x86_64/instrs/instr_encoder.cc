//
//  instr_encoder.cc
//  Katara
//
//  Created by Arne Philipeit on 12/11/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "instr_encoder.h"

namespace x86_64 {

void InstrEncoder::EncodeOperandSize(Size op_size) {
  if (opcode_ != nullptr) throw "attempted to encode operand mode after opcode";

  if (op_size == Size::k16) {
    code_[size_++] = 0x66;  // 16bit operand mode prefix
  } else if (op_size == Size::k64) {
    EncodeREX();

    *rex_ |= 0x08;  // REX.W
  }
}

void InstrEncoder::EncodeREX() {
  if (opcode_ != nullptr) throw "attempted to encode REX after opcode";
  if (rex_ != nullptr) {
    return;
  }

  rex_ = &code_[size_];
  code_[size_++] = 0x40;
}

void InstrEncoder::EncodeOpcode(uint8_t opcode_a) {
  if (opcode_ != nullptr) throw "attempted to encode opcode twice";

  opcode_ = &code_[size_];
  code_[size_++] = opcode_a;
}

void InstrEncoder::EncodeOpcode(uint8_t opcode_a, uint8_t opcode_b) {
  if (opcode_ != nullptr) throw "attempted to encode opcode twice";

  opcode_ = &code_[size_];
  code_[size_++] = opcode_a;
  code_[size_++] = opcode_b;
}

void InstrEncoder::EncodeOpcode(uint8_t opcode_a, uint8_t opcode_b, uint8_t opcode_c) {
  if (opcode_ != nullptr) throw "attempted to encode opcode twice";

  opcode_ = &code_[size_];
  code_[size_++] = opcode_a;
  code_[size_++] = opcode_b;
  code_[size_++] = opcode_c;
}

void InstrEncoder::EncodeOpcodeExt(uint8_t opcode_ext) {
  if (opcode_ == nullptr) throw "attempted to encode reg without opcode";
  if (imm_ != nullptr) throw "attempted to encode reg after imm";

  if (modrm_ == nullptr) {
    modrm_ = &code_[size_++];
  }
  *modrm_ &= ~0x38;                    // Reset Reg
  *modrm_ |= (opcode_ext & 0x7) << 3;  // Reg = reg lower bits
}

void InstrEncoder::EncodeOpcodeReg(const Reg& reg, uint8_t opcode_index, uint8_t lshift) {
  if (opcode_ == nullptr) throw "attempted to encode reg in missing opcode";
  if (imm_ != nullptr) throw "attempted to encode reg after imm";
  if (opcode_index >= 3) throw "opcode index out of range: " + std::to_string(opcode_index);
  if (lshift > 5) throw "opcode lshift out range: " + std::to_string(lshift);

  reg.EncodeInOpcode(rex_, opcode_ + opcode_index, lshift);
}

void InstrEncoder::EncodeModRMReg(const Reg& reg) {
  if (opcode_ == nullptr) throw "attempted to encode reg without opcode";
  if (imm_ != nullptr) throw "attempted to encode reg after imm";

  if (modrm_ == nullptr) {
    modrm_ = &code_[size_++];
  }
  reg.EncodeInModRMReg(rex_, modrm_);
}

void InstrEncoder::EncodeRM(const RM& rm) {
  if (opcode_ == nullptr) throw "attempted to encode reg without opcode";
  if (sib_ != nullptr || disp_ != nullptr) throw "attempted to encode ModRM twice";
  if (imm_ != nullptr) throw "attempted to encode rm after imm";

  if (modrm_ == nullptr) {
    modrm_ = &code_[size_++];
  }
  if (rm.RequiresSIB()) {
    sib_ = &code_[size_++];
  }
  disp_ = &code_[size_];
  size_ += rm.RequiredDispSize();

  if (size_ > code_.size()) throw "instruction exceeds code capacity";

  rm.EncodeInModRM_SIB_Disp(rex_, modrm_, sib_, disp_);
}

void InstrEncoder::EncodeImm(const Imm& imm) {
  if (opcode_ == nullptr) throw "attempted to encode imm without opcode";
  if (imm_ != nullptr) throw "attempted to encode imm twice";

  imm_ = &code_[size_];
  size_ += imm.RequiredImmSize();

  if (size_ > code_.size()) throw "instruction exceeds code capacity";

  imm.EncodeInImm(imm_);
}

}  // namespace x86_64
