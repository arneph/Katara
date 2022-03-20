//
//  checker.h
//  Katara
//
//  Created by Arne Philipeit on 2/20/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_checker_checker_h
#define ir_checker_checker_h

#include <vector>

#include "src/ir/checker/issues.h"
#include "src/ir/representation/program.h"

namespace ir_checker {

std::vector<Issue> CheckProgram(const ir::Program* program);
void AssertProgramIsOkay(const ir::Program* program);

}  // namespace ir_checker

#endif /* ir_checker_checker_h */
