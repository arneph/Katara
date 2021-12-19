//
//  load.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_load_h
#define cmd_load_h

#include <ostream>
#include <string>
#include <vector>

#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"

namespace cmd {

struct LoadResult {
  std::unique_ptr<lang::packages::PackageManager> pkg_manager;
  std::vector<lang::packages::Package*> arg_pkgs;
  bool generate_debug_info;
  int exit_code;
};

LoadResult Load(const std::vector<std::string> args, std::ostream& err);

}  // namespace cmd

#endif /* cmd_load_h */
