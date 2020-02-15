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

typedef enum : uint8_t {
    s00 = 0,
    s01 = 1,
    s10 = 2,
    s11 = 3
} scale_t;

class Operand {
public:
    virtual ~Operand() {}
    
    virtual bool RequiresREX() const = 0;
    virtual std::string ToString() const = 0;
};

class Imm : public Operand {
public:
    virtual uint8_t RequiredImmSize() const = 0;
    virtual void EncodeInImm(uint8_t *imm) const = 0;
};

class RM : public Operand {
public:
    virtual bool RequiresSIB() const = 0;
    virtual uint8_t RequiredDispSize() const = 0;
    virtual void EncodeInModRM_SIB_Disp(uint8_t *rex,
                                        uint8_t *modrm,
                                        uint8_t *sib,
                                        uint8_t *disp) const = 0;
};

class Reg : virtual public RM {
public:
    Reg(uint8_t reg);
    ~Reg() override;
    
    int8_t reg() const;
    
    bool RequiresREX() const override;
    
    // Opcode encoding:
    void EncodeInOpcode(uint8_t *rex,
                        uint8_t *opcode,
                        uint8_t lshift) const;
    
    // ModRM encoding:
    bool RequiresSIB() const override;
    uint8_t RequiredDispSize() const override;
    void EncodeInModRM_SIB_Disp(uint8_t *rex,
                                uint8_t *modrm,
                                uint8_t *sib,
                                uint8_t *disp) const override;
    
    // ModRM reg encoding:
    void EncodeInModRMReg(uint8_t *rex,
                          uint8_t *modrm) const;
    
protected:
    const int8_t reg_;
};

class Mem : virtual public RM {
public:
    Mem(int32_t disp);
    Mem(uint8_t base_reg);
    Mem(uint8_t base_reg, int32_t disp);
    Mem(uint8_t index_reg, scale_t scale);
    Mem(uint8_t index_reg, scale_t scale, int32_t disp);
    Mem(uint8_t base_reg, uint8_t index_reg, scale_t scale);
    Mem(uint8_t base_reg, uint8_t index_reg, scale_t scale,
        int32_t disp);
    ~Mem() override;
    
    uint8_t base_reg() const;
    uint8_t index_reg() const;
    scale_t scale() const;
    int32_t disp() const;
    
    bool RequiresREX() const override;
    
    // ModRM encoding:
    bool RequiresSIB() const override;
    uint8_t RequiredDispSize() const override;
    void EncodeInModRM_SIB_Disp(uint8_t *rex,
                                uint8_t *modrm,
                                uint8_t *sib,
                                uint8_t *disp) const override;
    
    std::string ToString() const override;

protected:
    const uint8_t base_reg_;
    const uint8_t index_reg_;
    const scale_t scale_;
    const int32_t disp_;
};

class FuncRef final : public Operand {
public:
    FuncRef(std::string func_name);
    ~FuncRef();
    
    std::string func_name() const;
    
    bool RequiresREX() const override;
    std::string ToString() const override;
    
private:
    const std::string func_name_;
};

class BlockRef final : public Operand {
public:
    BlockRef(int64_t block_id);
    ~BlockRef();
    
    int64_t block_id() const;
    
    bool RequiresREX() const override;
    std::string ToString() const override;
    
private:
    const int64_t block_id_;
};

class Imm8 final : public Imm {
public:
    Imm8(int8_t value);
    ~Imm8();
    
    int8_t value() const;
    
    bool RequiresREX() const override;
    uint8_t RequiredImmSize() const override;
    void EncodeInImm(uint8_t *imm) const override;
    std::string ToString() const override;

private:
    const int8_t value_;
};

class RM8 : virtual public RM {
};

class Mem8 final : public Mem, public RM8 {
public:
    Mem8(int32_t disp) :
        Mem(disp) {}
    Mem8(uint8_t base_reg) :
        Mem(base_reg) {}
    Mem8(uint8_t base_reg, int32_t disp) :
        Mem(base_reg, disp) {}
    Mem8(uint8_t index_reg, scale_t scale) :
        Mem(index_reg, scale) {}
    Mem8(uint8_t index_reg, scale_t scale, int32_t disp) :
        Mem(index_reg, scale, disp) {}
    Mem8(uint8_t base_reg, uint8_t index_reg, scale_t scale) :
        Mem(base_reg, index_reg, scale) {}
    Mem8(uint8_t base_reg, uint8_t index_reg, scale_t scale,
         int32_t disp) :
        Mem(base_reg, index_reg, scale, disp) {}
    ~Mem8() override {}
};

class Reg8 final : public Reg, public RM8 {
public:
    Reg8(uint8_t reg) : Reg(reg) {}
    ~Reg8() override {}
    
    bool RequiresREX() const override;
    std::string ToString() const override;
};

class Imm16 final : public Imm {
public:
    Imm16(int16_t value);
    ~Imm16();
    
    int16_t value() const;
    
    bool RequiresREX() const override;
    uint8_t RequiredImmSize() const override;
    void EncodeInImm(uint8_t *imm) const override;
    std::string ToString() const override;
    
private:
    const int16_t value_;
};

class RM16 : virtual public RM {
};

