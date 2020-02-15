//
//  ops.cc
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "ops.h"

#include <iomanip>
#include <sstream>

namespace x86_64 {

std::string RegToString(uint8_t reg, uint8_t size);

Reg::Reg(uint8_t r) : reg_(r) {
    if (r > 15) {
        throw "register out of bounds: " + std::to_string(r);
    }
}

Reg::~Reg() {}

int8_t Reg::reg() const {
    return reg_;
}

bool Reg::RequiresREX() const {
    return reg_ >= 8;
}

void Reg::EncodeInOpcode(uint8_t *rex,
                         uint8_t *opcode,
                         uint8_t ls) const {
    if (reg_ >= 8) {
        *rex |= 0x01;               // REX.B
    }
    *opcode &= ~0x7 << ls;          // Reset Reg
    *opcode |= (reg_ & 0x7) << ls;   // Reg = reg
}

bool Reg::RequiresSIB() const {
    return false;
}

uint8_t Reg::RequiredDispSize() const {
    return 0;
}

void Reg::EncodeInModRM_SIB_Disp(uint8_t *rex,
                                 uint8_t *modrm,
                                 uint8_t *sib,
                                 uint8_t *disp) const {
    if (reg_ >= 8) {
        *rex |= 0x01;           // REX.B
    }
    *modrm |= 0xc0;             // Mod = 11
    *modrm &= ~0x07;            // Reset R/M
    *modrm |= reg_ & 0x7;        // R/M = reg lower bits
}

void Reg::EncodeInModRMReg(uint8_t *rex,
                           uint8_t *modrm) const {
    if (reg_ >= 8) {
        *rex |= 0x04;           // REX.R
    }
    *modrm &= ~0x38;            // Reset Reg
    *modrm |= (reg_ & 0x7) << 3; // Reg = reg lower bits
}

Mem::Mem(int32_t disp)
    : Mem(0xff,     0xff,      scale_t::s00, disp) {}
Mem::Mem(uint8_t base_reg)
    : Mem(base_reg, 0xff,      scale_t::s00, 0) {}
Mem::Mem(uint8_t base_reg, int32_t disp)
    : Mem(base_reg, 0xff,      scale_t::s00, disp) {}
Mem::Mem(uint8_t index_reg, scale_t scale)
    : Mem(0xff,     index_reg, scale,        0) {}
Mem::Mem(uint8_t index_reg, scale_t scale, int32_t disp)
    : Mem(0xff,     index_reg, scale,        disp) {}
Mem::Mem(uint8_t base_reg, uint8_t index_reg, scale_t scale)
    : Mem(base_reg, index_reg, scale,        0) {}

Mem::Mem(uint8_t b, uint8_t i, scale_t s, int32_t d) :
    base_reg_(b), index_reg_(i), scale_(s), disp_(d) {
    if (b > 15 && b != 0xff) {
        throw "register out of bounds: " + std::to_string(b);
    }
    if (i > 15 && i != 0xff) {
        throw "register out bounds: " + std::to_string(i);
    } else if (i == 4) {
        throw "index register can't be stack pointer";
    }
}

Mem::~Mem() {}

uint8_t Mem::base_reg() const {
    return base_reg_;
}

uint8_t Mem::index_reg() const {
    return index_reg_;
}

scale_t Mem::scale() const {
    return scale_;
}

int32_t Mem::disp() const {
    return disp_;
}

bool Mem::RequiresREX() const {
    if (base_reg_ != 0xff && base_reg_ >= 8) {
        return true;
    }
    if (index_reg_ != 0xff && index_reg_ >= 8) {
        return true;
    }
    return false;
}

bool Mem::RequiresSIB() const {
    if (base_reg_ == 0xff) {
        return true;
    } else {
        if (index_reg_ == 0xff) {
            return base_reg_ == 4 || base_reg_ == 12;
        } else {
            return true;
        }
    }
}

uint8_t Mem::RequiredDispSize() const {
    if (base_reg_ == 0xff) {
        return 4;
    } else {
        if (disp_ == 0 && base_reg_ != 5 && base_reg_ != 13) {
            return 0;
        } else if (-128 <= disp_ && disp_ <= +127) {
            return 1;
        } else {
            return 4;
        }
    }
}

void Mem::EncodeInModRM_SIB_Disp(uint8_t *rex,
                                 uint8_t *modrm,
                                 uint8_t *sib,
                                 uint8_t *dispp) const {
    if (base_reg_ != 0xff && base_reg_ >= 8) {
        *rex |= 0x01;                       // REX.B
    }
    if (index_reg_ != 0xff && index_reg_ >= 8) {
        *rex |= 0x02;                       // REX.X
    }
    if (base_reg_ == 0xff) {
        *modrm &= ~0xc0;                    // Mod = 00
        *modrm &= ~0x07;                    // Reset R/M
        *modrm |= 0x04;                     // R/M = 100
        *sib = 0x00;                        // Reset SIB
        if (index_reg_ == 0xff) {
            *sib |= 0x20;                   // S = 00, I = 100
        } else {
            *sib |= (scale_ & 0x3) << 6;    // S = scale
            *sib |= (index_reg_ & 0x7) << 3;// I = index_reg
        }
        *sib |= 0x05;                       // B = 101
        dispp[0] = (disp_ >> 0)  & 0xff;
        dispp[1] = (disp_ >> 8)  & 0xff;
        dispp[2] = (disp_ >> 16) & 0xff;
        dispp[3] = (disp_ >> 24) & 0xff;
        
    } else {
        if (index_reg_ == 0xff) {
            *modrm &= ~0x07;                // Reset R/M
            *modrm |= base_reg_ & 0x7;      // R/M = base_reg
            if (base_reg_ == 4 || base_reg_ == 12) { // R/M = ->SIB
                *sib = 0x24;                // S = 00, I = 100, B = 100
            }
        } else {
            *modrm &= ~0x07;                // Reset R/M
            *modrm |= 0x04;                 // R/M = ->SIB
            *sib = 0x00;                    // Reset SIB
            *sib |= (scale_ & 0x3) << 6;     // S = scale
            *sib |= (index_reg_ & 0x7) << 3;  // I = index_reg
            *sib |= base_reg_ & 0x7;          // B = base_reg
        }
        if (disp_ == 0 && base_reg_ != 5 && base_reg_ != 13) {
            *modrm &= ~0xc0;            // Mod = 00
        } else if (-128 <= disp_ && disp_ <= +127) {
            *modrm &= ~0xc0;            // Reset Mod
            *modrm |= 0x40;             // Mod = 01
            dispp[0] = disp_ & 0xff;
        } else {
            *modrm &= ~0xc0;            // Reset Mod
            *modrm |= 0x80;             // Mod = 10
            dispp[0] = (disp_ >> 0)  & 0xff;
            dispp[1] = (disp_ >> 8)  & 0xff;
            dispp[2] = (disp_ >> 16) & 0xff;
            dispp[3] = (disp_ >> 24) & 0xff;
        }
    }
}

std::string Mem::ToString() const {
    std::stringstream ss;
    ss << "[";
    bool added_term = false;
    if (base_reg_ != 0xff) {
        ss << RegToString(base_reg_, 64);
        added_term = true;
    }
    if (index_reg_ != 0xff) {
        if (added_term) ss << " + ";
        ss << std::to_string(1 << scale_) << "*" << RegToString(index_reg_, 64);
        added_term = true;
    }
    if (disp_ != 0) {
        if (added_term) ss << " + ";
        if (-128 <= disp_ && disp_ <= +127) {
            ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << disp_;
        } else {
            ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << disp_;
        }
    }
    ss << "]";
    return ss.str();
}

FuncRef::FuncRef(std::string func_name) : func_name_(func_name) {}
FuncRef::~FuncRef() {}

std::string FuncRef::func_name() const {
    return func_name_;
}

bool FuncRef::RequiresREX() const {
    return false;
}

std::string FuncRef::ToString() const {
    return "<" + func_name_ + ">";
}

BlockRef::BlockRef(int64_t block_id)
    : block_id_(block_id) {}
BlockRef::~BlockRef() {}

int64_t BlockRef::block_id() const {
    return block_id_;
}

bool BlockRef::RequiresREX() const {
    return false;
}

std::string BlockRef::ToString() const {
    return "BB" + std::to_string(block_id_);
}

Imm8::Imm8(int8_t v) : value_(v) {}
Imm8::~Imm8() {}

int8_t Imm8::value() const {
    return value_;
}

bool Imm8::RequiresREX() const {
    return false;
}

uint8_t Imm8::RequiredImmSize() const {
    return 1;
}

void Imm8::EncodeInImm(uint8_t *imm) const {
    imm[0] = static_cast<uint8_t>(value_);
}

std::string Imm8::ToString() const {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(value_);
    return ss.str();
}

bool Reg8::RequiresREX() const {
    // 32bit: ah, ch, dh, lh
    // 64bit: spl, bpl, sil, dil
    return reg_ >= 4;
}

std::string Reg8::ToString() const {
    return RegToString(reg_, 8);
}

Imm16::Imm16(int16_t v) : value_(v) {}
Imm16::~Imm16() {}

int16_t Imm16::value() const {
    return value_;
}

bool Imm16::RequiresREX() const {
    return false;
}

uint8_t Imm16::RequiredImmSize() const {
    return 2;
}

void Imm16::EncodeInImm(uint8_t *imm) const {
    imm[0] = static_cast<uint8_t>(value_ >> 0) & 0xff;
    imm[1] = static_cast<uint8_t>(value_ >> 8) & 0xff;
}

std::string Imm16::ToString() const {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(4) << value_;
    return ss.str();
}

std::string Reg16::ToString() const {
    return RegToString(reg_, 16);
}

Imm32::Imm32(int32_t v) : value_(v) {}
Imm32::~Imm32() {}

int32_t Imm32::value() const {
    return value_;
}

bool Imm32::RequiresREX() const {
    return false;
}

uint8_t Imm32::RequiredImmSize() const {
    return 4;
}

void Imm32::EncodeInImm(uint8_t *imm) const {
    imm[0] = static_cast<uint8_t>(value_ >>  0) & 0xff;
    imm[1] = static_cast<uint8_t>(value_ >>  8) & 0xff;
    imm[2] = static_cast<uint8_t>(value_ >> 16) & 0xff;
    imm[3] = static_cast<uint8_t>(value_ >> 24) & 0xff;
}

std::string Imm32::ToString() const {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << value_;
    return ss.str();
}

std::string Reg32::ToString() const {
    return RegToString(reg_, 32);
}

Imm64::Imm64(int64_t v) : value_(v) {}
Imm64::~Imm64() {}

int64_t Imm64::value() const {
    return value_;
}

bool Imm64::RequiresREX() const {
    return true;
}

uint8_t Imm64::RequiredImmSize() const {
    return 8;
}

void Imm64::EncodeInImm(uint8_t *imm) const {
    imm[0] = static_cast<uint8_t>(value_ >> uint64_t{ 0}) & 0xff;
    imm[1] = static_cast<uint8_t>(value_ >> uint64_t{ 8}) & 0xff;
    imm[2] = static_cast<uint8_t>(value_ >> uint64_t{16}) & 0xff;
    imm[3] = static_cast<uint8_t>(value_ >> uint64_t{24}) & 0xff;
    imm[4] = static_cast<uint8_t>(value_ >> uint64_t{32}) & 0xff;
    imm[5] = static_cast<uint8_t>(value_ >> uint64_t{40}) & 0xff;
    imm[6] = static_cast<uint8_t>(value_ >> uint64_t{48}) & 0xff;
    imm[7] = static_cast<uint8_t>(value_ >> uint64_t{56}) & 0xff;
}

std::string Imm64::ToString() const {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(16) << value_;
    return ss.str();
}

std::string Reg64::ToString() const {
    return RegToString(reg_, 64);
}

std::string RegToString(uint8_t reg, uint8_t size) {
    if (size == 8) {
        switch (reg) {
            case 0x0: return "al";
            case 0x1: return "cl";
            case 0x2: return "dl";
            case 0x3: return "bl";
            case 0x4: return "spl";
            case 0x5: return "bpl";
            case 0x6: return "sil";
            case 0x7: return "dil";
            case 0x8: return "r8b";
            case 0x9: return "r9b";
            case 0xA: return "r10b";
            case 0xB: return "r11b";
            case 0xC: return "r12b";
            case 0xD: return "r13b";
            case 0xE: return "r14b";
            case 0xF: return "r15b";
            default:
                throw "unknown register index: " + std::to_string(reg);
        }
    } else if (size == 16) {
        switch (reg) {
            case 0x0: return "ax";
            case 0x1: return "cx";
            case 0x2: return "dx";
            case 0x3: return "bx";
            case 0x4: return "sp";
            case 0x5: return "bp";
            case 0x6: return "si";
            case 0x7: return "di";
            case 0x8: return "r8w";
            case 0x9: return "r9w";
            case 0xA: return "r10w";
            case 0xB: return "r11w";
            case 0xC: return "r12w";
            case 0xD: return "r13w";
            case 0xE: return "r14w";
            case 0xF: return "r15w";
            default:
                throw "unknown register index: " + std::to_string(reg);
        }
    } else if (size == 32) {
        switch (reg) {
            case 0x0: return "eax";
            case 0x1: return "ecx";
            case 0x2: return "edx";
            case 0x3: return "ebx";
            case 0x4: return "esp";
            case 0x5: return "ebp";
            case 0x6: return "esi";
            case 0x7: return "edi";
            case 0x8: return "r8d";
            case 0x9: return "r9d";
            case 0xA: return "r10d";
            case 0xB: return "r11d";
            case 0xC: return "r12d";
            case 0xD: return "r13d";
            case 0xE: return "r14d";
            case 0xF: return "r15d";
            default:
                throw "unknown register index: " + std::to_string(reg);
        }
    } else if (size == 64) {
        switch (reg) {
            case 0x0: return "rax";
            case 0x1: return "rcx";
            case 0x2: return "rdx";
            case 0x3: return "rbx";
            case 0x4: return "rsp";
            case 0x5: return "rbp";
            case 0x6: return "rsi";
            case 0x7: return "rdi";
            case 0x8: return "r8";
            case 0x9: return "r9";
            case 0xA: return "r10";
            case 0xB: return "r11";
            case 0xC: return "r12";
            case 0xD: return "r13";
            case 0xE: return "r14";
            case 0xF: return "r15";
            default:
                throw "unknown register index: " + std::to_string(reg);
        }
    } else {
        throw "unknown register size: " + std::to_string(size);
    }
}

const std::shared_ptr<Reg8> al  (new Reg8(0x0));
const std::shared_ptr<Reg8> cl  (new Reg8(0x1));
const std::shared_ptr<Reg8> dl  (new Reg8(0x2));
const std::shared_ptr<Reg8> bl  (new Reg8(0x3));
const std::shared_ptr<Reg8> spl (new Reg8(0x4));
const std::shared_ptr<Reg8> bpl (new Reg8(0x5));
const std::shared_ptr<Reg8> sil (new Reg8(0x6));
const std::shared_ptr<Reg8> dil (new Reg8(0x7));
const std::shared_ptr<Reg8> r8b (new Reg8(0x8));
const std::shared_ptr<Reg8> r9b (new Reg8(0x9));
const std::shared_ptr<Reg8> r10b(new Reg8(0xA));
const std::shared_ptr<Reg8> r11b(new Reg8(0xB));
const std::shared_ptr<Reg8> r12b(new Reg8(0xC));
const std::shared_ptr<Reg8> r13b(new Reg8(0xD));
const std::shared_ptr<Reg8> r14b(new Reg8(0xE));
const std::shared_ptr<Reg8> r15b(new Reg8(0xF));

const std::shared_ptr<Reg16> ax  (new Reg16(0x0));
const std::shared_ptr<Reg16> cx  (new Reg16(0x1));
const std::shared_ptr<Reg16> dx  (new Reg16(0x2));
const std::shared_ptr<Reg16> bx  (new Reg16(0x3));
const std::shared_ptr<Reg16> sp  (new Reg16(0x4));
const std::shared_ptr<Reg16> bp  (new Reg16(0x5));
const std::shared_ptr<Reg16> si  (new Reg16(0x6));
const std::shared_ptr<Reg16> di  (new Reg16(0x7));
const std::shared_ptr<Reg16> r8w (new Reg16(0x8));
const std::shared_ptr<Reg16> r9w (new Reg16(0x9));
const std::shared_ptr<Reg16> r10w(new Reg16(0xA));
const std::shared_ptr<Reg16> r11w(new Reg16(0xB));
const std::shared_ptr<Reg16> r12w(new Reg16(0xC));
const std::shared_ptr<Reg16> r13w(new Reg16(0xD));
const std::shared_ptr<Reg16> r14w(new Reg16(0xE));
const std::shared_ptr<Reg16> r15w(new Reg16(0xF));

const std::shared_ptr<Reg32> eax (new Reg32(0x0));
const std::shared_ptr<Reg32> ecx (new Reg32(0x1));
const std::shared_ptr<Reg32> edx (new Reg32(0x2));
const std::shared_ptr<Reg32> ebx (new Reg32(0x3));
const std::shared_ptr<Reg32> esp (new Reg32(0x4));
const std::shared_ptr<Reg32> ebp (new Reg32(0x5));
const std::shared_ptr<Reg32> esi (new Reg32(0x6));
const std::shared_ptr<Reg32> edi (new Reg32(0x7));
const std::shared_ptr<Reg32> r8d (new Reg32(0x8));
const std::shared_ptr<Reg32> r9d (new Reg32(0x9));
const std::shared_ptr<Reg32> r10d(new Reg32(0xA));
const std::shared_ptr<Reg32> r11d(new Reg32(0xB));
const std::shared_ptr<Reg32> r12d(new Reg32(0xC));
const std::shared_ptr<Reg32> r13d(new Reg32(0xD));
const std::shared_ptr<Reg32> r14d(new Reg32(0xE));
const std::shared_ptr<Reg32> r15d(new Reg32(0xF));

const std::shared_ptr<Reg64> rax(new Reg64(0x0));
const std::shared_ptr<Reg64> rcx(new Reg64(0x1));
const std::shared_ptr<Reg64> rdx(new Reg64(0x2));
const std::shared_ptr<Reg64> rbx(new Reg64(0x3));
const std::shared_ptr<Reg64> rsp(new Reg64(0x4));
const std::shared_ptr<Reg64> rbp(new Reg64(0x5));
const std::shared_ptr<Reg64> rsi(new Reg64(0x6));
const std::shared_ptr<Reg64> rdi(new Reg64(0x7));
const std::shared_ptr<Reg64> r8 (new Reg64(0x8));
const std::shared_ptr<Reg64> r9 (new Reg64(0x9));
const std::shared_ptr<Reg64> r10(new Reg64(0xA));
const std::shared_ptr<Reg64> r11(new Reg64(0xB));
const std::shared_ptr<Reg64> r12(new Reg64(0xC));
const std::shared_ptr<Reg64> r13(new Reg64(0xD));
const std::shared_ptr<Reg64> r14(new Reg64(0xE));
const std::shared_ptr<Reg64> r15(new Reg64(0xF));

inline namespace literals {

std::unique_ptr<Imm8> operator""_imm8(unsigned long long value) {
    return std::make_unique<Imm8>(static_cast<int8_t>(value));
}

std::unique_ptr<Imm16> operator""_imm16(unsigned long long value) {
    return std::make_unique<Imm16>(static_cast<int16_t>(value));
}

std::unique_ptr<Imm32> operator""_imm32(unsigned long long value) {
    return std::make_unique<Imm32>(static_cast<int32_t>(value));
}

std::unique_ptr<Imm64> operator""_imm64(unsigned long long value) {
    return std::make_unique<Imm64>(static_cast<int64_t>(value));
}

std::unique_ptr<FuncRef> operator""_f(const char *value, unsigned long len) {
    return std::make_unique<FuncRef>(value);
}

}
}
