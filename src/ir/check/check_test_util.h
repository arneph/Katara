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
#include "src/ir/issues/issues.h"
#include "src/ir/serialization/positions.h"
#include "src/ir/serialization/print.h"

namespace ir_check {

using ::common::logging::fail;

template <typename Checker = Checker>
void CheckProgramOrDie(const ir::Program* program) {
  common::positions::FileSet file_set;
  ir_serialization::FilePrintResults print_results =
      ir_serialization::PrintProgramToNewFile("program.ir", program, file_set);
  ir_serialization::ProgramPositions program_positions = print_results.program_positions;
  ir_issues::IssueTracker issue_tracker(&file_set);
  CheckProgram<Checker>(program, program_positions, issue_tracker);
  if (issue_tracker.issues().empty()) {
    return;
  }
  issue_tracker.PrintIssues(common::issues::Format::kTerminal, &std::cerr);
  fail("ir::Program did not pass check");
}

}  // namespace ir_check

#endif /* ir_checker_check_test_util_h */
