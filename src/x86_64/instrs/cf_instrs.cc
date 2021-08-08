//
//  cf_instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "cf_instrs.h"

#include <iomanip>

#include "src/common/logging.h"
#include "src/x86_64/instrs/instr_encoder.h"

namespace x86_64 {

int8_t Jcc::Encode(Linker& linker, common::DataView code) const {
  code[0] = 0x0f;
  code[1] = 0x80 | cond_;
  code[2] = 0x00;
  code[3] = 0x00;
  code[4] = 0x00;
  code[5] = 0x00;

  linker.AddBlockRef(dst_, code.SubView(2, 6));

  return 6;
}

std::string Jcc::ToString() const { return "j" + to_suffix_string(cond_) + " " + dst_.ToString(); }

Jmp::Jmp(RM rm) : dst_(rm) {
  if (rm.size() != Size::k64) {
    common::fail("unsupported rm size");
  }
}

int8_t Jmp::Encode(Linker& linker, common::DataView code) const {
  if (dst_.is_rm()) {
    RM rm = dst_.rm();
    InstrEncoder encoder(code);

    if (dst_.RequiresREX()) {
      encoder.EncodeREX();
    }
    encoder.EncodeOpcode(0xff);
    encoder.EncodeOpcodeExt(4);
    encoder.EncodeRM(rm);

    return encoder.size();

  } else if (dst_.is_block_ref()) {
    BlockRef block_ref = dst_.block_ref();
    code[0] = 0xe9;
    code[1] = 0x00;
    code[2] = 0x00;
    code[3] = 0x00;
    code[4] = 0x00;

    linker.AddBlockRef(block_ref, code.SubView(1, 5));

    return 5;
  } else {
    return -1;
  }
}

std::string Jmp::ToString() const { return "jmp " + dst_.ToString(); }

Call::Call(RM rm) : callee_(rm) {
  if (rm.size() != Size::k64) common::fail("unsupported rm size");
}

int8_t Call::Encode(Linker& linker, common::DataView code) const {
  if (callee_.is_rm()) {
    RM rm = callee_.rm();

    InstrEncoder encoder(code);

    if (callee_.RequiresREX()) {
      encoder.EncodeREX();
    }
    encoder.EncodeOpcode(0xff);
    encoder.EncodeOpcodeExt(2);
    encoder.EncodeRM(rm);

    return encoder.size();

  } else if (callee_.is_func_ref()) {
    FuncRef func_ref = callee_.func_ref();
    code[0] = 0xe8;
    code[1] = 0x00;
    code[2] = 0x00;
    code[3] = 0x00;
    code[4] = 0x00;

    linker.AddFuncRef(func_ref, code.SubView(1, 5));

    return 5;
  } else {
    return -1;
  }
}

std::string Call::ToString() const { return "call " + callee_.ToString(); }

int8_t Syscall::Encode(Linker&, common::DataView code) const {
  code[0] = 0x0f;
  code[1] = 0x05;

  return 2;
}

std::string Syscall::ToString() const { return "syscall"; }

int8_t Ret::Encode(Linker&, common::DataView code) const {
  code[0] = 0xc3;

  return 1;
}

std::string Ret::ToString() const { return "ret"; }

}  // namespace x86_64
