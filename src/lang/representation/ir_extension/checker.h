//
//  checker.h
//  Katara
//
//  Created by Arne Philipeit on 3/20/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_ext_checker_h
#define lang_ir_ext_checker_h

#include <vector>

#include "src/ir/checker/checker.h"
#include "src/ir/checker/issues.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"

namespace lang {
namespace ir_ext {

std::vector<::ir_checker::Issue> CheckProgram(const ir::Program* program);
void AssertProgramIsOkay(const ir::Program* program);

class Checker : public ::ir_checker::Checker {
 private:
  Checker(const ir::Program* program) : ::ir_checker::Checker(program) {}
  ~Checker() {}

  void CheckInstr(const ir::Instr* instr, const ir::Block* block, const ir::Func* func) final;
  void CheckLoadInstr(const ir::LoadInstr* load_instr) final;
  void CheckStoreInstr(const ir::StoreInstr* store_instr) final;

  friend std::vector<::ir_checker::Issue> CheckProgram(const ir::Program* program);
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_checker_h */
