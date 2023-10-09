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
#include "src/ir/serialization/positions.h"

namespace cmd {
namespace katara_ir {

ParseDetails ParseWithDetails(std::filesystem::path path, Context* ctx) {
  ParseDetails result;
  std::string code = ctx->filesystem()->ReadContentsOfFile(path);
  result.program_file = result.file_set.AddFile(path, code);
  auto [program, program_positions] =
      ir_serialization::ParseProgramWithPositions(result.program_file, result.issue_tracker);
  result.program = std::move(program);
  result.program_positions = program_positions;
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
