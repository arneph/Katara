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

#include "src/common/data/data_view.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/instrs/instr_cond.h"
#include "src/x86_64/machine_code/linker.h"
#include "src/x86_64/ops.h"

namespace x86_64 {

class Mov final : public Instr {
 public:
  Mov(RM dst, Operand src);

  RM dst() const { return dst_; }
  Operand src() const { return src_; }

  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;

 private:
  typedef enum : uint8_t { kRM_REG, kREG_RM, kREG_IMM, kRM_IMM, kREG_FuncRef, kRM_FuncRef } MovType;

  MovType mov_type_;
  RM dst_;
  Operand src_;
};

class Xchg final : public Instr {
 public:
  Xchg(RM rm, Reg reg);

  RM op_a() const { return op_a_; }
  Reg op_b() const { return op_b_; }

  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;

 private:
  bool CanUseRegAShortcut() const;

  RM op_a_;
  Reg op_b_;
};

class Push final : public Instr {
 public:
  Push(RM rm);
  Push(Imm imm);

  Operand op() const { return op_; }

  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;

 private:
  Operand op_;
};

class Pop final : public Instr {
 public:
  Pop(RM rm);

  RM op() const { return op_; }

  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;

 private:
  RM op_;
};

class Setcc final : public Instr {
 public:
  Setcc(InstrCond cond, RM op);

  InstrCond cond() const { return cond_; }
  RM op() const { return op_; }

  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;

 private:
  InstrCond cond_;
  RM op_;
};

}  // namespace x86_64

#endif /* x86_64_data_instrs_h */
