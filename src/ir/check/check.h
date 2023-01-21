//
//  check.h
//  Katara
//
//  Created by Arne Philipeit on 1/14/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef ir_checker_check_h
#define ir_checker_check_h

#include "src/ir/check/checker.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/program.h"

namespace ir_check {

template <typename Checker = Checker>
void CheckProgram(const ir::Program* program, ir_issues::IssueTracker& issue_tracker) {
  Checker checker(issue_tracker, program);
  checker.CheckProgram();
}

}  // namespace ir_check

#endif /* ir_checker_check_h */
