//
//  format.cc
//  katara-ir
//
//  Created by Arne Philipeit on 8/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "format.h"

#include "src/cmd/katara-ir/check.h"
#include "src/ir/serialization/print.h"

namespace cmd {
namespace katara_ir {

ErrorCode Format(std::vector<std::filesystem::path>& paths, Context* ctx) {
  ErrorCode error_code = ErrorCode::kNoError;
  for (std::filesystem::path path : paths) {
    std::variant<std::unique_ptr<ir::Program>, ErrorCode> program_or_error = Check(path, ctx);
    if (std::holds_alternative<ErrorCode>(program_or_error) && error_code == ErrorCode::kNoError) {
      error_code = std::get<ErrorCode>(program_or_error);
      continue;
    }
    auto program = std::get<std::unique_ptr<ir::Program>>(std::move(program_or_error));
    ctx->filesystem()->WriteFile(path, [&program](std::ostream* stream) {
      *stream << ir_serialization::PrintProgram(program.get());
    });
  }
  return error_code;
}

}  // namespace katara_ir
}  // namespace cmd
