//
//  instr_decoder.h
//  Katara
//
//  Created by Arne Philipeit on 12/11/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_instr_decoder_h
#define x86_64_instr_decoder_h

#include <memory>

#include "src/common/data_view/data_view.h"
#include "src/x86_64/ops.h"

namespace x86_64 {

class InstrDecoder final {
 public:
  InstrDecoder(const common::DataView code);

  uint8_t size() const { return size_; }

  Size GetOperandSize() const;
  void SetOperandSize(Size op_size);

  uint8_t DecodeOpcodePart();
  uint8_t DecodeOpcodeExt();

  Reg DecodeOpcodeReg(uint8_t opcode_index = 0, uint8_t lshift = 0);
  Reg DecodeModRMReg();
  RM DecodeRM();
  Imm DecodeImm(uint8_t imm_size);

 private:
  void DecodeModRM();
  void DecodeSIB();
  void DecodeDisp(uint8_t disp_size);

  const common::DataView code_;
  uint8_t size_ = 0;

  Size op_size_ = Size::k32;
  const uint8_t* rex_ = nullptr;
  uint8_t opcode_size_ = 0;
  const uint8_t* opcode_ = nullptr;
  const uint8_t* modrm_ = nullptr;
  const uint8_t* sib_ = nullptr;
  const uint8_t* disp_ = nullptr;
  const uint8_t* imm_ = nullptr;
};

}  // namespace x86_64

#endif /* x86_64_instr_decoder_h */
