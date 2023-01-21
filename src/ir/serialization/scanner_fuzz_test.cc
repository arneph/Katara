//
//  scanner_fuzz_test.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include <cstddef>
#include <cstdint>
#include <string>

#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"
#include "src/ir/serialization/scanner.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string contents(reinterpret_cast<const char*>(data), size);

  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("test_file.ir", contents);
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  while (scanner.token() != ir_serialization::Scanner::kEoF) {
    scanner.Next();
  }

  return 0;
}
