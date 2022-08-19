//
//  debug.cc
//  Katara
//
//  Created by Arne Philipeit on 2/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "debug.h"

namespace cmd {
namespace katara {

DebugHandler& DebugHandler::WithDebugEnabledButOutputDisabled() {
  static DebugHandler handler = DebugHandler(DebugConfig{.check_ir = true}, /*ctx=*/nullptr);
  return handler;
}

void DebugHandler::CreateDebugDirectory() { ctx_->filesystem()->CreateDirectory(DebugPath()); }

void DebugHandler::CreateDebugSubDirectory(std::string subdir_name) {
  ctx_->filesystem()->CreateDirectory(DebugPath() / subdir_name);
}

void DebugHandler::WriteToDebugFile(std::string text, std::string subdir_name,
                                    std::string out_file) {
  CreateDebugDirectory();
  if (!subdir_name.empty()) {
    CreateDebugSubDirectory(subdir_name);
    ctx_->filesystem()->WriteContentsOfFile(DebugPath() / subdir_name / out_file, text);
  } else {
    ctx_->filesystem()->WriteContentsOfFile(DebugPath() / out_file, text);
  }
}

}  // namespace katara
}  // namespace cmd
