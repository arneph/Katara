//
//  cf_instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "cf_instrs.h"

#include <iomanip>

#include "src/x86_64/coding/instr_decoder.h"
#include "src/x86_64/coding/instr_encoder.h"

namespace x86_64 {

extern

    Jcc::Jcc(InstrCond cond, BlockRef block_ref)
    : cond_(cond), dst_(block_ref) {}
Jcc::~Jcc() {}

InstrCond Jcc::cond() const { return cond_; }

BlockRef Jcc::dst() const { return dst_; }

int8_t Jcc::Encode(Linker* linker, common::data code) const {
  code[0] = 0x0f;
  code[1] = 0x80 | cond_;
  code[2] = 0x00;
  code[3] = 0x00;
  code[4] = 0x00;
  code[5] = 0x00;

  linker->AddBlockRef(dst_, code.view(2, 6));

  return 6;
}

std::string Jcc::ToString() const { return "j" + to_suffix_string(cond_) + " " + dst_.ToString(); }

Jmp::Jmp(RM rm) : dst_(rm) {
  if (rm.size() != Size::k64) throw "unsupported rm size";
}

Jmp::Jmp(BlockRef block_ref) : dst_(block_ref) {}
Jmp::~Jmp() {}

int8_t Jmp::Encode(Linker* linker, common::data code) const {
  if (dst_.is_rm()) {
    RM rm = dst_.rm();
    coding::InstrEncoder encoder(code);

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

    linker->AddBlockRef(block_ref, code.view(1, 5));

    return 5;
  } else {
    return -1;
  }
}

std::string Jmp::ToString() const { return "jmp " + dst_.ToString(); }

Call::Call(RM rm) : callee_(rm) {
  if (rm.size() != Size::k64) throw "unsupported rm size";
}

Call::Call(FuncRef func_ref) : callee_(func_ref) {}
Call::~Call() {}

int8_t Call::Encode(Linker* linker, common::data code) const {
  if (callee_.is_rm()) {
    RM rm = callee_.rm();

    coding::InstrEncoder encoder(code);

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

    linker->AddFuncRef(func_ref, code.view(1, 5));

    return 5;
  } else {
    return -1;
  }
}

std::string Call::ToString() const { return "call " + callee_.ToString(); }

Syscall::Syscall() {}
Syscall::~Syscall() {}

int8_t Syscall::Encode(Linker*, common::data code) const {
  code[0] = 0x0f;
  code[1] = 0x05;

  return 2;
}

std::string Syscall::ToString() const { return "syscall"; }

Ret::Ret() {}
Ret::~Ret() {}

int8_t Ret::Encode(Linker*, common::data code) const {
  code[0] = 0xc3;

  return 1;
}

std::string Ret::ToString() const { return "ret"; }

}  // namespace x86_64
