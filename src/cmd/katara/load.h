//
//  load.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef katara_load_h
#define katara_load_h

#include <filesystem>
#include <variant>
#include <vector>

#include "src/cmd/context.h"
#include "src/cmd/katara/debug.h"
#include "src/cmd/katara/error_codes.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"

namespace cmd {
namespace katara {

struct LoadResult {
  std::unique_ptr<lang::packages::PackageManager> pkg_manager;
  std::vector<lang::packages::Package*> arg_pkgs;
};

std::variant<LoadResult, ErrorCode> Load(std::vector<std::filesystem::path>& paths,
                                         DebugHandler& debug_handler, Context* ctx);

}  // namespace katara
}  // namespace cmd

#endif /* katara_load_h */
