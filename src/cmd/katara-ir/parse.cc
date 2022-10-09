//
//  parse.cc
//  katara-ir
//
//  Created by Arne Philipeit on 8/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "parse.h"

#include <istream>

#include "src/ir/serialization/parse.h"

namespace cmd {
namespace katara_ir {

std::variant<std::unique_ptr<ir::Program>, ErrorCode> Parse(std::filesystem::path path,
                                                            Context* ctx) {
  std::unique_ptr<ir::Program> program;
  ctx->filesystem()->ReadFile(path, [&program](std::istream* stream) {
    program = ir_serialization::ParseProgram(*stream);
  });
  if (program == nullptr) {
    return ErrorCode::kParseFailed;
  } else {
    return std::move(program);
  }
}

}  // namespace katara_ir
}  // namespace cmd
