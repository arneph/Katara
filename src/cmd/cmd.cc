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

int Execute(int argc, char* argv[], std::istream& in, std::ostream& out, std::ostream& err) {
  if (argc <= 1) {
    PrintHelp(err);
    return 0;
  }
  std::string command(argv[1]);
  std::vector<std::string> args(argv + 2, argv + argc);

  if (command == "build") {
    return Build(args, err).exit_code;
  } else if (command == "doc") {
    return Doc(args, err);
  } else if (command == "run") {
    return Run(args, in, out, err);
  } else if (command == "version") {
    PrintVersion(err);
    return 0;
  } else {
    PrintHelp(err);
    return 0;
  }
}

}  // namespace cmd
