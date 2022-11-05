//
//  parser_fuzz_test.cpp
//  Katara
//
//  Created by Arne Philipeit on 12/19/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "src/common/positions/positions.h"
#include "src/lang/processors/issues/issues.h"
#include "src/lang/processors/scanner/scanner.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace lang;

  std::string contents(reinterpret_cast<const char*>(data), size);

  common::PosFileSet pos_file_set;
  common::PosFile* pos_file = pos_file_set.AddFile("test_file.kat", contents);

  scanner::Scanner scanner(pos_file);

  while (scanner.token() != tokens::kEOF) {
    scanner.Next();
  }

  return 0;
}
