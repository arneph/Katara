//
//  util.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "util.h"

namespace cmd {

std::vector<std::string> ConvertMainArgs(int argc, char* argv[]) {
  return std::vector<std::string>(argv + 1, argv + argc);
}

}  // namespace cmd
