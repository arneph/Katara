//
//  check.cc
//  katara-ir
//
//  Created by Arne Philipeit on 8/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "check.h"

#include <vector>

#include "src/cmd/katara-ir/parse.h"
#include "src/ir/checker/checker.h"

namespace cmd {
namespace katara_ir {

ErrorCode Check(std::vector<std::filesystem::path>& paths, Context* ctx) {
  ErrorCode error_code = ErrorCode::kNoError;
  for (std::filesystem::path path : paths) {
    std::variant<std::unique_ptr<ir::Program>, ErrorCode> program_or_error = Check(path, ctx);
    if (std::holds_alternative<ErrorCode>(program_or_error) && error_code == ErrorCode::kNoError) {
      error_code = std::get<ErrorCode>(program_or_error);
    }
  }
  return error_code;
}

std::variant<std::unique_ptr<ir::Program>, ErrorCode> Check(std::filesystem::path path,
                                                            Context* ctx) {
  std::variant<std::unique_ptr<ir::Program>, ErrorCode> program_or_error = Parse(path, ctx);
  if (std::holds_alternative<ErrorCode>(program_or_error)) {
    return std::get<ErrorCode>(program_or_error);
  }
  auto program = std::get<std::unique_ptr<ir::Program>>(std::move(program_or_error));
  std::vector<ir_checker::Issue> issues = ir_checker::CheckProgram(program.get());
  if (issues.empty()) {
    return std::move(program);
  } else {
    for (ir_checker::Issue& issue : issues) {
      *ctx->stderr() << issue.ToDetailedString();
    }
    return ErrorCode::kCheckFailed;
  }
}

}  // namespace katara_ir
}  // namespace cmd
