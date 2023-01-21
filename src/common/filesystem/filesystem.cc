//
//  filesystem.cc
//  Katara
//
//  Created by Arne Philipeit on 1/29/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "filesystem.h"

#include <fstream>

namespace common::filesystem {

std::string Filesystem::ReadContentsOfFile(std::filesystem::path path) const {
  std::string contents;
  ReadFile(path, [&](std::istream* stream) {
    contents = std::string(std::istreambuf_iterator<char>(*stream), {});
  });
  return contents;
}

void Filesystem::WriteContentsOfFile(std::filesystem::path path, std::string contents) {
  WriteFile(path, [&](std::ostream* stream) { *stream << contents; });
}

}  // namespace common::filesystem
