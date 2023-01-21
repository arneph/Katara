//
//  parse.h
//  katara-ir
//
//  Created by Arne Philipeit on 8/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef katara_ir_parse_h
#define katara_ir_parse_h

#include <filesystem>
#include <memory>
#include <variant>

#include "src/cmd/context.h"
#include "src/cmd/katara-ir/error_codes.h"
#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/program.h"

namespace cmd {
namespace katara_ir {

struct ParseDetails {
  ErrorCode error_code;
  std::unique_ptr<ir::Program> program;
  common::PosFileSet file_set;
  ir_issues::IssueTracker issue_tracker = ir_issues::IssueTracker(&file_set);
};

ParseDetails ParseWithDetails(std::filesystem::path path, Context* ctx);
std::variant<std::unique_ptr<ir::Program>, ErrorCode> Parse(std::filesystem::path path,
                                                            Context* ctx);

}  // namespace katara_ir
}  // namespace cmd

#endif /* katara_ir_parse_h */
