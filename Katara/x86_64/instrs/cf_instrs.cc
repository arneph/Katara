//
//  cf_instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "cf_instrs.h"

#include <iomanip>

#include "x86_64/coding/instr_decoder.h"
#include "x86_64/coding/instr_encoder.h"

namespace x86_64 {

Jcc::Jcc(Jcc::CondType cond,
         std::shared_ptr<BlockRef> block_ref)
    : cond_(cond), dst_(block_ref) {}
Jcc::~Jcc() {}

int8_t Jcc::Encode(Linker *linker,
                   common::data code) const {
    code[0] = 0x0f;
    code[1] = 0x80 | cond_;
    code[2] = 0x00;
    code[3] = 0x00;
    code[4] = 0x00;
    code[5] = 0x00;
    
    linker->AddBlockRef(dst_, code.view(2, 6));
    
    return 6;
}

std::string Jcc::CondAsOpcodeString() const {
    switch (cond_) {
    case kOverflow:       return "jo";
    case kNoOverflow:     return "jno";
    case kSign:           return "js";
    case kNoSign:         return "jns";
    case kParityEven:     return "jpe";
    case kParityOdd:      return "jpo";
    case kEqual:          return "je";
    case kNotEqual:       return "jne";
    case kAbove:          return "ja";
    case kAboveOrEqual:   return "jae";
    case kBelowOrEqual:   return "jbe";
    case kBelow:          return "jb";
    case kGreater:        return "jg";
    case kGreaterOrEqual: return "jge";
    case kLessOrEqual:    return "jle";
    case kLess:           return "jl";
    }
}

std::string Jcc::ToString() const {
    return CondAsOpcodeString() + " " + dst_->ToString();
}

Jmp::Jmp(std::shared_ptr<RM64> rm)
    : dst_(rm) {}
Jmp::Jmp(std::shared_ptr<BlockRef> block_ref)
    : dst_(block_ref) {}
Jmp::~Jmp() {}

int8_t Jmp::Encode(Linker *linker,
                   common::data code) const {
    if (RM64 *rm = dynamic_cast<RM64 *>(dst_.get())) {
        coding::InstrEncoder encoder(code);
        
        if (dst_->RequiresREX()) {
            encoder.EncodeREX();
        }
        encoder.EncodeOpcode(0xff);
        encoder.EncodeOpcodeExt(4);
        encoder.EncodeRM(rm);
        
        return encoder.size();
        
    } else if (BlockRef *block_ref =
               dynamic_cast<BlockRef *>(dst_.get())) {
        code[0] = 0xe9;
        code[1] = 0x00;
        code[2] = 0x00;
        code[3] = 0x00;
        code[4] = 0x00;
        
        linker->AddBlockRef(std::shared_ptr<BlockRef>(dst_, block_ref),
                            code.view(1, 5));
        
        return 5;
    } else {
        return -1;
    }
}

std::string Jmp::ToString() const {
    return "jmp " + dst_->ToString();
}

Call::Call(std::shared_ptr<RM64> rm)
    : callee_(rm) {}
Call::Call(std::shared_ptr<FuncRef> func_ref)
    : callee_(func_ref) {}
Call::~Call() {}

int8_t Call::Encode(Linker *linker,
                    common::data code) const {
    if (RM64 *rm = dynamic_cast<RM64 *>(callee_.get())) {
        coding::InstrEncoder encoder(code);
        
        if (callee_->RequiresREX()) {
            encoder.EncodeREX();
        }
        encoder.EncodeOpcode(0xff);
        encoder.EncodeOpcodeExt(2);
        encoder.EncodeRM(rm);
        
        return encoder.size();
        
    } else if (FuncRef *func_ref =
               dynamic_cast<FuncRef *>(callee_.get())) {
        code[0] = 0xe8;
        code[1] = 0x00;
        code[2] = 0x00;
        code[3] = 0x00;
        code[4] = 0x00;
        
        linker->AddFuncRef(std::shared_ptr<FuncRef>(callee_, func_ref),
                           code.view(1, 5));
        
        return 5;
    } else {
        return -1;
    }
}

std::string Call::ToString() const {
    return "call " + callee_->ToString();
}

Syscall::Syscall() {}
Syscall::~Syscall() {}

int8_t Syscall::Encode(Linker *linker,
                       common::data code) const {
    code[0] = 0x0f;
    code[1] = 0x05;
    
    return 2;
}

std::string Syscall::ToString() const {
    return "syscall";
}

Ret::Ret() {}
Ret::~Ret() {}

int8_t Ret::Encode(Linker *linker,
                   common::data code) const {
    code[0] = 0xc3;
    
    return 1;
}

std::string Ret::ToString() const {
    return "ret";
}

}
