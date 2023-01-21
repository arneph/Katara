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

#include "src/ir/check/checker.h"
#include "src/ir/issues/issues.h"
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

namespace lang::ir_check {

class Checker : public ::ir_check::Checker {
 public:
  Checker(ir_issues::IssueTracker& issue_tracker, const ir::Program* program)
      : ::ir_check::Checker(issue_tracker, program) {}
  ~Checker() = default;

 private:
  void CheckInstr(const ir::Instr* instr, const ir::Block* block, const ir::Func* func) final;
  void CheckMakeSharedPointerInstr(const ir_ext::MakeSharedPointerInstr* make_shared_pointer_instr);
  void CheckCopySharedPointerInstr(const ir_ext::CopySharedPointerInstr* copy_shared_pointer_instr);
  void CheckDeleteSharedPointerInstr(
      const ir_ext::DeleteSharedPointerInstr* delete_shared_pointer_instr);
  void CheckMakeUniquePointerInstr(const ir_ext::MakeUniquePointerInstr* make_unique_pointer_instr);
  void CheckDeleteUniquePointerInstr(
      const ir_ext::DeleteUniquePointerInstr* delete_unique_pointer_instr);
  void CheckLoadInstr(const ir::LoadInstr* load_instr) final;
  void CheckStoreInstr(const ir::StoreInstr* store_instr) final;

  void CheckMovInstr(const ir::MovInstr* mov_instr) final;

  void CheckStringIndexInstr(const ir_ext::StringIndexInstr* string_index_instr);
  void CheckStringConcatInstr(const ir_ext::StringConcatInstr* string_concat_instr);
};

}  // namespace lang::ir_check

#endif /* lang_ir_ext_checker_h */
