//
//  interpret.cc
//  Katara
//
//  Created by Arne Philipeit on 08/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "interpret.h"

#include "src/cmd/katara-ir/check.h"
#include "src/ir/interpreter/interpreter.h"
#include "src/ir/representation/program.h"

namespace cmd {
namespace katara_ir {

ErrorCode Interpret(std::filesystem::path path, InterpretOptions& interpret_options, Context* ctx) {
  std::variant<std::unique_ptr<ir::Program>, ErrorCode> ir_program_or_error = Check(path, ctx);
  if (std::holds_alternative<ErrorCode>(ir_program_or_error)) {
    return std::get<ErrorCode>(ir_program_or_error);
  }
  std::unique_ptr<ir::Program> ir_program =
      std::get<std::unique_ptr<ir::Program>>(std::move(ir_program_or_error));

  ir_interpreter::Interpreter interpreter(ir_program.get(), interpret_options.sanitize);
  interpreter.Run();
  return ErrorCode(interpreter.exit_code());
}

}  // namespace katara_ir
}  // namespace cmd
