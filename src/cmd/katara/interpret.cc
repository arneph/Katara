//
//  interpret.cc
//  Katara
//
//  Created by Arne Philipeit on 1/30/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "interpret.h"

#include "src/cmd/katara/build.h"
#include "src/ir/interpreter/interpreter.h"
#include "src/ir/representation/program.h"

namespace cmd {
namespace katara {

ErrorCode Interpret(std::vector<std::filesystem::path>& paths, BuildOptions& build_options,
                    InterpretOptions& interpret_options, DebugHandler& debug_handler,
                    Context* ctx) {
  std::variant<std::unique_ptr<ir::Program>, ErrorCode> ir_program_or_error =
      Build(paths, build_options, debug_handler, ctx);
  if (std::holds_alternative<ErrorCode>(ir_program_or_error)) {
    return std::get<ErrorCode>(ir_program_or_error);
  }
  std::unique_ptr<ir::Program> ir_program =
      std::get<std::unique_ptr<ir::Program>>(std::move(ir_program_or_error));

  ir_interpreter::Interpreter interpreter(ir_program.get(), interpret_options.sanitize);
  interpreter.Run();
  return ErrorCode(interpreter.exit_code());
}

}  // namespace katara
}  // namespace cmd
