//
//  interpret.h
//  Katara
//
//  Created by Arne Philipeit on 1/30/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef katara_interpret_h
#define katara_interpret_h

#include <filesystem>
#include <vector>

#include "src/cmd/context.h"
#include "src/cmd/katara/build.h"
#include "src/cmd/katara/debug.h"
#include "src/cmd/katara/error_codes.h"

namespace cmd {

struct InterpretOptions {
  bool sanitize = false;
};

ErrorCode Interpret(std::vector<std::filesystem::path>& paths, BuildOptions& build_options,
                    InterpretOptions& interpret_options, DebugHandler& debug_handler, Context* ctx);

}  // namespace cmd

#endif /* katara_interpret_h */
