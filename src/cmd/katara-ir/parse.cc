//
//  parse.cc
//  katara-ir
//
//  Created by Arne Philipeit on 8/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "parse.h"

#include <istream>
#include <string>

#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"
#include "src/ir/serialization/parse.h"

namespace cmd {
namespace katara_ir {

ParseDetails ParseWithDetails(std::filesystem::path path, Context* ctx) {
  ParseDetails result;
  std::string code = ctx->filesystem()->ReadContentsOfFile(path);
  common::positions::File* file = result.file_set.AddFile(path, code);
  result.program = ir_serialization::ParseProgram(file, result.issue_tracker);
  if (result.program == nullptr || !result.issue_tracker.issues().empty()) {
    result.error_code = ErrorCode::kParseFailed;
  } else {
    result.error_code = ErrorCode::kNoError;
  }
  return result;
}

std::variant<std::unique_ptr<ir::Program>, ErrorCode> Parse(std::filesystem::path path,
                                                            Context* ctx) {
  ParseDetails details = ParseWithDetails(path, ctx);
  if (details.error_code == ErrorCode::kNoError) {
    return std::move(details).program;
  } else {
    details.issue_tracker.PrintIssues(common::issues::Format::kTerminal, ctx->stderr());
    return details.error_code;
  }
}

}  // namespace katara_ir
}  // namespace cmd
