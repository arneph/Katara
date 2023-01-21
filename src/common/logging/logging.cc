//
//  logging.cc
//  Katara
//
//  Created by Arne Philipeit on 7/23/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "logging.h"

namespace common::logging {

void info(const std::string message) { std::cerr << "internal info: " << message << "\n"; }

void warning(const std::string message) { std::cerr << "internal warning: " << message << "\n"; }

void error(const std::string message) { std::cerr << "internal error: " << message << "\n"; }

[[noreturn]] void fail(const std::string message) {
  std::cerr << "internal error: " << message << "\n";
  std::exit(-1);
}

}  // namespace common::logging
