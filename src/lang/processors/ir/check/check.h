//
//  check.h
//  Katara
//
//  Created by Arne Philipeit on 1/14/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_ext_check_h
#define lang_ir_ext_check_h

#include "src/ir/check/check.h"
#include "src/ir/representation/program.h"
#include "src/lang/processors/ir/check/checker.h"

namespace lang::ir_check {

void CheckProgram(const ir::Program* program, ir_issues::IssueTracker& issue_tracker) {
  ::ir_check::CheckProgram<Checker>(program, issue_tracker);
}

}  // namespace lang::ir_check

#endif /* lang_ir_ext_check_h */
