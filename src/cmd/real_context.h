//
//  real_context.h
//  Katara
//
//  Created by Arne Philipeit on 1/29/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef cmd_real_context_h
#define cmd_real_context_h

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "src/cmd/context.h"
#include "src/common/filesystem/filesystem.h"
#include "src/common/filesystem/real_filesystem.h"

namespace cmd {

class RealContext : public Context {
 public:
  common::filesystem::Filesystem* filesystem() override { return &filesystem_; }
  std::istream* stdin() override { return &std::cin; }
  std::ostream* stdout() override { return &std::cout; }
  std::ostream* stderr() override { return &std::cerr; }

 private:
  common::filesystem::RealFilesystem filesystem_;
};

}  // namespace cmd

#endif /* cmd_real_context_h */
