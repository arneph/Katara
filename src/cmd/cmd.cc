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

namespace cmd {
namespace {

constexpr std::string_view kVersion = "0.1";

void PrintHelp(Context* ctx) {
  ctx->stdout() << "Katara is a tool to work with Katara source code.\n"
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

void PrintVersion(Context* ctx) { ctx->stdout() << "Katara version " << kVersion << "\n"; }

}  // namespace

ErrorCode Execute(Context* ctx) {
  if (ctx->args().empty()) {
    PrintHelp(ctx);
    return ErrorCode::kNoError;
  }
  std::string command = ctx->args().front();
  ctx->args().erase(ctx->args().begin());

  if (command == "build") {
    std::variant<BuildResult, ErrorCode> build_result_or_error = Build(ctx);
    if (std::holds_alternative<ErrorCode>(build_result_or_error)) {
      return std::get<ErrorCode>(build_result_or_error);
    } else {
      return kNoError;
    }
  } else if (command == "doc") {
    return Doc(ctx);
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
