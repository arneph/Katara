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

#include "src/lang/processors/issues/issues.h"
#include "src/lang/processors/parser/parser.h"
#include "src/lang/representation/positions/positions.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace lang;

  std::string contents(reinterpret_cast<const char*>(data), size);

  pos::FileSet pos_file_set;
  pos::File* pos_file = pos_file_set.AddFile("test_file.kat", contents);

  scanner::Scanner scanner(pos_file);

  while (scanner.token() != tokens::kEOF) {
    scanner.Next();
  }

  return 0;
}
