//
//  debug.h
//  katara-ir
//
//  Created by Arne Philipeit on 8/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef katara_ir_debug_h
#define katara_ir_debug_h

#include <filesystem>
#include <memory>
#include <variant>
#include <vector>

#include "src/cmd/context.h"
#include "src/cmd/katara-ir/error_codes.h"
#include "src/cmd/katara-ir/interpret.h"
#include "src/ir/representation/program.h"

namespace cmd {
namespace katara_ir {

struct DebugOptions {
  bool sanitize = true;
};

ErrorCode Debug(std::filesystem::path path, DebugOptions& debug_options, Context* ctx);

}  // namespace katara_ir
}  // namespace cmd

#endif /* katara_ir_debug_h */
