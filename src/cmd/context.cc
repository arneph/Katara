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
  if (generate_debug_info_) {
    std::filesystem::create_directory(debug_path_);
  }
}

void Context::CreateDebugSubDirectory(std::string subdir_name) const {
  CreateDirectory(debug_path_ / subdir_name);
}

void Context::WriteToDebugFile(std::string text, std::string subdir_name,
                               std::string out_file) const {
  if (!subdir_name.empty()) {
    CreateDebugSubDirectory(subdir_name);
    WriteToFile(text, debug_path_ / subdir_name / out_file);
  } else {
    WriteToFile(text, debug_path_ / out_file);
  }
}

std::string RealContext::ReadFromFile(std::filesystem::path in_file) const {
  std::ifstream in_stream(in_file, std::ios::in);
  std::ostringstream buffer;
  buffer << in_stream.rdbuf();
  return buffer.str();
}

void RealContext::WriteToFile(std::string text, std::filesystem::path out_file) const {
  std::ofstream out_stream(out_file, std::ios::out);
  out_stream << text;
}

void RealContext::CreateDirectory(std::filesystem::path path) const {
  std::filesystem::create_directory(path);
}

}  // namespace cmd
