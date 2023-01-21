//
//  check_test_util.h
//  Katara
//
//  Created by Arne Philipeit on 1/21/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_ext_check_test_util_h
#define lang_ir_ext_check_test_util_h

#include "src/ir/check/check_test_util.h"
#include "src/ir/representation/program.h"
#include "src/lang/processors/ir/check/checker.h"

namespace lang::ir_check {

void CheckProgramOrDie(const ir::Program* program) {
  ::ir_check::CheckProgramOrDie<Checker>(program);
}

}  // namespace lang::ir_check

#endif /* lang_ir_ext_check_test_util_h */
