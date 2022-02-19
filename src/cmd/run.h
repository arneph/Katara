//
//  run.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_run_h
#define cmd_run_h

#include <filesystem>
#include <vector>

#include "src/cmd/build.h"
#include "src/cmd/context/context.h"
#include "src/cmd/debug.h"
#include "src/cmd/error_codes.h"

namespace cmd {

ErrorCode Run(std::vector<std::filesystem::path>& paths, BuildOptions& options,
              DebugHandler& debug_handler, Context* ctx);

}

#endif /* cmd_run_h */
