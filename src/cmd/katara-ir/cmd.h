//
//  cmd.h
//  Katara
//
//  Created by Arne Philipeit on 08/18/22.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef katara_ir_cmd_h
#define katara_ir_cmd_h

#include <string>
#include <vector>

#include "src/cmd/context.h"
#include "src/cmd/katara-ir/error_codes.h"

namespace cmd {
namespace katara_ir {

ErrorCode Execute(std::vector<std::string> args, Context* ctx);

}
}  // namespace cmd

#endif /* katara_ir_cmd_h */
