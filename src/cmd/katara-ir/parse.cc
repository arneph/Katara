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

std::variant<std::unique_ptr<ir::Program>, ErrorCode> Parse(std::filesystem::path path,
                                                            Context* ctx) {
  std::string code = ctx->filesystem()->ReadContentsOfFile(path);
  common::PosFileSet file_set;
  common::PosFile* file = file_set.AddFile(path, code);
  ir_issues::IssueTracker issue_tracker(&file_set);
  std::unique_ptr<ir::Program> program = ir_serialization::ParseProgram(file, issue_tracker);
  issue_tracker.PrintIssues(common::IssuePrintFormat::kTerminal, ctx->stderr());
  if (program == nullptr || !issue_tracker.issues().empty()) {
    return ErrorCode::kParseFailed;
  } else {
    return std::move(program);
  }
}

}  // namespace katara_ir
}  // namespace cmd
