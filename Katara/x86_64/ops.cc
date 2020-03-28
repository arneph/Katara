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

Reg::Reg(Size size, uint8_t reg) : size_(size), reg_(reg) {
    if (reg > 15)
        throw "register out of bounds: " + std::to_string(reg);
}

Size Reg::size() const {
    return size_;
}

int8_t Reg::reg() const {
    return reg_;
}

bool Reg::RequiresREX() const {
    switch (size_) {
        case Size::k8:
            // 32bit: ah, ch, dh, lh
            // 64bit: spl, bpl, sil, dil
            return reg_ >= 4;
        case Size::k16:
        case Size::k32:
        case Size::k64:
            return reg_ >= 8;
        default:
            throw "unexpected reg size";
    }
    return reg_ >= 8;
}

void Reg::EncodeInOpcode(uint8_t *rex,
                         uint8_t *opcode,
                         uint8_t ls) const {
    if (reg_ >= 8) {
        *rex |= 0x01;               // REX.B
    }
    *opcode &= ~0x7 << ls;          // Reset Reg
    *opcode |= (reg_ & 0x7) << ls;  // Reg = reg
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
        *rex |= 0x01;            // REX.B
    }
    *modrm |= 0xc0;              // Mod = 11
    *modrm &= ~0x07;             // Reset R/M
    *modrm |= reg_ & 0x7;        // R/M = reg lower bits
}

void Reg::EncodeInModRMReg(uint8_t *rex,
                           uint8_t *modrm) const {
    if (reg_ >= 8) {
        *rex |= 0x04;            // REX.R
    }
    *modrm &= ~0x38;             // Reset Reg
    *modrm |= (reg_ & 0x7) << 3; // Reg = reg lower bits
}

std::string Reg::ToString() const {
    switch (size_) {
        case Size::k8:
            switch (reg_) {
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
                    throw "unknown register index";
            }
        case Size::k16:
            switch (reg_) {
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
                    throw "unknown register index";
            }
        case Size::k32:
            switch (reg_) {
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
                    throw "unknown register index";
            }
        case Size::k64:
            switch (reg_) {
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
                    throw "unknown register index";
            }
        default:
            throw "unknown register size";
    }
}

bool operator==(const Reg& lhs,
                const Reg& rhs) {
    return lhs.size() == rhs.size()
        && lhs.reg() == rhs.reg();
}

bool operator!=(const Reg& lhs,
                const Reg& rhs) {
    return !(lhs == rhs);
}

Mem::Mem(Size size, int32_t disp)
    : Mem(size, 0xff, 0xff, Scale::kS00, disp) {}

Mem::Mem(Size size, uint8_t base_reg, int32_t disp)
    : Mem(size, base_reg, 0xff, Scale::kS00, disp) {}

Mem::Mem(Size size, uint8_t index_reg, Scale scale, int32_t disp)
    : Mem(size, 0xff, index_reg, scale, disp) {}

Mem::Mem(Size size, uint8_t base_reg,
         uint8_t index_reg, Scale scale, int32_t disp)
    : size_(size), base_reg_(base_reg),
      index_reg_(index_reg), scale_(scale), disp_(disp) {
    if (base_reg > 15 && base_reg != 0xff) {
        throw "register out of bounds: " + std::to_string(base_reg);
    }
    if (index_reg > 15 && index_reg != 0xff) {
        throw "register out bounds: " + std::to_string(index_reg);
    } else if (index_reg == 4) {
        throw "index register can't be stack pointer";
    }
}

Size Mem::size() const {
    return size_;
}

uint8_t Mem::base_reg() const {
    return base_reg_;
}

uint8_t Mem::index_reg() const {
    return index_reg_;
}

Scale Mem::scale() const {
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
    uint8_t s = static_cast<uint8_t>(scale_);
    
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
            *sib |= (s & 0x3) << 6;         // S = scale
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
            *sib |= (s & 0x3) << 6;         // S = scale
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
    uint8_t s = static_cast<uint8_t>(scale_);
    
    std::stringstream ss;
    ss << "[";
    bool added_term = false;
    if (base_reg_ != 0xff) {
        ss << Reg(Size::k64, base_reg_).ToString();
        added_term = true;
    }
    if (index_reg_ != 0xff) {
        if (added_term) ss << "+";
        ss << std::to_string(1 << s);
        ss << "*";
        ss << Reg(Size::k64, index_reg_).ToString();
    }
    if (disp_ != 0) {
        ss << std::showpos << disp_;
    }
    ss << "]";
    return ss.str();
}