class Mem16 final : public Mem, public RM16 {
public:
    Mem16(int32_t disp) :
        Mem(disp) {}
    Mem16(uint8_t base_reg) :
        Mem(base_reg) {}
    Mem16(uint8_t base_reg, int32_t disp) :
        Mem(base_reg, disp) {}
    Mem16(uint8_t index_reg, scale_t scale) :
        Mem(index_reg, scale) {}
    Mem16(uint8_t index_reg, scale_t scale, int32_t disp) :
        Mem(index_reg, scale, disp) {}
    Mem16(uint8_t base_reg, uint8_t index_reg, scale_t scale) :
        Mem(base_reg, index_reg, scale) {}
    Mem16(uint8_t base_reg, uint8_t index_reg, scale_t scale,
         int32_t disp) :
        Mem(base_reg, index_reg, scale, disp) {}
    ~Mem16() override {}
};

class Reg16 final : public Reg, public RM16 {
public:
    Reg16(uint8_t reg) : Reg(reg) {}
    ~Reg16() override {}
    
    std::string ToString() const override;
};

class Imm32 final : public Imm {
public:
    Imm32(int32_t value);
    ~Imm32();
    
    int32_t value() const;
    
    bool RequiresREX() const override;
    uint8_t RequiredImmSize() const override;
    void EncodeInImm(uint8_t *imm) const override;
    std::string ToString() const override;
    
private:
    const int32_t value_;
};

class RM32 : virtual public RM {
};

class Mem32 final : public Mem, public RM32 {
public:
    Mem32(int32_t disp) :
        Mem(disp) {}
    Mem32(uint8_t base_reg) :
        Mem(base_reg) {}
    Mem32(uint8_t base_reg, int32_t disp) :
        Mem(base_reg, disp) {}
    Mem32(uint8_t index_reg, scale_t scale) :
        Mem(index_reg, scale) {}
    Mem32(uint8_t index_reg, scale_t scale, int32_t disp) :
        Mem(index_reg, scale, disp) {}
    Mem32(uint8_t base_reg, uint8_t index_reg, scale_t scale) :
        Mem(base_reg, index_reg, scale) {}
    Mem32(uint8_t base_reg, uint8_t index_reg, scale_t scale,
         int32_t disp) :
        Mem(base_reg, index_reg, scale, disp) {}
    ~Mem32() override {}
};

class Reg32 final : public Reg, public RM32 {
public:
    Reg32(uint8_t reg) : Reg(reg) {}
    ~Reg32() override {}
    
    std::string ToString() const override;
};

class Imm64 final : public Imm {
public:
    Imm64(int64_t value);
    ~Imm64();
    
    int64_t value() const;
    
    bool RequiresREX() const override;
    uint8_t RequiredImmSize() const override;
    void EncodeInImm(uint8_t *imm) const override;
    std::string ToString() const override;
    
private:
    const int64_t value_;
};

class RM64 : virtual public RM {
};

class Mem64 final : public Mem, public RM64 {
public:
    Mem64(int32_t disp) :
        Mem(disp) {}
    Mem64(uint8_t base_reg) :
        Mem(base_reg) {}
    Mem64(uint8_t base_reg, int32_t disp) :
        Mem(base_reg, disp) {}
    Mem64(uint8_t index_reg, scale_t scale) :
        Mem(index_reg, scale) {}
    Mem64(uint8_t index_reg, scale_t scale, int32_t disp) :
        Mem(index_reg, scale, disp) {}
    Mem64(uint8_t base_reg, uint8_t index_reg, scale_t scale) :
        Mem(base_reg, index_reg, scale) {}
    Mem64(uint8_t base_reg, uint8_t index_reg, scale_t scale,
         int32_t disp) :
        Mem(base_reg, index_reg, scale, disp) {}
};

class Reg64 final : public Reg, public RM64 {
public:
    Reg64(uint8_t reg) : Reg(reg) {}
    
    std::string ToString() const override;
};

extern const std::shared_ptr<Reg8> al,   cl,   dl,   bl;
extern const std::shared_ptr<Reg8> spl,  bpl,  sil,  dil;
extern const std::shared_ptr<Reg8> r8b,  r9b,  r10b, r11b;
extern const std::shared_ptr<Reg8> r12b, r12b, r14b, r15b;

extern const std::shared_ptr<Reg16> ax,   cx,   dx,   bx;
extern const std::shared_ptr<Reg16> sp,   bp,   si,   di;
extern const std::shared_ptr<Reg16> r8w,  r9w,  r10w, r11w;
extern const std::shared_ptr<Reg16> r12w, r12w, r14w, r15w;

extern const std::shared_ptr<Reg32> eax,  ecx,  edx,  ebx;
extern const std::shared_ptr<Reg32> esp,  ebp,  esi,  edi;
extern const std::shared_ptr<Reg32> r8d,  r9d,  r10d, r11d;
extern const std::shared_ptr<Reg32> r12d, r12d, r14d, r15d;

extern const std::shared_ptr<Reg64> rax, rcx, rdx, rbx;
extern const std::shared_ptr<Reg64> rsp, rbp, rsi, rdi;
extern const std::shared_ptr<Reg64> r8,  r9,  r10, r11;
extern const std::shared_ptr<Reg64> r12, r13, r14, r15;

inline namespace literals {

extern std::unique_ptr<Imm8>  operator""_imm8 (unsigned long long value);
extern std::unique_ptr<Imm16> operator""_imm16(unsigned long long value);
extern std::unique_ptr<Imm32> operator""_imm32(unsigned long long value);
extern std::unique_ptr<Imm64> operator""_imm64(unsigned long long value);

extern std::unique_ptr<FuncRef> operator""_f(const char *value, unsigned long len);

}
}

#endif /* x86_64_ops_h */
