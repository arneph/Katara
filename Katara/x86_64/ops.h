//
//  ops.h
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_ops_h
#define x86_64_ops_h

#include <memory>
#include <string>

namespace x86_64 {

enum class Size : uint8_t {
    k8 = 8,
    k16 = 16,
    k32 = 32,
    k64 = 64,
};

class Reg {
public:
    Reg(Size size, uint8_t reg);
    
    Size size() const;
    int8_t reg() const;
    
    bool RequiresREX() const;
    
    // Opcode encoding:
    void EncodeInOpcode(uint8_t *rex,
                        uint8_t *opcode,
                        uint8_t lshift) const;
    
    // ModRM encoding:
    bool RequiresSIB() const;
    uint8_t RequiredDispSize() const;
    void EncodeInModRM_SIB_Disp(uint8_t *rex,
                                uint8_t *modrm,
                                uint8_t *sib,
                                uint8_t *disp) const;
    
    // ModRM reg encoding:
    void EncodeInModRMReg(uint8_t *rex,
                          uint8_t *modrm) const;
    
    std::string ToString() const;
    
private:
    Size size_;
    int8_t reg_;
};

enum class Scale : uint8_t {
    kS00 = 0,
    kS01 = 1,
    kS10 = 2,
    kS11 = 3
};

class Mem {
public:
    Mem(Size size, int32_t disp);
    Mem(Size size, uint8_t base_reg, int32_t disp = 0);
    Mem(Size size, uint8_t index_reg, Scale scale, int32_t disp = 0);
    Mem(Size size, uint8_t base_reg,
        uint8_t index_reg, Scale scale, int32_t disp = 0);
    
    Size size() const;
    uint8_t base_reg() const;
    uint8_t index_reg() const;
    Scale scale() const;
    int32_t disp() const;
    
    bool RequiresREX() const;
    
    // ModRM encoding:
    bool RequiresSIB() const;
    uint8_t RequiredDispSize() const;
    void EncodeInModRM_SIB_Disp(uint8_t *rex,
                                uint8_t *modrm,
                                uint8_t *sib,
                                uint8_t *disp) const;
    
    std::string ToString() const;

private:
    Size size_;
    uint8_t base_reg_;
    uint8_t index_reg_;
    Scale scale_;
    int32_t disp_;
};

class Imm {
public:
    Imm(int8_t value);
    Imm(int16_t value);
    Imm(int32_t value);
    Imm(int64_t value);
    
    Size size() const;
    int64_t value() const;
    
    bool RequiresREX() const;
    uint8_t RequiredImmSize() const;
    void EncodeInImm(uint8_t *imm) const;
    
    std::string ToString() const;

private:
    Size size_;
    int64_t value_;
};

class FuncRef {
public:
    FuncRef(int64_t func_id);
    
    int64_t func_id() const;
    
    std::string ToString() const;
    
private:
    int64_t func_id_;
};

class BlockRef {
public:
    BlockRef(int64_t block_id);
    
    int64_t block_id() const;
    
    std::string ToString() const;
    
private:
    int64_t block_id_;
};

class RM;

class Operand {
public:
    enum class Kind : uint8_t {
        kReg,
        kMem,
        kImm,
        kFuncRef,
        kBlockRef,
    };
    
    Operand(Reg reg);
    Operand(Mem mem);
    Operand(Imm imm);
    Operand(FuncRef func_ref);
    Operand(BlockRef block_ref);
    
    Kind kind() const;
    Size size() const;
    
    bool is_reg() const;
    Reg reg() const;
    
    bool is_mem() const;
    Mem mem() const;
    
    bool is_rm() const;
    RM rm() const;
    
    bool is_imm() const;
    Imm imm() const;
    
    bool is_func_ref() const;
    FuncRef func_ref() const;
    
    bool is_block_ref() const;
    BlockRef block_ref() const;
    
    bool RequiresREX() const;
    
    std::string ToString() const;
    
protected:
    union Data {
        Reg reg;
        Mem mem;
        Imm imm;
        FuncRef func_ref;
        BlockRef block_ref;
        
        Data(Reg r) : reg(r) {}
        Data(Mem m) : mem(m) {}
        Data(Imm i) : imm(i) {}
        Data(FuncRef r) : func_ref(r) {}
        Data(BlockRef r) : block_ref(r) {}
    };
    
    Kind kind_;
    Data data_;
};

class RM : public Operand {
public:
    RM(Reg reg);
    RM(Mem mem);
    
    bool RequiresSIB() const;
    uint8_t RequiredDispSize() const;
    void EncodeInModRM_SIB_Disp(uint8_t *rex,
                                uint8_t *modrm,
                                uint8_t *sib,
                                uint8_t *disp) const;
};

extern const Reg al;
extern const Reg cl;
extern const Reg dl;
extern const Reg bl;
extern const Reg spl;
extern const Reg bpl;
extern const Reg sil;
extern const Reg dil;
extern const Reg r8b;
extern const Reg r9b;
extern const Reg r10b;
extern const Reg r11b;
extern const Reg r12b;
extern const Reg r13b;
extern const Reg r14b;
extern const Reg r15b;

extern const Reg ax;
extern const Reg cx;
extern const Reg dx;
extern const Reg bx;
extern const Reg sp;
extern const Reg bp;
extern const Reg si;
extern const Reg di;
extern const Reg r8w;
extern const Reg r9w;
extern const Reg r10w;
extern const Reg r11w;
extern const Reg r12w;
extern const Reg r13w;
extern const Reg r14w;
extern const Reg r15w;

extern const Reg eax;
extern const Reg ecx;
extern const Reg edx;
extern const Reg ebx;
extern const Reg esp;
extern const Reg ebp;
extern const Reg esi;
extern const Reg edi;
extern const Reg r8d;
extern const Reg r9d;
extern const Reg r10d;
extern const Reg r11d;
extern const Reg r12d;
extern const Reg r13d;
extern const Reg r14d;
extern const Reg r15d;

extern const Reg rax;
extern const Reg rcx;
extern const Reg rdx;
extern const Reg rbx;
extern const Reg rsp;
extern const Reg rbp;
extern const Reg rsi;
extern const Reg rdi;
extern const Reg r8;
extern const Reg r9;
extern const Reg r10;
extern const Reg r11;
extern const Reg r12;
extern const Reg r13;
extern const Reg r14;
extern const Reg r15;

}

#endif /* x86_64_ops_h */
