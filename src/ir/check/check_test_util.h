//
//  check_test_util.h
//  Katara
//
//  Created by Arne Philipeit on 1/21/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef ir_checker_check_test_util_h
#define ir_checker_check_test_util_h

#include "src/common/logging/logging.h"
#include "src/common/positions/positions.h"
#include "src/ir/check/check.h"

namespace ir_check {

using ::common::logging::fail;

template <typename Checker = Checker>
void CheckProgramOrDie(const ir::Program* program) {
  common::positions::FileSet file_set;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram<Checker>(program, issue_tracker);
  if (issue_tracker.issues().empty()) {
    return;
  }
  issue_tracker.PrintIssues(common::IssuePrintFormat::kTerminal, &std::cerr);
  fail("ir::Program did not pass check");
}

}  // namespace ir_check

#endif /* ir_checker_check_test_util_h */
