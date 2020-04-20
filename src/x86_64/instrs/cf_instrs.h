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
#include "x86_64/instrs/instr_cond.h"
#include "x86_64/mc/linker.h"
#include "x86_64/mc/unlinker.h"
#include "x86_64/instr.h"
#include "x86_64/ops.h"

namespace x86_64 {

class Jcc final : public Instr {
public:
    Jcc(InstrCond cond, BlockRef block_ref);
    ~Jcc() override;

    InstrCond cond() const;
    BlockRef dst() const;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    InstrCond cond_;
    BlockRef dst_;
};

class Jmp final : public Instr {
public:
    Jmp(RM rm);
    Jmp(BlockRef block_ref);
    ~Jmp() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    Operand dst_;
};

class Call final : public Instr {
public:
    Call(RM rm);
    Call(FuncRef func_ref);
    ~Call() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;

private:
    Operand callee_;
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
