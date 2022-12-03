//
//  ir_file_check.cc
//  Katara
//
//  Created by Arne Philipeit on 8/7/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include <filesystem>
#include <fstream>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/representation/program.h"
#include "src/lang/processors/ir/checker/checker.h"
#include "src/lang/processors/ir/serialization/parse.h"

namespace {

TEST(IrFileTest, ProgramsParseAndAreOkay) {
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::recursive_directory_iterator(std::filesystem::current_path())) {
    if (entry.path().extension() != ".ir") continue;
    std::ifstream fstream(entry.path());
    std::stringstream sstream;
    sstream << fstream.rdbuf();
    std::unique_ptr<ir::Program> program = lang::ir_serialization::ParseProgramOrDie(sstream.str());
    lang::ir_checker::AssertProgramIsOkay(program.get());
  }
}

}  // namespace
