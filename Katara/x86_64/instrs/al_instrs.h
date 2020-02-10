//
//  al_instrs.h
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_al_instrs_h
#define x86_64_al_instrs_h

#include <memory>
#include <string>

#include "common/data.h"
#include "x86_64/mc/linker.h"
#include "x86_64/mc/unlinker.h"
#include "x86_64/instr.h"
#include "x86_64/ops.h"

namespace x64 {

class UnaryALInstr : public Instr {
public:
    UnaryALInstr(std::shared_ptr<RM8> rm);
    UnaryALInstr(std::shared_ptr<RM16> rm);
    UnaryALInstr(std::shared_ptr<RM32> rm);
    UnaryALInstr(std::shared_ptr<RM64> rm);
    
    virtual ~UnaryALInstr() override {}
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    
protected:
    UnaryALInstr(Unlinker *unlinker,
                 common::data code,
                 int8_t *size);
    
    uint8_t op_size() const;
    std::shared_ptr<RM> op() const;
    
    virtual uint8_t Opcode() const = 0;
    virtual uint8_t OpcodeExt() const = 0;
    
private:
    const uint8_t op_size_;
    const std::shared_ptr<RM> op_;
};

class BinaryALInstr : public Instr {
public:
    BinaryALInstr(std::shared_ptr<Mem8> mem,
                  std::shared_ptr<Imm8> imm);
    BinaryALInstr(std::shared_ptr<Mem16> mem,
                  std::shared_ptr<Imm16> imm);
    BinaryALInstr(std::shared_ptr<Mem32> mem,
                  std::shared_ptr<Imm32> imm);
    BinaryALInstr(std::shared_ptr<Mem64> mem,
                  std::shared_ptr<Imm32> imm);
    
    BinaryALInstr(std::shared_ptr<Mem16> mem,
                  std::shared_ptr<Imm8> imm);
    BinaryALInstr(std::shared_ptr<Mem32> mem,
                  std::shared_ptr<Imm8> imm);
    BinaryALInstr(std::shared_ptr<Mem64> mem,
                  std::shared_ptr<Imm8> imm);
    
    BinaryALInstr(std::shared_ptr<Reg8> reg,
                  std::shared_ptr<Imm8> imm);
    BinaryALInstr(std::shared_ptr<Reg16> reg,
                  std::shared_ptr<Imm16> imm);
    BinaryALInstr(std::shared_ptr<Reg32> reg,
                  std::shared_ptr<Imm32> imm);
    BinaryALInstr(std::shared_ptr<Reg64> reg,
                  std::shared_ptr<Imm32> imm);
    
    BinaryALInstr(std::shared_ptr<Reg16> reg,
                  std::shared_ptr<Imm8> imm);
    BinaryALInstr(std::shared_ptr<Reg32> reg,
                  std::shared_ptr<Imm8> imm);
    BinaryALInstr(std::shared_ptr<Reg64> reg,
                  std::shared_ptr<Imm8> imm);
    
    BinaryALInstr(std::shared_ptr<Mem8> mem,
                  std::shared_ptr<Reg8> reg);
    BinaryALInstr(std::shared_ptr<Mem16> mem,
                  std::shared_ptr<Reg16> reg);
    BinaryALInstr(std::shared_ptr<Mem32> mem,
                  std::shared_ptr<Reg32> reg);
    BinaryALInstr(std::shared_ptr<Mem64> mem,
                  std::shared_ptr<Reg64> reg);
    
    BinaryALInstr(std::shared_ptr<Reg8> regA,
                  std::shared_ptr<Reg8> regB);
    BinaryALInstr(std::shared_ptr<Reg16> regA,
                  std::shared_ptr<Reg16> regB);
    BinaryALInstr(std::shared_ptr<Reg32> regA,
                  std::shared_ptr<Reg32> regB);
    BinaryALInstr(std::shared_ptr<Reg64> regA,
                  std::shared_ptr<Reg64> regB);
    
    BinaryALInstr(std::shared_ptr<Reg8> reg,
                  std::shared_ptr<Mem8> mem);
    BinaryALInstr(std::shared_ptr<Reg16> reg,
                  std::shared_ptr<Mem16> mem);
    BinaryALInstr(std::shared_ptr<Reg32> reg,
                  std::shared_ptr<Mem32> mem);
    BinaryALInstr(std::shared_ptr<Reg64> reg,
                  std::shared_ptr<Mem64> mem);
    
    virtual ~BinaryALInstr() override {}
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    
protected:
    typedef enum : uint8_t {
        kRM_IMM,
        kRM_IMM8,
        kRM_REG,
        kREG_RM
    } OpEncoding;
    
    OpEncoding op_encoding() const;
    uint8_t op_size() const;
    std::shared_ptr<Operand> op_a() const;
    std::shared_ptr<Operand> op_b() const;
    
    bool CanUseRegAShortcut() const;
    
    virtual uint8_t Opcode() const = 0;
    virtual uint8_t OpcodeExt() const = 0;
    
private:
    const OpEncoding op_encoding_;
    const uint8_t op_size_;
    const std::shared_ptr<Operand> op_a_, op_b_;
};

class Not final : public UnaryALInstr {
public:
    using UnaryALInstr::UnaryALInstr;
    
    ~Not() override;
    
