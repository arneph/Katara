//
//  cmd.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "cmd.h"

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "src/cmd/build.h"
#include "src/cmd/doc.h"
#include "src/cmd/run.h"

namespace {

constexpr std::string_view kVersion = "0.1";

void PrintHelp(std::ostream& out) {
  out << "Katara is a tool to work with Katara source code.\n"
         "\n"
         "Usage:\n"
         "\n"
         "\tkatara <command> [arguments]\n"
         "\n"
         "The commands are:\n"
         "\n"
         "\tbuild\tbuild Katara packages\n"
         "\tdoc\tgenerate documentation for Katara packages\n"
         "\thelp\tprint this documentation\n"
         "\trun\trun Katara programs\n"
         "\tversion\tprint Katara version\n"
         "\n";
}

void PrintVersion(std::ostream& out) { out << "Katara version " << kVersion << "\n"; }

}  // namespace

namespace cmd {

ErrorCode Execute(int argc, char* argv[], std::istream& in, std::ostream& out, std::ostream& err) {
  if (argc <= 1) {
    PrintHelp(err);
    return ErrorCode::kNoError;
  }
  std::string command(argv[1]);
  std::vector<std::string> args(argv + 2, argv + argc);

  if (command == "build") {
    std::variant<BuildResult, ErrorCode> build_result_or_error = Build(args, err);
    if (std::holds_alternative<ErrorCode>(build_result_or_error)) {
      return std::get<ErrorCode>(build_result_or_error);
    } else {
      return kNoError;
    }
  } else if (command == "doc") {
    return Doc(args, err);
  } else if (command == "run") {
    return Run(args, in, out, err);
  } else if (command == "version") {
    PrintVersion(err);
    return kNoError;
  } else {
    PrintHelp(err);
    return kNoError;
  }
}

}  // namespace cmd
