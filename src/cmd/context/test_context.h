//
//  test_context.h
//  Katara
//
//  Created by Arne Philipeit on 1/29/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef cmd_context_test_context_h
#define cmd_context_test_context_h

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "src/cmd/context/context.h"
#include "src/common/filesystem/filesystem.h"
#include "src/common/filesystem/test_filesystem.h"

namespace cmd {

class TestContext : public Context {
 public:
  TestContext(std::string input = "") { stdin_ << input; }

  common::Filesystem* filesystem() override { return &filesystem_; }
  std::istream* stdin() override { return &stdin_; }
  std::ostream* stdout() override { return &stdout_; }
  std::ostream* stderr() override { return &stderr_; }

  std::string output() const { return stdout_.str(); }
  std::string errors() const { return stderr_.str(); }

 private:
  common::TestFilesystem filesystem_;
  std::stringstream stdin_;
  std::stringstream stdout_;
  std::stringstream stderr_;
};

}  // namespace cmd

#endif /* cmd_context_test_context_h */