bool operator==(const Mem& lhs,
                const Mem& rhs) {
    return lhs.size() == rhs.size()
        && lhs.base_reg() == rhs.base_reg()
        && lhs.index_reg() == rhs.index_reg()
        && lhs.scale() == rhs.scale()
        && lhs.disp() == rhs.disp();
}

bool operator!=(const Mem& lhs,
                const Mem& rhs) {
    return !(lhs == rhs);
}

Imm::Imm(int8_t value) : size_(Size::k8), value_(value) {}
Imm::Imm(int16_t value) : size_(Size::k16), value_(value) {}
Imm::Imm(int32_t value) : size_(Size::k32), value_(value) {}
Imm::Imm(int64_t value) : size_(Size::k64), value_(value) {}

Size Imm::size() const {
    return size_;
}

int64_t Imm::value() const {
    return value_;
}

bool Imm::RequiresREX() const {
    switch (size_) {
        case Size::k8:
        case Size::k16:
        case Size::k32:
            return false;
        case Size::k64:
            return true;
        default:
            throw "unexpected imm size";
    }
}

uint8_t Imm::RequiredImmSize() const {
    switch (size_) {
        case Size::k8:
            return 1;
        case Size::k16:
            return 2;
        case Size::k32:
            return 4;
        case Size::k64:
            return 8;
        default:
            throw "unexpected imm size";
    }
}

void Imm::EncodeInImm(uint8_t *imm) const {
    switch (size_) {
        case Size::k8:
            imm[0] = static_cast<uint8_t>(value_);
            return;
        case Size::k16:
            imm[0] = static_cast<uint8_t>(value_ >> 0) & 0xff;
            imm[1] = static_cast<uint8_t>(value_ >> 8) & 0xff;
            return;
        case Size::k32:
            imm[0] = static_cast<uint8_t>(value_ >>  0) & 0xff;
            imm[1] = static_cast<uint8_t>(value_ >>  8) & 0xff;
            imm[2] = static_cast<uint8_t>(value_ >> 16) & 0xff;
            imm[3] = static_cast<uint8_t>(value_ >> 24) & 0xff;
            return;
        case Size::k64:
            imm[0] = static_cast<uint8_t>(value_ >> uint64_t{ 0}) & 0xff;
            imm[1] = static_cast<uint8_t>(value_ >> uint64_t{ 8}) & 0xff;
            imm[2] = static_cast<uint8_t>(value_ >> uint64_t{16}) & 0xff;
            imm[3] = static_cast<uint8_t>(value_ >> uint64_t{24}) & 0xff;
            imm[4] = static_cast<uint8_t>(value_ >> uint64_t{32}) & 0xff;
            imm[5] = static_cast<uint8_t>(value_ >> uint64_t{40}) & 0xff;
            imm[6] = static_cast<uint8_t>(value_ >> uint64_t{48}) & 0xff;
            imm[7] = static_cast<uint8_t>(value_ >> uint64_t{56}) & 0xff;
            return;
        default:
            throw "unexpected imm size";
    }
}

std::string Imm::ToString() const {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0');
    switch (size_) {
        case Size::k8:
            ss << std::setw(2) << static_cast<int32_t>(value_);
            break;
        case Size::k16:
            ss << std::setw(4) << static_cast<int32_t>(value_);
            break;
        case Size::k32:
            ss << std::setw(8) << static_cast<int32_t>(value_);
            break;
        case Size:: k64:
            ss << std::setw(16) << value_;
            break;
        default:
            throw "unexpected imm size";
    }
    return ss.str();
}

bool operator==(const Imm& lhs,
                const Imm& rhs) {
    return lhs.size() == rhs.size()
        && lhs.value() == rhs.value();
}

bool operator!=(const Imm& lhs,
                const Imm& rhs) {
    return !(lhs == rhs);
}

FuncRef::FuncRef(int64_t func_id) : func_id_(func_id) {}

int64_t FuncRef::func_id() const {
    return func_id_;
}

std::string FuncRef::ToString() const {
    return "<" + std::to_string(func_id_) + ">";
}

bool operator==(const FuncRef& lhs,
                const FuncRef& rhs) {
    return lhs.func_id() == rhs.func_id();
}

