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

namespace x86_64 {

class UnaryALInstr : public Instr {
public:
    UnaryALInstr(RM op);
    virtual ~UnaryALInstr() override {}
    
    RM op() const;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    
protected:
    UnaryALInstr(Unlinker *unlinker,
                 common::data code,
                 int8_t *size);
    
    virtual uint8_t Opcode() const = 0;
    virtual uint8_t OpcodeExt() const = 0;
    
private:
    RM op_;
};

class BinaryALInstr : public Instr {
public:
    BinaryALInstr(RM op_a, Operand op_b);
    virtual ~BinaryALInstr() override {}
    
    RM op_a() const;
    Operand op_b() const;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    
protected:
    enum class OpEncoding : uint8_t {
        kRM_IMM,
        kRM_IMM8,
        kRM_REG,
        kREG_RM,
    };
    
    OpEncoding op_encoding() const;
    
    bool CanUseRegAShortcut() const;
    
    virtual uint8_t Opcode() const = 0;
    virtual uint8_t OpcodeExt() const = 0;
    
private:
    OpEncoding op_encoding_;
    RM op_a_;
    Operand op_b_;
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
    Mul(RM rm);
    ~Mul() override;
    
    RM factor() const;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    RM factor_;
};

class Imul final : public Instr {
public:
    Imul(RM rm);
    Imul(Reg reg, RM rm);
    Imul(Reg reg, RM rm, Imm imm);
    ~Imul() override;
    
    Reg factor_a() const;
    RM factor_b() const;
    Imm factor_c() const;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    typedef enum : uint8_t {
        kRegAD_RM,
        kReg_RM,
        kReg_RM_IMM,
        kReg_RM_IMM8,
    } ImulType;
    
    bool CanSkipImm() const;
    
    ImulType imul_type_;
    Reg factor_a_;
    RM factor_b_;
    Imm factor_c_;
};

class Div final : public Instr {
public:
    Div(RM rm);
    ~Div() override;
    
    RM divisor() const;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    RM divisor_;
};

class Idiv final : public Instr {
public:
    Idiv(RM rm);
    ~Idiv() override;
    
    RM divisor() const;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    RM divisor_;
};

class SignExtendRegA final : public Instr {
public:
    SignExtendRegA(Size op_size);
    ~SignExtendRegA() override;
    
    Size op_size() const;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;

private:
    Size op_size_;
};

class SignExtendRegAD final : public Instr {
    SignExtendRegAD(Size op_size);
    ~SignExtendRegAD() override;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;
    
private:
    Size op_size_;
};

class Test final : public Instr {
    Test(RM rm, Imm imm);
    Test(RM rm, Reg reg);
    
    ~Test() override;
    
    RM op_a() const;
    Operand op_b() const;
    
    int8_t Encode(Linker *linker,
                  common::data code) const override;
    std::string ToString() const override;

private:
    typedef enum : uint8_t {
        kRM_IMM,
        kRM_REG
    } TestType;
    
    bool CanUseRegAShortcut() const;
    
    TestType test_type_;
    RM op_a_;
    Operand op_b_;
};

}

#endif /* x86_64_al_instrs_h */
