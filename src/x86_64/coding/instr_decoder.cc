//
//  instr_decoder.cc
//  Katara
//
//  Created by Arne Philipeit on 12/11/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "instr_decoder.h"

namespace x86_64 {
namespace coding {

InstrDecoder::InstrDecoder(const common::data code) : code_(code) {
  if (code[size_] == 0x66) {
    op_size_ = Size::k16;
    size_++;
  }
  if ((code[size_] & 0xf0) == 0x40) {
    rex_ = &code[size_];
    size_++;

    if (*rex_ & 0x08) {
      op_size_ = Size::k64;
    }
  }
}
InstrDecoder::~InstrDecoder() {}

uint8_t InstrDecoder::size() const { return size_; }

void InstrDecoder::DecodeModRM() {
  if (modrm_ != nullptr) return;
  if (sib_ != nullptr || disp_ != nullptr || imm_ != nullptr)
    throw "attemted to decode modRM after decoding later instruction parts";

  modrm_ = &code_[size_++];
}

void InstrDecoder::DecodeSIB() {
  if (sib_ != nullptr) return;
  if (disp_ != nullptr || imm_ != nullptr)
    throw "attemted to decode SIB after decoding later instruction parts";

  sib_ = &code_[size_++];
}

void InstrDecoder::DecodeDisp(uint8_t disp_size) {
  if (disp_ != nullptr) return;
  if (imm_ != nullptr) throw "attemted to decode Disp after decoding later instruction parts";

  disp_ = &code_[size_];
  size_ += disp_size / 8;
}

Size InstrDecoder::GetOperandSize() const { return op_size_; }

void InstrDecoder::SetOperandSize(Size op_size) { op_size_ = op_size; }

uint8_t InstrDecoder::DecodeOpcodePart() {
  if (opcode_size_ == 3) throw "attempted to decode fourth opcode byte";
  if (modrm_ != nullptr || sib_ != nullptr || disp_ != nullptr || imm_ != nullptr)
    throw "attemted to decode opcode after decoding later instruction parts";

  if (opcode_ == nullptr) {
    opcode_ = &code_[size_];
  }
  opcode_size_++;
  return code_[size_++];
}

uint8_t InstrDecoder::DecodeOpcodeExt() {
  DecodeModRM();

  return (*modrm_ >> 3) & 0x7;
}

Reg InstrDecoder::DecodeOpcodeReg(uint8_t opcode_index, uint8_t lshift) {
  if (opcode_index >= opcode_size_) throw "attempted to decode opcode reg in unknown opcode part";
  if (lshift > 5)
    if (lshift > 5) throw "opcode lshift out range: " + std::to_string(lshift);

  uint8_t opcode_part = opcode_[opcode_index];
  uint8_t reg_index = (opcode_part >> lshift) & 0x7;
  if (rex_ != nullptr && *rex_ & 0x04) {
    reg_index += 8;
  }
  return Reg(op_size_, reg_index);
}

Reg InstrDecoder::DecodeModRMReg() {
  DecodeModRM();

  uint8_t reg_index = (*modrm_ >> 3) & 0x7;
  if (rex_ != nullptr && *rex_ & 0x04) {
    reg_index += 8;
  }
  return Reg(op_size_, reg_index);
}

RM InstrDecoder::DecodeRM() {
  // Obtain ModRM byte:
  DecodeModRM();
  const uint8_t mod = (*modrm_ >> 6) & 0x03;
  const uint8_t rm = (*modrm_ >> 0) & 0x07;

  // Handle register in ModRM:
  if (mod == 3) {
    uint8_t reg_index = rm;
    if (rex_ != nullptr && *rex_ & 0x01) {
      reg_index += 8;
    }
    return Reg(op_size_, reg_index);
  }

  // Obtain SIB if present:
  if (rm == 0x04) DecodeSIB();

  // Obtain Disp if present:
  uint8_t disp_size = 0;
  if (mod == 0 && rm == 0x05) disp_size = 32;
  if (mod == 1) disp_size = 8;
  if (mod == 2) disp_size = 32;
  if (disp_size != 0) DecodeDisp(disp_size);

  // Decode Disp:
  int32_t disp = 0;
  if (disp_size == 8 || disp_size == 32) {
    disp = disp_[0];
  }
  if (disp_size == 32) {
    disp |= disp_[1] << 8;
    disp |= disp_[2] << 16;
    disp |= disp_[3] << 24;
  }

  // Handle disp32 only:
  if (mod == 0 && rm == 0x05) {
    return Mem(op_size_, disp);
  }

  // Handle no SIB:
  if (rm != 0x04) {
    uint8_t base_reg = rm;
    if (rex_ != nullptr && *rex_ & 0x01) {
      base_reg += 8;
    }
    return Mem(op_size_, base_reg, disp);
  }

  // Decode SIB:
  uint8_t s = (*sib_ >> 6) & 0x03;
  uint8_t i = (*sib_ >> 3) & 0x07;
  uint8_t b = (*sib_ >> 0) & 0x07;

  uint8_t base_reg = 0xff;
  uint8_t index_reg = 0xff;
  Scale scale;
  if (s == 0) scale = Scale::kS00;
  if (s == 1) scale = Scale::kS01;
  if (s == 2) scale = Scale::kS10;
  if (s == 3)
    scale = Scale::kS11;
  else
    throw "unexpected scale bits";

  if (mod != 0 || b != 5) {
    base_reg = s;
    if (rex_ != nullptr && *rex_ & 0x01) {
      base_reg += 8;
    }
  }
  if (i != 4 || (rex_ != nullptr && *rex_ & 0x02)) {
    index_reg = i;
    if (rex_ != nullptr && *rex_ & 0x02) {
      index_reg += 8;
    }
  }

  return Mem(op_size_, base_reg, index_reg, scale, disp);
}

Imm InstrDecoder::DecodeImm(uint8_t imm_size) {
  if (imm_ == nullptr) {
    imm_ = &code_[size_];
    size_ += imm_size / 8;
  }

  int64_t imm = static_cast<int64_t>(imm_[0]);
  if (imm_size >= 16) {
    imm |= static_cast<int64_t>(imm_[1]) << 8;
  }
  if (imm_size >= 32) {
    imm |= static_cast<int64_t>(imm_[2]) << 16;
    imm |= static_cast<int64_t>(imm_[3]) << 24;
  }
  if (imm_size == 64) {
    imm |= static_cast<int64_t>(imm_[4]) << 32;
    imm |= static_cast<int64_t>(imm_[5]) << 40;
    imm |= static_cast<int64_t>(imm_[6]) << 48;
    imm |= static_cast<int64_t>(imm_[7]) << 56;
  }
  switch (imm_size) {
    case 8:
      return Imm((int8_t)imm);
    case 16:
      return Imm((int16_t)imm);
    case 32:
      return Imm((int32_t)imm);
    case 64:
      return Imm((int64_t)imm);
    default:
      throw "unknown imm size: " + std::to_string(imm_size);
  }
}

}  // namespace coding
}  // namespace x86_64
