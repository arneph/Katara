//
//  context.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_context_context_h
#define cmd_context_context_h

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "src/common/filesystem/filesystem.h"

namespace cmd {

class Context {
 public:
  virtual ~Context() {}

  std::vector<std::string>& args() { return args_; }

  bool generate_debug_info() const { return generate_debug_info_; }
  std::filesystem::path debug_path() const { return debug_path_; }

  virtual common::Filesystem* filesystem() = 0;
  virtual std::istream* stdin() = 0;
  virtual std::ostream* stdout() = 0;
  virtual std::ostream* stderr() = 0;

  void WriteToDebugFile(std::string text, std::string subdir_name, std::string file_name);

 protected:
  Context(std::vector<std::string> args);

 private:
  void CreateDebugDirectory();
  void CreateDebugSubDirectory(std::string subdir_name);

  std::vector<std::string> args_;

  bool generate_debug_info_ = false;
  std::filesystem::path debug_path_ = "debug";
};

}  // namespace cmd

#endif /* cmd_context_context_h */