bool operator!=(const FuncRef& lhs,
                const FuncRef& rhs) {
    return !(lhs == rhs);
}

BlockRef::BlockRef(int64_t block_id) : block_id_(block_id) {}

int64_t BlockRef::block_id() const {
    return block_id_;
}

std::string BlockRef::ToString() const {
    return "BB" + std::to_string(block_id_);
}

bool operator==(const BlockRef& lhs,
                const BlockRef& rhs) {
    return lhs.block_id() == rhs.block_id();
}

bool operator!=(const BlockRef& lhs,
                const BlockRef& rhs) {
    return !(lhs == rhs);
}

Operand::Operand(Reg reg) : kind_(Kind::kReg), data_(reg) {}
Operand::Operand(Mem mem) : kind_(Kind::kMem), data_(mem) {}
Operand::Operand(Imm imm) : kind_(Kind::kImm), data_(imm) {}
Operand::Operand(FuncRef func_ref)
    : kind_(Kind::kFuncRef), data_(func_ref) {}
Operand::Operand(BlockRef block_ref)
    : kind_(Kind::kBlockRef), data_(block_ref) {}

Operand::Kind Operand::kind() const {
    return kind_;
}

Size Operand::size() const {
    switch (kind_) {
        case Kind::kReg: return data_.reg.size();
        case Kind::kMem: return data_.mem.size();
        case Kind::kImm: return data_.imm.size();
        default:
            throw "size operation not supported for operand kind";
    }
}

bool Operand::is_reg() const {
    return kind_ == Kind::kReg;
}

Reg Operand::reg() const {
    if (kind_ != Kind::kReg)
        throw "attempted to obtain reg from non-reg operand";
    return data_.reg;
}

bool Operand::is_mem() const {
    return kind_ == Kind::kMem;
}

Mem Operand::mem() const {
    if (kind_ != Kind::kMem)
        throw "attempted to obtain mem from non-mem operand";
    return data_.mem;
}

bool Operand::is_rm() const {
    return kind_ == Kind::kReg || kind_ == Kind::kMem;
}

RM Operand::rm() const {
    if (kind_ == Kind::kReg) {
        return RM(data_.reg);
    } else if (kind_ == Kind::kMem) {
        return RM(data_.mem);
    } else {
        throw "attempted to obtain rm from non-rm operand";
    }
}

bool Operand::is_imm() const {
    return kind_ == Kind::kImm;
}

Imm Operand::imm() const {
    if (kind_ != Kind::kImm)
        throw "attempted to obtain imm from non-imm operand";
    return data_.imm;
}

bool Operand::is_func_ref() const {
    return kind_ == Kind::kFuncRef;
}

FuncRef Operand::func_ref() const {
    if (kind_ != Kind::kFuncRef)
        throw "attempted to obtain func-ref from non-func-ref operand";
    return data_.func_ref;
}

bool Operand::is_block_ref() const {
    return kind_ == Kind::kBlockRef;
}

BlockRef Operand::block_ref() const {
    if (kind_ != Kind::kBlockRef)
        throw "attempted to obtain block-ref from non-block-ref operand";
    return data_.block_ref;
}

bool Operand::RequiresREX() const {
    switch (kind_) {
        case Kind::kReg: return data_.reg.RequiresREX();
        case Kind::kMem: return data_.mem.RequiresREX();
        case Kind::kImm: return data_.imm.RequiresREX();
        case Kind::kBlockRef:
        case Kind::kFuncRef:
            return false;
        default:
            throw "unexpected operand kind";
    }
}

std::string Operand::ToString() const {
    switch (kind_) {
        case Kind::kReg: return data_.reg.ToString();
        case Kind::kMem: return data_.mem.ToString();
        case Kind::kImm: return data_.imm.ToString();
        case Kind::kBlockRef: return data_.block_ref.ToString();
        case Kind::kFuncRef: return data_.func_ref.ToString();
        default:
            throw "unexpected operand kind";
    }
}

bool operator==(const Operand& lhs,
                const Operand& rhs) {
    if (lhs.kind() != rhs.kind()) {
        return false;
    }
    switch (lhs.kind()) {
        case Operand::Kind::kReg:
            return lhs.reg() == rhs.reg();
        case Operand::Kind::kMem:
            return lhs.mem() == rhs.mem();
        case Operand::Kind::kImm:
            return lhs.imm() == rhs.imm();
        case Operand::Kind::kFuncRef:
            return lhs.func_ref() == rhs.func_ref();
        case Operand::Kind::kBlockRef:
            return lhs.block_ref() == rhs.block_ref();
    }
}

