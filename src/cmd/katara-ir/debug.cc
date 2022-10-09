//
//  debug.cc
//  katara-ir
//
//  Created by Arne Philipeit on 8/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "debug.h"

#include <iomanip>
#include <iostream>
#include <string>

#include "src/cmd/katara-ir/check.h"
#include "src/cmd/repl.h"
#include "src/ir/interpreter/debugger.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/print.h"

namespace cmd {
namespace katara_ir {
namespace {

void HandleRunCommand(std::vector<std::string> args, ir_interpreter::Debugger& db, Context* ctx) {
  if (args.size() != 1) {
    *ctx->stderr() << "Unknown command.\n";
    return;
  }
  switch (db.execution_state()) {
    case ir_interpreter::Debugger::ExecutionState::kRunning:
      *ctx->stderr() << "Program is already running.\n";
      return;
    case ir_interpreter::Debugger::ExecutionState::kPausing:
      db.AwaitPause();
      if (db.execution_state() == ir_interpreter::Debugger::ExecutionState::kTerminated) {
        *ctx->stderr() << "Program has terminated.\n";
        return;
      }
      // fallthrough
    case ir_interpreter::Debugger::ExecutionState::kPaused:
      db.Run();
      return;
    case ir_interpreter::Debugger::ExecutionState::kTerminated:
      *ctx->stderr() << "Program has terminated.\n";
      return;
  }
}

void HandlePauseCommand(std::vector<std::string> args, ir_interpreter::Debugger& db, Context* ctx) {
  if (args.size() != 1) {
    *ctx->stderr() << "Unknown command.\n";
    return;
  }
  db.PauseAndAwait();
  if (db.execution_state() == ir_interpreter::Debugger::ExecutionState::kTerminated) {
    *ctx->stderr() << "Program has terminated.\n";
    return;
  }
}

void HandleStepCommand(std::vector<std::string> args, ir_interpreter::Debugger& db, Context* ctx) {
  if (db.execution_state() != ir_interpreter::Debugger::ExecutionState::kPaused) {
    *ctx->stderr() << "Cannot step when the program is not paused.\n";
    return;
  }
  if (args.size() == 1) {
    db.StepIn();
    return;
  }
  if (args.size() == 2) {
    if (args.at(1) == "in") {
      db.StepIn();
    } else if (args.at(1) == "over") {
      db.StepOver();
    } else if (args.at(1) == "out") {
      db.StepOut();
    } else {
      *ctx->stderr() << "Unknown command.\n";
    }
    return;
  }
  *ctx->stderr() << "Unknown command.\n";
}

void HandlePrintCommand(std::vector<std::string> args, ir_interpreter::Debugger& db, Context* ctx) {
  if (args.size() != 2) {
    *ctx->stderr() << "Unknown command.\n";
    return;
  }
  switch (db.execution_state()) {
    case ir_interpreter::Debugger::ExecutionState::kPaused:
    case ir_interpreter::Debugger::ExecutionState::kTerminated:
      break;
    default:
      *ctx->stderr() << "Cannot print when the program is not paused or terminated.\n";
      return;
  }
  if (args.at(1) == "location") {
    if (db.execution_state() == ir_interpreter::Debugger::ExecutionState::kTerminated) {
      *ctx->stderr() << "Program has terminated.\n";
      return;
    }
    std::size_t frame_index = db.stack().depth() - 1;
    *ctx->stdout() << db.stack().ToDebuggerString(frame_index, /*include_computed_values=*/false);

  } else if (args.at(1) == "stackframe") {
    if (db.execution_state() == ir_interpreter::Debugger::ExecutionState::kTerminated) {
      *ctx->stderr() << "Program has terminated.\n";
      return;
    }
    std::size_t frame_index = db.stack().depth() - 1;
    *ctx->stdout() << db.stack().ToDebuggerString(frame_index, /*include_computed_values=*/true);

  } else if (args.at(1) == "stack") {
    *ctx->stdout() << db.stack().ToDebuggerString();

  } else if (args.at(1) == "heap") {
    if (!db.heap().sanitizes()) {
      *ctx->stderr() << "Cannot print heap when sanitizing is not turned on.\n";
      return;
    }
    *ctx->stdout() << db.heap().ToDebuggerString();

  } else if (args.at(1) == "program") {
    ir_serialization::Print(db.program(), *ctx->stdout());

  } else if (args.at(1).starts_with("@")) {
    std::string func_num_str = args.at(1).substr(1);
    ir::func_num_t func_num = std::stoul(func_num_str);
    if (func_num == ir::kNoFuncNum || !db.program()->HasFunc(func_num)) {
      *ctx->stderr() << "Function does not exist.\n";
      return;
    }
    ir_serialization::Print(db.program()->GetFunc(func_num), *ctx->stdout());
    *ctx->stdout() << "\n";

  } else if (args.at(1).starts_with("<") && args.at(1).ends_with(">")) {
    std::string frame_index_str = args.at(1).substr(1, args.at(1).size() - 2);
    std::size_t frame_index = std::stoul(frame_index_str) - 1;
    if (frame_index < 0 || frame_index >= db.stack().depth()) {
      *ctx->stderr() << "Stackframe does not exist.\n";
      return;
    }
    *ctx->stdout() << db.stack().ToDebuggerString(frame_index, /*include_computed_values=*/true);

  } else if (args.at(1).starts_with("%")) {
    if (db.execution_state() == ir_interpreter::Debugger::ExecutionState::kTerminated) {
      *ctx->stderr() << "Program has terminated.\n";
      return;
    }
    std::string value_num_str = args.at(1).substr(1);
    ir::value_num_t value_num = std::stoll(value_num_str);
    auto it = db.stack().current_frame()->computed_values().find(value_num);
    if (it == db.stack().current_frame()->computed_values().end()) {
      *ctx->stderr() << "%" << value_num << " has no value.\n";
    } else {
      *ctx->stdout() << "%" << value_num << " = " << it->second->RefStringWithType() << "\n";
    }

  } else if (args.at(1).starts_with("0x")) {
    if (!db.heap().sanitizes()) {
      *ctx->stderr() << "Cannot print heap when sanitizing is not turned on.\n";
      return;
    }
    int64_t address = std::stoll(args.at(1), /*pos=*/nullptr, /*base=*/16);
    *ctx->stdout() << db.heap().ToDebuggerString(address);

  } else {
    *ctx->stderr() << "Unknown command.\n";
  }
}

std::vector<std::string> CommandToArgs(std::string command) {
  std::vector<std::string> args;
  std::size_t start_of_arg = 0;
  for (std::size_t i = 0; i < command.length(); i++) {
    if (command.at(i) != ' ') continue;
    if (start_of_arg == i) {
      start_of_arg++;
      continue;
    }
    args.push_back(command.substr(start_of_arg, i - start_of_arg));
    start_of_arg = i + 1;
  }
  if (start_of_arg != command.length()) {
    args.push_back(command.substr(start_of_arg));
  }
  return args;
}

std::string ExpandShortcuts(std::string command) {
  if (command == "si") {
    return "step in";
  } else if (command == "so") {
    return "step over";
  } else if (command == "su") {
    return "step out";
  } else if (command == "pl") {
    return "print location";
  } else if (command == "pf") {
    return "print stackframe";
  } else if (command == "ps") {
    return "print stack";
  } else if (command == "ph") {
    return "print heap";
  } else if (command == "pp") {
    return "print program";
  } else {
    return command;
  }
}

void HandleDebuggerCommand(std::string command, ir_interpreter::Debugger& db, Context* ctx) {
  command = ExpandShortcuts(command);
  std::vector<std::string> args = CommandToArgs(command);
  if (args.empty()) return;
  if (args.front() == "run" || args.front() == "r") {
    HandleRunCommand(args, db, ctx);
  } else if (args.front() == "pause" || args.front() == "h") {
    HandlePauseCommand(args, db, ctx);
  } else if (args.front() == "step" || args.front() == "s") {
    HandleStepCommand(args, db, ctx);
  } else if (args.front() == "print" || args.front() == "p") {
    HandlePrintCommand(args, db, ctx);
  } else {
    *ctx->stderr() << "Unknown command.\n";
  }
}

void HandleDebuggerCommands(ir_interpreter::Debugger& db, Context* ctx) {
  auto command_executor = [&db, ctx](std::string command) {
    HandleDebuggerCommand(command, db, ctx);
  };
  REPL repl(command_executor, ctx, REPL::kDefaultConfig);
  db.SetTerminationObserver([ctx, &db, &repl] {
    repl.InterruptOutput([ctx, &db] {
      std::string message =
          "Program terminated with exit code " + std::to_string(db.exit_code()) + "\n";
      *ctx->stdout() << message;
    });
  });
  repl.Run();
}

}  // namespace

ErrorCode Debug(std::filesystem::path path, DebugOptions& debug_options, Context* ctx) {
  std::variant<std::unique_ptr<ir::Program>, ErrorCode> ir_program_or_error = Check(path, ctx);
  if (std::holds_alternative<ErrorCode>(ir_program_or_error)) {
    return std::get<ErrorCode>(ir_program_or_error);
  }
  std::unique_ptr<ir::Program> ir_program =
      std::get<std::unique_ptr<ir::Program>>(std::move(ir_program_or_error));

  ir_interpreter::Debugger debugger(ir_program.get(), debug_options.sanitize);
  HandleDebuggerCommands(debugger, ctx);
  return ErrorCode::kNoError;
}

}  // namespace katara_ir
}  // namespace cmd
