//
//  temporary_reg.h
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_temporary_reg_h
#define ir_to_x86_64_translator_temporary_reg_h

#include <optional>

#include "src/ir/representation/instrs.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/ir_translator/context.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

class TemporaryReg {
 public:
  static TemporaryReg ForOperand(x86_64::Operand operand, bool can_use_result_reg,
                                 const ir::Instr* instr, BlockContext& ctx);
  static TemporaryReg Prepare(x86_64::Size x86_64_size, bool can_use_result_reg,
                              const ir::Instr* instr, BlockContext& ctx);
  static TemporaryReg Prepare(x86_64::Reg reg, const ir::Instr* instr, BlockContext& ctx);

  x86_64::Reg reg() const { return reg_; }

  void Restore(BlockContext& ctx);

 private:
  enum class RestorationState {
    kNotNeeded,
    kNeeded,
  };

  static std::optional<TemporaryReg> PrepareFromResultReg(x86_64::Size x86_64_size,
                                                          const ir::Instr* instr,
                                                          BlockContext& ctx);
  static std::optional<TemporaryReg> PrepareFromUsedInFuncButNotLive(x86_64::Size x86_64_size,
                                                                     const ir::Instr* instr,
                                                                     BlockContext& ctx);
  static std::optional<TemporaryReg> PrepareFromUnusedInFunc(x86_64::Size x86_64_size,
                                                             const ir::Instr* instr,
                                                             BlockContext& ctx);
  static std::optional<TemporaryReg> PrepareFromLiveButNotInvolvedInInstr(x86_64::Size x86_64_size,
                                                                          const ir::Instr* instr,
                                                                          BlockContext& ctx);

  TemporaryReg(x86_64::Reg reg, RestorationState restoration)
      : reg_(reg), restoration_(restoration) {}

  x86_64::Reg reg_;
  RestorationState restoration_;
};

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_temporary_reg_h */
