//
//  context.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "context.h"

#include <fstream>
#include <sstream>

namespace cmd {

Context::Context(std::vector<std::string> args) {
  std::erase_if(args, [this](std::string arg) {
    if (arg == "-d" || arg == "--debug") {
      generate_debug_info_ = true;
      return true;
    } else if (arg.starts_with("-d=")) {
      generate_debug_info_ = true;
      debug_path_ = arg.substr(3);
      return true;
    } else if (arg.starts_with("--debug=")) {
      generate_debug_info_ = true;
      debug_path_ = arg.substr(8);
      return true;
    }
    return false;
  });
  args_ = args;
}

void Context::CreateDebugDirectory() { filesystem()->CreateDirectory(debug_path_); }

void Context::CreateDebugSubDirectory(std::string subdir_name) {
  filesystem()->CreateDirectory(debug_path_ / subdir_name);
}

void Context::WriteToDebugFile(std::string text, std::string subdir_name, std::string out_file) {
  CreateDebugDirectory();
  if (!subdir_name.empty()) {
    CreateDebugSubDirectory(subdir_name);
    filesystem()->WriteContentsOfFile(debug_path_ / subdir_name / out_file, text);
  } else {
    filesystem()->WriteContentsOfFile(debug_path_ / out_file, text);
  }
}

}  // namespace cmd
