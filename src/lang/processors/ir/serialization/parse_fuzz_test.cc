//
//  parse_fuzz_test.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/program.h"
#include "src/lang/processors/ir/serialization/parse.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string contents(reinterpret_cast<const char*>(data), size);

  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("test_file.ir", contents);
  ir_issues::IssueTracker issue_tracker(&file_set);
  std::unique_ptr<ir::Program> program = lang::ir_serialization::ParseProgram(file, issue_tracker);

  return 0;
}
