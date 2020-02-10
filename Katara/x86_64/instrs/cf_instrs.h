//
//  cf_instrs.h
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_cf_instrs_h
#define x86_64_cf_instrs_h

#include <memory>
#include <string>

#include "common/data.h"
#include "x86_64/mc/linker.h"
#include "x86_64/mc/unlinker.h"
#include "x86_64/instr.h"
#include "x86_64/ops.h"

namespace x64 {

class Jcc final : public Instr {
public:
    typedef enum : int8_t {
        kOverflow       = 0x00,
        kNoOverflow     = 0x01,
        kCarry          = 0x02,
        kNoCarry        = 0x03,
        kZero           = 0x04,
        kNoZero         = 0x05,
        kCarryZero      = 0x06,
        kNoCarryZero    = 0x07,
        kSign           = 0x08,
        kNoSign         = 0x09,
        kParity         = 0x0a,
        kNoParity       = 0x0b,
        kParityEven     = kParity,
        kParityOdd      = kNoParity,
        
        // all integers:
        kEqual          = kZero,
        kNotEqual       = kNoZero,
        
        // unsigned integers:
        kAbove          = kNoCarryZero,
        kAboveOrEqual   = kNoCarry,
        kBelowOrEqual   = kCarryZero,
        kBelow          = kCarry,
        
        // signed integers:
        kGreater        = 0x0f,
        kGreaterOrEqual = 0x0d,
        kLessOrEqual    = 0x0e,
        kLess           = 0x0c
    } CondType;
    
    Jcc(CondType cond, std::shared_ptr<BlockRef> block_ref);
    ~Jcc() override;

    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    std::string CondAsOpcodeString() const;
    
    const CondType cond_;
    const std::shared_ptr<BlockRef> dst_;
};

class Jmp final : public Instr {
public:
    Jmp(std::shared_ptr<RM64> rm);
    Jmp(std::shared_ptr<BlockRef> block_ref);
    ~Jmp() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    const std::shared_ptr<Operand> dst_;
};

class Call final : public Instr {
public:
    Call(std::shared_ptr<RM64> rm);
    Call(std::shared_ptr<FuncRef> func_ref);
    ~Call() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;

private:
    const std::shared_ptr<Operand> callee_;
};

class Syscall final : public Instr {
public:
    Syscall();
    ~Syscall() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
};

class Ret final : public Instr {
public:
    Ret();
    ~Ret() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
};

}

#endif /* x86_64_cf_instrs_h */
