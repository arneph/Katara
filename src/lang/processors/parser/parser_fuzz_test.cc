//
//  parser_fuzz_test.cpp
//  Katara
//
//  Created by Arne Philipeit on 12/19/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include "lang/representation/positions/positions.h"
#include "lang/processors/issues/issues.h"
#include "lang/processors/parser/parser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    using namespace lang;
    
    std::string contents(reinterpret_cast<const char*>(data), size);
    
    pos::FileSet pos_file_set;
    pos::File *pos_file = pos_file_set.AddFile("test_file.kat", contents);
    ast::AST ast;
    ast::ASTBuilder ast_builder = ast.builder();
    std::vector<issues::Issue> issues;
    
    parser::Parser::ParseFile(pos_file, ast_builder, issues);
    
    return 0;
}
