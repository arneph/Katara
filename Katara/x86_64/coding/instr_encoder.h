//
//  instr_encoder.h
//  Katara
//
//  Created by Arne Philipeit on 12/11/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_instr_encoder_h
#define x86_64_instr_encoder_h

#include <memory>

#include "common/data.h"
#include "x86_64/ops.h"

namespace x86_64 {
namespace coding {

struct InstrEncoder final {
public:
    InstrEncoder(common::data code);
    ~InstrEncoder();
    
    uint8_t size() const;
    
    void EncodeOperandSize(Size op_size);
    
    void EncodeREX();
    
    void EncodeOpcode(uint8_t opcode_a);
    void EncodeOpcode(uint8_t opcode_a,
                      uint8_t opcode_b);
    void EncodeOpcode(uint8_t opcode_a,
                      uint8_t opcode_b,
                      uint8_t opcode_c);
    // Encode constant opcode extension (3 bits) in ModRM reg:
    void EncodeOpcodeExt(uint8_t opcode_ext);
    
    void EncodeOpcodeReg(const Reg &reg,
                         uint8_t opcode_index = 0,
                         uint8_t lshift = 0);
    void EncodeModRMReg(const Reg &reg);
    void EncodeRM(const RM &rm);
    void EncodeImm(const Imm &imm);

private:
    common::data code_;
    uint8_t size_ = 0;
    
    uint8_t *rex_ = nullptr;
    uint8_t *opcode_ = nullptr;
    uint8_t *modrm_ = nullptr;
    uint8_t *sib_ = nullptr;
    uint8_t *disp_ = nullptr;
    uint8_t *imm_ = nullptr;
};

}
}

#endif /* instr_encoder_h */
