//
//  format.h
//  katara-ir
//
//  Created by Arne Philipeit on 8/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef katara_ir_format_h
#define katara_ir_format_h

#include <filesystem>
#include <vector>

#include "src/cmd/context.h"
#include "src/cmd/katara-ir/error_codes.h"

namespace cmd {
namespace katara_ir {

ErrorCode Format(std::vector<std::filesystem::path>& paths, Context* ctx);

}
}  // namespace cmd

#endif /* katara_ir_format_h */
