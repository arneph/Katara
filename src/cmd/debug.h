//
//  debug.h
//  Katara
//
//  Created by Arne Philipeit on 2/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef cmd_debug_h
#define cmd_debug_h

#include <filesystem>

#include "src/cmd/context/context.h"

namespace cmd {

struct DebugConfig {
  bool generate_debug_info = false;
  std::filesystem::path debug_path = "debug";
};

class DebugHandler {
 public:
  static DebugHandler& WithDebuggingDisabled();

  DebugHandler(DebugConfig config, Context* ctx) : config_(config), ctx_(ctx) {}

  bool GenerateDebugInfo() const { return config_.generate_debug_info; }
  std::filesystem::path DebugPath() const { return config_.debug_path; }

  void CreateDebugDirectory();
  void CreateDebugSubDirectory(std::string subdir_name);
  void WriteToDebugFile(std::string text, std::string subdir_name, std::string out_file);

 private:
  DebugConfig config_;
  Context* ctx_;
};

}  // namespace cmd

#endif /* cmd_debug_h */
