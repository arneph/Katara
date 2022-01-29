//
//  load.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_load_h
#define cmd_load_h

#include <string>
#include <variant>
#include <vector>

#include "src/cmd/context/context.h"
#include "src/cmd/error_codes.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"

namespace cmd {

struct LoadResult {
  std::unique_ptr<lang::packages::PackageManager> pkg_manager;
  std::vector<lang::packages::Package*> arg_pkgs;
};

std::variant<LoadResult, ErrorCode> Load(Context* ctx);

}  // namespace cmd

#endif /* cmd_load_h */
