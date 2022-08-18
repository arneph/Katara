//
//  build.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef katara_build_h
#define katara_build_h

#include <filesystem>
#include <memory>
#include <variant>
#include <vector>

#include "src/cmd/katara/context/context.h"
#include "src/cmd/katara/debug.h"
#include "src/cmd/katara/error_codes.h"
#include "src/ir/representation/program.h"
#include "src/x86_64/program.h"

namespace cmd {

struct BuildOptions {
  bool optimize_ir_ext = true;
  bool optimize_ir = true;
};

std::variant<std::unique_ptr<ir::Program>, ErrorCode> Build(
    std::vector<std::filesystem::path>& paths, BuildOptions& options, DebugHandler& debug_handler,
    Context* ctx);

}  // namespace cmd

#endif /* katara_build_h */
