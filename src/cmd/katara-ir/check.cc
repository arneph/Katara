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
#include "src/ir/check/check.h"

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
  ParseDetails parse_details = ParseWithDetails(path, ctx);
  if (parse_details.program == nullptr) {
    parse_details.issue_tracker.PrintIssues(common::issues::Format::kTerminal, ctx->stderr());
    return parse_details.error_code;
  }
  ir_check::CheckProgram(parse_details.program.get(), parse_details.issue_tracker);
  if (parse_details.issue_tracker.issues().empty()) {
    return std::move(parse_details).program;
  }
  parse_details.issue_tracker.PrintIssues(common::issues::Format::kTerminal, ctx->stderr());
  if (parse_details.error_code != ErrorCode::kNoError) {
    return parse_details.error_code;
  } else {
    return ErrorCode::kCheckFailed;
  }
}

}  // namespace katara_ir
}  // namespace cmd
