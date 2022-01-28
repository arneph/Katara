//
//  logging.h
//  Katara
//
//  Created by Arne Philipeit on 7/23/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef common_logging_h
#define common_logging_h

#include <iostream>
#include <string>

namespace common {

void info(const std::string message);
void warning(const std::string message);
void error(const std::string message);
[[noreturn]] void fail(const std::string message);

}  // namespace common

#endif /* common_logging_h */
