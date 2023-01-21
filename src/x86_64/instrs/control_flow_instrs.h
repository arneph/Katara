//
//  control_flow_instrs.h
//  Katara
//
//  Created by Arne Philipeit on 12/17/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_control_flow_instrs_h
#define x86_64_control_flow_instrs_h

#include <memory>
#include <string>

#include "src/common/data/data_view.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/instrs/instr_cond.h"
#include "src/x86_64/machine_code/linker.h"
#include "src/x86_64/ops.h"

namespace x86_64 {

class Jcc final : public Instr {
 public:
  Jcc(InstrCond cond, BlockRef block_ref) : cond_(cond), dst_(block_ref) {}

  InstrCond cond() const { return cond_; }
  BlockRef dst() const { return dst_; }

  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;

 private:
  InstrCond cond_;
  BlockRef dst_;
};

class Jmp final : public Instr {
 public:
  Jmp(RM rm);
  Jmp(BlockRef block_ref) : dst_(block_ref) {}

  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;

 private:
  Operand dst_;
};

class Call final : public Instr {
 public:
  Call(RM rm);
  Call(FuncRef func_ref) : callee_(func_ref) {}

  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;

 private:
  Operand callee_;
};

class Syscall final : public Instr {
 public:
  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;
};

class Ret final : public Instr {
 public:
  int8_t Encode(Linker& linker, common::data::DataView code) const override;
  std::string ToString() const override;
};

}  // namespace x86_64

#endif /* x86_64_control_flow_instrs_h */
