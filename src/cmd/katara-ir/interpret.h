//
//  interpret.h
//  Katara
//
//  Created by Arne Philipeit on 08/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef katara_ir_interpret_h
#define katara_ir_interpret_h

#include <filesystem>

#include "src/cmd/context.h"
#include "src/cmd/katara-ir/error_codes.h"

namespace cmd {
namespace katara_ir {

struct InterpretOptions {
  bool sanitize = false;
};

ErrorCode Interpret(std::filesystem::path path, InterpretOptions& interpret_options, Context* ctx);

}  // namespace katara_ir
}  // namespace cmd

#endif /* katara_ir_interpret_h */
