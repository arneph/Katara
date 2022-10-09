//
//  check.h
//  katara-ir
//
//  Created by Arne Philipeit on 8/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef katara_ir_check_h
#define katara_ir_check_h

#include <filesystem>
#include <memory>
#include <variant>
#include <vector>

#include "src/cmd/context.h"
#include "src/cmd/katara-ir/error_codes.h"
#include "src/ir/representation/program.h"

namespace cmd {
namespace katara_ir {

ErrorCode Check(std::vector<std::filesystem::path>& paths, Context* ctx);
std::variant<std::unique_ptr<ir::Program>, ErrorCode> Check(std::filesystem::path path,
                                                            Context* ctx);

}  // namespace katara_ir
}  // namespace cmd

#endif /* katara_ir_check_h */
