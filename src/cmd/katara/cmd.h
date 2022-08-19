//
//  cmd.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef katara_cmd_h
#define katara_cmd_h

#include <string>
#include <vector>

#include "src/cmd/context.h"
#include "src/cmd/katara/error_codes.h"

namespace cmd {
namespace katara {

ErrorCode Execute(std::vector<std::string> args, Context* ctx);

}  // namespace katara
}  // namespace cmd

#endif /* katara_cmd_h */
