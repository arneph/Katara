//
//  run.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef katara_run_h
#define katara_run_h

#include <filesystem>
#include <vector>

#include "src/cmd/context.h"
#include "src/cmd/katara/build.h"
#include "src/cmd/katara/debug.h"
#include "src/cmd/katara/error_codes.h"

namespace cmd {
namespace katara {

ErrorCode Run(std::vector<std::filesystem::path>& paths, BuildOptions& options,
              DebugHandler& debug_handler, Context* ctx);

}
}  // namespace cmd

#endif /* katara_run_h */
