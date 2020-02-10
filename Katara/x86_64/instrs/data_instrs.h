//
//  data_instrs.h
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_data_instrs_h
#define x86_64_data_instrs_h

#include <memory>
#include <string>

#include "common/data.h"
#include "x86_64/mc/linker.h"
#include "x86_64/mc/unlinker.h"
#include "x86_64/instr.h"
#include "x86_64/ops.h"

namespace x64 {

class Mov final : public Instr {
public:
    Mov(std::shared_ptr<Reg8> d,
        std::shared_ptr<Reg8> s);
    Mov(std::shared_ptr<Reg16> d,
        std::shared_ptr<Reg16> s);
    Mov(std::shared_ptr<Reg32> d,
        std::shared_ptr<Reg32> s);
    Mov(std::shared_ptr<Reg64> d,
        std::shared_ptr<Reg64> s);
    
    Mov(std::shared_ptr<Mem8> d,
        std::shared_ptr<Reg8> s);
    Mov(std::shared_ptr<Mem16> d,
        std::shared_ptr<Reg16> s);
    Mov(std::shared_ptr<Mem32> d,
        std::shared_ptr<Reg32> s);
    Mov(std::shared_ptr<Mem64> d,
        std::shared_ptr<Reg64> s);
    
    Mov(std::shared_ptr<Reg8> d,
        std::shared_ptr<Mem8> s);
    Mov(std::shared_ptr<Reg16> d,
        std::shared_ptr<Mem16> s);
    Mov(std::shared_ptr<Reg32> d,
        std::shared_ptr<Mem32> s);
    Mov(std::shared_ptr<Reg64> d,
        std::shared_ptr<Mem64> s);
    
    Mov(std::shared_ptr<Reg8> d,
        std::shared_ptr<Imm8> s);
    Mov(std::shared_ptr<Reg16> d,
        std::shared_ptr<Imm16> s);
    Mov(std::shared_ptr<Reg32> d,
        std::shared_ptr<Imm32> s);
    Mov(std::shared_ptr<Reg64> d,
        std::shared_ptr<Imm32> s);
    Mov(std::shared_ptr<Reg64> d,
        std::shared_ptr<Imm64> s);
    
    Mov(std::shared_ptr<Mem8> d,
        std::shared_ptr<Imm8> s);
    Mov(std::shared_ptr<Mem16> d,
        std::shared_ptr<Imm16> s);
    Mov(std::shared_ptr<Mem32> d,
        std::shared_ptr<Imm32> s);
    Mov(std::shared_ptr<Mem64> d,
        std::shared_ptr<Imm32> s);
    ~Mov() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;

private:
    typedef enum : uint8_t {
        kRM_REG,
        kREG_RM,
        kREG_IMM,
        kRM_IMM
    } MovType;
    
    const MovType mov_type_;
    const uint8_t op_size_;
    const std::shared_ptr<Operand> dst_, src_;
};

class Xchg final : public Instr {
    Xchg(std::shared_ptr<RM8> rm,
         std::shared_ptr<Reg8> reg);
    Xchg(std::shared_ptr<RM16> rm,
         std::shared_ptr<Reg16> reg);
    Xchg(std::shared_ptr<RM32> rm,
         std::shared_ptr<Reg32> reg);
    Xchg(std::shared_ptr<RM64> rm,
         std::shared_ptr<Reg64> reg);
    ~Xchg() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    bool CanUseRegAShortcut() const;
    
    const uint8_t op_size_;
    const std::shared_ptr<RM> op_a_;
    const std::shared_ptr<Reg> op_b_;
};

class Push final : public Instr {
public:
    Push(std::shared_ptr<Mem16> mem);
    Push(std::shared_ptr<Mem64> mem);
    Push(std::shared_ptr<Reg16> reg);
    Push(std::shared_ptr<Reg64> reg);
    Push(std::shared_ptr<Imm8> imm);
    Push(std::shared_ptr<Imm16> imm);
    Push(std::shared_ptr<Imm32> imm);
    ~Push() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;

private:
    const uint8_t op_size_;
    const std::shared_ptr<Operand> op_;
};

class Pop final : public Instr {
public:
    Pop(std::shared_ptr<Mem16> mem);
    Pop(std::shared_ptr<Mem64> mem);
    Pop(std::shared_ptr<Reg16> reg);
    Pop(std::shared_ptr<Reg64> reg);
    ~Pop() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    const uint8_t op_size_;
    const std::shared_ptr<Operand> op_;
};

}

#endif /* x86_64_data_instrs_h */
