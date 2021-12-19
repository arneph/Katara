//
//  run.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_run_h
#define cmd_run_h

#include <iostream>
#include <string>
#include <vector>

#include "src/cmd/error_codes.h"

namespace cmd {

ErrorCode Run(const std::vector<std::string> args, std::istream& in, std::ostream& out,
              std::ostream& err);

}

#endif /* cmd_run_h */
