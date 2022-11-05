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
#include "src/lang/processors/parser/parser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace lang;

  std::string contents(reinterpret_cast<const char*>(data), size);

  common::FileSet pos_file_set;
  common::File* pos_file = pos_file_set.AddFile("test_file.kat", contents);
  ast::AST ast;
  ast::ASTBuilder ast_builder = ast.builder();
  issues::IssueTracker issues(&pos_file_set);

  parser::Parser::ParseFile(pos_file, ast_builder, issues);

  return 0;
}
