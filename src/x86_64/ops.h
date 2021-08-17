//
//  ops.h
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_ops_h
#define x86_64_ops_h

#include <cstdint>
#include <string>
#include <variant>

namespace x86_64 {

typedef enum : int8_t {
  k8 = 8,
  k16 = 16,
  k32 = 32,
  k64 = 64,
} Size;

Size Max(Size a, Size b);

class Reg {
 public:
  Reg(Size size, uint8_t reg);

  Size size() const { return size_; }
  int8_t reg() const { return reg_; }

  bool RequiresREX() const;

  // Opcode encoding:
  void EncodeInOpcode(uint8_t* rex, uint8_t* opcode, uint8_t lshift) const;

  // ModRM encoding:
  bool RequiresSIB() const { return false; }
  uint8_t RequiredDispSize() const { return 0; }
  void EncodeInModRM_SIB_Disp(uint8_t* rex, uint8_t* modrm, uint8_t* sib, uint8_t* disp) const;

  // ModRM reg encoding:
  void EncodeInModRMReg(uint8_t* rex, uint8_t* modrm) const;

  std::string ToString() const;

 private:
  Size size_;
  int8_t reg_;
};

Reg Resize(Reg reg, Size new_size);

bool operator==(const Reg& lhs, const Reg& rhs);
bool operator!=(const Reg& lhs, const Reg& rhs);

enum class Scale : uint8_t { kS00 = 0, kS01 = 1, kS10 = 2, kS11 = 3 };

class Mem {
 public:
  static Mem BasePointerDisp(Size size, int32_t disp);

  Mem(Size size, int32_t disp);
  Mem(Size size, uint8_t base_reg, int32_t disp = 0);
  Mem(Size size, uint8_t index_reg, Scale scale, int32_t disp = 0);
  Mem(Size size, uint8_t base_reg, uint8_t index_reg, Scale scale, int32_t disp = 0);

  Size size() const { return size_; }
  uint8_t base_reg() const { return base_reg_; }
  uint8_t index_reg() const { return index_reg_; }
  Scale scale() const { return scale_; }
  int32_t disp() const { return disp_; }

  bool RequiresREX() const;

  // ModRM encoding:
  bool RequiresSIB() const;
  uint8_t RequiredDispSize() const;
  void EncodeInModRM_SIB_Disp(uint8_t* rex, uint8_t* modrm, uint8_t* sib, uint8_t* disp) const;

  std::string ToString() const;

 private:
  Size size_;
  uint8_t base_reg_;
  uint8_t index_reg_;
  Scale scale_;
  int32_t disp_;
};

bool operator==(const Mem& lhs, const Mem& rhs);
bool operator!=(const Mem& lhs, const Mem& rhs);

Mem Resize(Mem mem, Size new_size);

class Imm {
 public:
  Imm(int8_t value) : size_(Size::k8), value_(value) {}
  Imm(int16_t value) : size_(Size::k16), value_(value) {}
  Imm(int32_t value) : size_(Size::k32), value_(value) {}
  Imm(int64_t value) : size_(Size::k64), value_(value) {}

  Size size() const { return size_; }
  int64_t value() const { return value_; }

  bool RequiresREX() const;
  uint8_t RequiredImmSize() const;
  void EncodeInImm(uint8_t* imm) const;

  std::string ToString() const;

 private:
  Size size_;
  int64_t value_;
};

bool operator==(const Imm& lhs, const Imm& rhs);
bool operator!=(const Imm& lhs, const Imm& rhs);

class FuncRef {
 public:
  FuncRef(int64_t func_id) : func_id_(func_id) {}

  int64_t func_id() const { return func_id_; }

  std::string ToString() const { return "<" + std::to_string(func_id_) + ">"; }

 private:
  int64_t func_id_;
};

bool operator==(const FuncRef& lhs, const FuncRef& rhs);
bool operator!=(const FuncRef& lhs, const FuncRef& rhs);

class BlockRef {
 public:
  BlockRef(int64_t block_id) : block_id_(block_id) {}

  int64_t block_id() const { return block_id_; }

  std::string ToString() const { return "BB" + std::to_string(block_id_); }

 private:
  int64_t block_id_;
};

bool operator==(const BlockRef& lhs, const BlockRef& rhs);
bool operator!=(const BlockRef& lhs, const BlockRef& rhs);

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

  Operand(Reg reg) : value_(reg) {}
  Operand(Mem mem) : value_(mem) {}
  Operand(Imm imm) : value_(imm) {}
  Operand(FuncRef func_ref) : value_(func_ref) {}
  Operand(BlockRef block_ref) : value_(block_ref) {}

  Kind kind() const { return Kind(value_.index()); }
  Size size() const;

  bool is_reg() const { return kind() == Kind::kReg; }
  Reg reg() const;

  bool is_mem() const { return kind() == Kind::kMem; }
  Mem mem() const;

  bool is_rm() const { return kind() == Kind::kReg || kind() == Kind::kMem; }
  RM rm() const;

  bool is_imm() const { return kind() == Kind::kImm; }
  Imm imm() const;

  bool is_func_ref() const { return kind() == Kind::kFuncRef; }
  FuncRef func_ref() const;

  bool is_block_ref() const { return kind() == Kind::kBlockRef; }
  BlockRef block_ref() const;

  bool RequiresREX() const;

  std::string ToString() const;

 protected:
  typedef std::variant<Reg, Mem, Imm, FuncRef, BlockRef> operand_t;

  operand_t value_;
};

bool operator==(const Operand& lhs, const Operand& rhs);
bool operator!=(const Operand& lhs, const Operand& rhs);

class RM : public Operand {
 public:
  RM(Reg reg) : Operand(reg) {}
  RM(Mem mem) : Operand(mem) {}

  bool RequiresSIB() const;
  uint8_t RequiredDispSize() const;
  void EncodeInModRM_SIB_Disp(uint8_t* rex, uint8_t* modrm, uint8_t* sib, uint8_t* disp) const;
};

RM Resize(RM reg, Size new_size);

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

}  // namespace x86_64

#endif /* x86_64_ops_h */
