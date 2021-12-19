//
//  util.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "util.h"

#include <filesystem>
#include <fstream>

namespace cmd {

void WriteToFile(std::string text, std::filesystem::path out_file) {
  std::ofstream out_stream(out_file, std::ios::out);
  out_stream << text;
}

}  // namespace cmd