    std::string ToString() const override;
    
public:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class And final : public BinaryALInstr {
public:
    using BinaryALInstr::BinaryALInstr;
    
    ~And() override;
    
    std::string ToString() const override;
    
protected:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class Or final : public BinaryALInstr {
public:
    using BinaryALInstr::BinaryALInstr;
    
    ~Or() override;
    
    std::string ToString() const override;
    
protected:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class Xor final : public BinaryALInstr {
public:
    using BinaryALInstr::BinaryALInstr;
    
    ~Xor() override;
    
    std::string ToString() const override;
    
protected:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class Neg final : public UnaryALInstr {
public:
    using UnaryALInstr::UnaryALInstr;
    
    ~Neg() override;
    
    std::string ToString() const override;
    
public:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class Add final : public BinaryALInstr {
public:
    using BinaryALInstr::BinaryALInstr;
    
    ~Add() override;
    
    std::string ToString() const override;
    
protected:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class Adc final : public BinaryALInstr {
public:
    using BinaryALInstr::BinaryALInstr;
    
    ~Adc() override;
    
    std::string ToString() const override;
    
protected:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class Sub final : public BinaryALInstr {
public:
    using BinaryALInstr::BinaryALInstr;
    
    ~Sub() override;
    
    std::string ToString() const override;
    
protected:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class Sbb final : public BinaryALInstr {
public:
    using BinaryALInstr::BinaryALInstr;
    
    ~Sbb() override;
    
    std::string ToString() const override;
    
protected:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class Cmp final : public BinaryALInstr {
public:
    using BinaryALInstr::BinaryALInstr;
    
    ~Cmp() override;
    
    std::string ToString() const override;
    
protected:
    uint8_t Opcode() const override;
    uint8_t OpcodeExt() const override;
};

class Mul final : public Instr {
public:
    Mul(std::shared_ptr<RM8> rm);
    Mul(std::shared_ptr<RM16> rm);
    Mul(std::shared_ptr<RM32> rm);
    Mul(std::shared_ptr<RM64> rm);
    ~Mul() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    const uint8_t op_size_;
    const std::shared_ptr<RM> factor_;
};

class Imul final : public Instr {
public:
    Imul(std::shared_ptr<RM8> rm);
    Imul(std::shared_ptr<RM16> rm);
    Imul(std::shared_ptr<RM32> rm);
    Imul(std::shared_ptr<RM64> rm);
    
    Imul(std::shared_ptr<Reg16> reg,
         std::shared_ptr<RM16> rm,
         std::shared_ptr<Imm8> imm = nullptr);
    Imul(std::shared_ptr<Reg32> reg,
         std::shared_ptr<RM32> rm,
         std::shared_ptr<Imm8> imm = nullptr);
    Imul(std::shared_ptr<Reg64> reg,
         std::shared_ptr<RM64> rm,
         std::shared_ptr<Imm8> imm = nullptr);
    
    Imul(std::shared_ptr<Reg16> reg,
         std::shared_ptr<RM16> rm,
         std::shared_ptr<Imm16> imm);
    Imul(std::shared_ptr<Reg32> reg,
         std::shared_ptr<RM32> rm,
         std::shared_ptr<Imm32> imm);
    Imul(std::shared_ptr<Reg64> reg,
         std::shared_ptr<RM64> rm,
         std::shared_ptr<Imm32> imm);
    
    ~Imul() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    typedef enum : uint8_t {
        kRegAD_RM,
        kReg_RM_IMM8,
        kReg_RM_IMM,
    } ImulType;
    
    bool CanSkipImm() const;
    
    const ImulType imul_type_;
    const uint8_t op_size_;
    const std::shared_ptr<Reg> factor_a_;
    const std::shared_ptr<RM> factor_b_;
    const std::shared_ptr<Imm> factor_c_;
};

class Div final : public Instr {
public:
    Div(std::shared_ptr<RM8> rm);
    Div(std::shared_ptr<RM16> rm);
    Div(std::shared_ptr<RM32> rm);
    Div(std::shared_ptr<RM64> rm);
    ~Div() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    const uint8_t op_size_;
    const std::shared_ptr<RM> factor_;
};

class Idiv final : public Instr {
public:
    Idiv(std::shared_ptr<RM8> rm);
    Idiv(std::shared_ptr<RM16> rm);
    Idiv(std::shared_ptr<RM32> rm);
    Idiv(std::shared_ptr<RM64> rm);
    ~Idiv() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    const uint8_t op_size_;
    const std::shared_ptr<RM> factor_;
};

class SignExtendRegA final : public Instr {
public:
    SignExtendRegA(uint8_t op_size);
    ~SignExtendRegA() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;

private:
    const uint8_t op_size_;
};

class SignExtendRegAD final : public Instr {
    SignExtendRegAD(uint8_t op_size);
    ~SignExtendRegAD() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    const uint8_t op_size_;
};

class Test final : public Instr {
    Test(std::shared_ptr<RM8> rm,
         std::shared_ptr<Imm8> imm);
    Test(std::shared_ptr<RM16> rm,
         std::shared_ptr<Imm16> imm);
    Test(std::shared_ptr<RM32> rm,
         std::shared_ptr<Imm32> imm);
    Test(std::shared_ptr<RM64> rm,
         std::shared_ptr<Imm32> imm);
    
    Test(std::shared_ptr<RM8> rm,
         std::shared_ptr<Reg8> reg);
    Test(std::shared_ptr<RM16> rm,
         std::shared_ptr<Reg16> reg);
    Test(std::shared_ptr<RM32> rm,
         std::shared_ptr<Reg32> reg);
    Test(std::shared_ptr<RM64> rm,
         std::shared_ptr<Reg64> reg);
    ~Test() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;

private:
    typedef enum : uint8_t {
        kRM_IMM,
        kRM_REG
    } TestType;
    
    bool CanUseRegAShortcut() const;
    
    const TestType test_type_;
    const uint8_t op_size_;
    const std::shared_ptr<RM> op_a_;
    const std::shared_ptr<Operand> op_b_;
};

}

#endif /* x86_64_al_instrs_h */
