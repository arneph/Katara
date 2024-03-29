//
//  context.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_context_h
#define cmd_context_h

#include <filesystem>
#include <iostream>
#include <string>

#include "src/common/filesystem/filesystem.h"

namespace cmd {

class Context {
 public:
  virtual ~Context() {}

  virtual common::filesystem::Filesystem* filesystem() = 0;
  virtual std::istream* stdin() = 0;
  virtual std::ostream* stdout() = 0;
  virtual std::ostream* stderr() = 0;

 private:
  void CreateDebugDirectory();
  void CreateDebugSubDirectory(std::string subdir_name);
};

}  // namespace cmd

#endif /* cmd_context_h */