bool operator!=(const Operand& lhs,
                const Operand& rhs) {
    return !(lhs == rhs);
}

RM::RM(Reg reg) : Operand(reg) {}
RM::RM(Mem mem) : Operand(mem) {}

bool RM::RequiresSIB() const {
    switch (kind_) {
        case Kind::kReg: return data_.reg.RequiresSIB();
        case Kind::kMem: return data_.mem.RequiresSIB();
        default:
            throw "unexpected rm kind";
    }
}

uint8_t RM::RequiredDispSize() const {
    switch (kind_) {
        case Kind::kReg: return data_.reg.RequiredDispSize();
        case Kind::kMem: return data_.mem.RequiredDispSize();
        default:
            throw "unexpected rm kind";
    }
}

void RM::EncodeInModRM_SIB_Disp(uint8_t *rex,
                                uint8_t *modrm,
                                uint8_t *sib,
                                uint8_t *disp) const {
    switch (kind()) {
        case Operand::Kind::kReg:
            data_.reg.EncodeInModRM_SIB_Disp(rex, modrm, sib, disp);
            break;
        case Operand::Kind::kMem:
            data_.mem.EncodeInModRM_SIB_Disp(rex, modrm, sib, disp);
            break;
        default:
            throw "unexpected rm kind";
    }
}


const Reg al(Size::k8, 0);
const Reg cl(Size::k8, 1);
const Reg dl(Size::k8, 2);
const Reg bl(Size::k8, 3);
const Reg spl(Size::k8, 4);
const Reg bpl(Size::k8, 5);
const Reg sil(Size::k8, 6);
const Reg dil(Size::k8, 7);
const Reg r8b(Size::k8, 8);
const Reg r9b(Size::k8, 9);
const Reg r10b(Size::k8, 10);
const Reg r11b(Size::k8, 11);
const Reg r12b(Size::k8, 12);
const Reg r13b(Size::k8, 13);
const Reg r14b(Size::k8, 14);
const Reg r15b(Size::k8, 15);

const Reg ax(Size::k16, 0);
const Reg cx(Size::k16, 1);
const Reg dx(Size::k16, 2);
const Reg bx(Size::k16, 3);
const Reg sp(Size::k16, 4);
const Reg bp(Size::k16, 5);
const Reg si(Size::k16, 6);
const Reg di(Size::k16, 7);
const Reg r8w(Size::k16, 8);
const Reg r9w(Size::k16, 9);
const Reg r10w(Size::k16, 10);
const Reg r11w(Size::k16, 11);
const Reg r12w(Size::k16, 12);
const Reg r13w(Size::k16, 13);
const Reg r14w(Size::k16, 14);
const Reg r15w(Size::k16, 15);

const Reg eax(Size::k32, 0);
const Reg ecx(Size::k32, 1);
const Reg edx(Size::k32, 2);
const Reg ebx(Size::k32, 3);
const Reg esp(Size::k32, 4);
const Reg ebp(Size::k32, 5);
const Reg esi(Size::k32, 6);
const Reg edi(Size::k32, 7);
const Reg r8d(Size::k32, 8);
const Reg r9d(Size::k32, 9);
const Reg r10d(Size::k32, 10);
const Reg r11d(Size::k32, 11);
const Reg r12d(Size::k32, 12);
const Reg r13d(Size::k32, 13);
const Reg r14d(Size::k32, 14);
const Reg r15d(Size::k32, 15);

const Reg rax(Size::k64, 0);
const Reg rcx(Size::k64, 1);
const Reg rdx(Size::k64, 2);
const Reg rbx(Size::k64, 3);
const Reg rsp(Size::k64, 4);
const Reg rbp(Size::k64, 5);
const Reg rsi(Size::k64, 6);
const Reg rdi(Size::k64, 7);
const Reg r8(Size::k64, 8);
const Reg r9(Size::k64, 9);
const Reg r10(Size::k64, 10);
const Reg r11(Size::k64, 11);
const Reg r12(Size::k64, 12);
const Reg r13(Size::k64, 13);
const Reg r14(Size::k64, 14);
const Reg r15(Size::k64, 15);

}
