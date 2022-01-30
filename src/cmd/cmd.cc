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
#include "src/cmd/interpret.h"
#include "src/cmd/run.h"

namespace cmd {
namespace {

constexpr std::string_view kVersion = "0.1";

void PrintHelp(Context* ctx) {
  *ctx->stdout() << "Katara is a tool to work with Katara source code.\n"
                    "\n"
                    "Usage:\n"
                    "\n"
                    "\tkatara <command> [arguments]\n"
                    "\n"
                    "The commands are:\n"
                    "\n"
                    "\tbuild     build Katara packages\n"
                    "\tdoc       generate documentation for Katara packages\n"
                    "\tinterpret build and interpret Katara programs\n"
                    "\thelp      print this documentation\n"
                    "\trun       build and run Katara programs\n"
                    "\tversion   print Katara version\n"
                    "\n";
}

void PrintVersion(Context* ctx) { *ctx->stdout() << "Katara version " << kVersion << "\n"; }

}  // namespace

ErrorCode Execute(Context* ctx) {
  if (ctx->args().empty()) {
    PrintHelp(ctx);
    return ErrorCode::kNoError;
  }
  std::string command = ctx->args().front();
  ctx->args().erase(ctx->args().begin());

  if (command == "build") {
    std::variant<std::unique_ptr<ir::Program>, ErrorCode> program_or_error = Build(ctx);
    if (std::holds_alternative<ErrorCode>(program_or_error)) {
      return std::get<ErrorCode>(program_or_error);
    } else {
      return kNoError;
    }
  } else if (command == "doc") {
    return Doc(ctx);
  } else if (command == "interpret") {
    return Interpret(ctx);
  } else if (command == "run") {
    return Run(ctx);
  } else if (command == "version") {
    PrintVersion(ctx);
    return kNoError;
  } else {
    PrintHelp(ctx);
    return kNoError;
  }
}

}  // namespace cmd
