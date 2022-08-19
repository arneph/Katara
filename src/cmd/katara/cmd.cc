//
//  cmd.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "cmd.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "src/cmd/katara/build.h"
#include "src/cmd/katara/doc.h"
#include "src/cmd/katara/interpret.h"
#include "src/cmd/katara/run.h"
#include "src/cmd/version.h"
#include "src/common/flags/flags.h"
#include "src/common/logging/logging.h"

namespace cmd {
namespace katara {
namespace {

enum class Command {
  kBuild,
  kDoc,
  kInterpret,
  kHelp,
  kRun,
  kVersion,
};

std::optional<Command> ParseCommand(std::string command) {
  if (command == "build") {
    return Command::kBuild;
  } else if (command == "doc") {
    return Command::kDoc;
  } else if (command == "interpret") {
    return Command::kInterpret;
  } else if (command == "help") {
    return Command::kHelp;
  } else if (command == "run") {
    return Command::kRun;
  } else if (command == "version") {
    return Command::kVersion;
  } else {
    return std::nullopt;
  }
}

struct FlagSets {
  common::FlagSet debug_flags;
  common::FlagSet build_flags;
  common::FlagSet doc_flags;
  common::FlagSet interpret_flags;
  common::FlagSet run_flags;
};

void GenerateFlagSets(DebugConfig& debug_config, BuildOptions& build_options,
                      InterpretOptions& interpret_options, FlagSets& flag_sets) {
  flag_sets.debug_flags.Add<bool>("debug_output",
                                  "If true, debug information will be written in the directory "
                                  "specified with -debug_output_path.",
                                  debug_config.generate_debug_info);
  flag_sets.debug_flags.Add<std::filesystem::path>(
      "debug_output_path", "The directory where debug information will be written to (if enabled).",
      debug_config.debug_path);
  flag_sets.debug_flags.Add<bool>(
      "debug_check_ir", "If true, runs the ir_checker over the IR between each transformation.",
      debug_config.check_ir);

  flag_sets.build_flags = flag_sets.debug_flags.CreateChild();
  flag_sets.build_flags.Add<bool>("optimize_ir_ext",
                                  "If true, optimizes the program based on the intermediate "
                                  "representation of the language extension.",
                                  build_options.optimize_ir_ext);
  flag_sets.build_flags.Add<bool>(
      "optimize_ir", "If true, optimizes the program based on the intermediate representation.",
      build_options.optimize_ir);

  flag_sets.doc_flags = flag_sets.debug_flags.CreateChild();
  flag_sets.interpret_flags = flag_sets.build_flags.CreateChild();
  flag_sets.interpret_flags.Add<bool>("sanitize",
                                      "If true, performs dynamic checks during interpretation.",
                                      interpret_options.sanitize);

  flag_sets.run_flags = flag_sets.build_flags.CreateChild();
}

std::vector<std::filesystem::path> ArgsToPaths(std::vector<std::string>& args) {
  std::vector<std::filesystem::path> main_file_paths;
  main_file_paths.reserve(args.size());
  for (std::string& arg : args) {
    main_file_paths.push_back(arg);
  }
  return main_file_paths;
}

void PrintGeneralHelp(Context* ctx) {
  *ctx->stdout()
      << "katara is a tool to work with Katara source code.\n"
         "\n"
         "Usage:\n"
         "\n"
         "\tkatara <command> [arguments]\n"
         "\n"
         "The commands are:\n"
         "\n"
         "\tbuild     build Katara packages\n"
         "\tdoc       generate documentation for Katara packages\n"
         "\tinterpret build and interpret Katara programs\n"
         "\thelp      print this documentation or detailed documentation for another command\n"
         "\trun       build and run Katara programs\n"
         "\tversion   print Katara version\n"
         "\n";
}

void PrintHelpForCommand(std::string command, bool has_args, common::FlagSet* flags, Context* ctx) {
  *ctx->stdout() << "Usage: katara " << command;
  if (has_args) {
    *ctx->stdout() << " [arguments]";
  }
  *ctx->stdout() << "\n";
  if (flags != nullptr) {
    *ctx->stdout() << "\n";
    flags->PrintDefaults(ctx->stdout());
  }
}

void PrintHelpForArgs(std::vector<std::string> args, FlagSets& flag_sets, Context* ctx) {
  if (args.size() != 1) {
    PrintGeneralHelp(ctx);
    return;
  }
  std::optional<Command> command = ParseCommand(args.front());
  if (!command.has_value()) {
    PrintGeneralHelp(ctx);
    return;
  }
  switch (*command) {
    case Command::kBuild:
      PrintHelpForCommand("build", /*has_args=*/true, &flag_sets.build_flags, ctx);
      break;
    case Command::kDoc:
      PrintHelpForCommand("doc", /*has_args=*/true, &flag_sets.doc_flags, ctx);
      break;
    case Command::kInterpret:
      PrintHelpForCommand("interpret", /*has_args=*/true, &flag_sets.interpret_flags, ctx);
      break;
    case Command::kRun:
      PrintHelpForCommand("run", /*has_args=*/true, &flag_sets.run_flags, ctx);
      break;
    case Command::kVersion:
      PrintHelpForCommand("version", /*has_args=*/false, /*flags=*/nullptr, ctx);
      break;
    case Command::kHelp:
    default:
      PrintGeneralHelp(ctx);
      break;
  }
}

}  // namespace

ErrorCode Execute(std::vector<std::string> args, Context* ctx) {
  if (args.empty()) {
    PrintGeneralHelp(ctx);
    return ErrorCode::kNoError;
  }
  std::optional<Command> command = ParseCommand(args.front());
  args.erase(args.begin());
  if (!command.has_value()) {
    PrintGeneralHelp(ctx);
    return kNoError;
  }

  DebugConfig debug_config;
  BuildOptions build_options;
  InterpretOptions interpret_options;
  FlagSets flag_sets;
  GenerateFlagSets(debug_config, build_options, interpret_options, flag_sets);

  switch (*command) {
    case Command::kHelp:
      PrintHelpForArgs(args, flag_sets, ctx);
      return kNoError;
    case Command::kVersion:
      Version(ctx);
      return kNoError;
    case Command::kBuild: {
      flag_sets.build_flags.Parse(args, ctx->stderr());
      std::vector<std::filesystem::path> paths = ArgsToPaths(args);
      DebugHandler debug_handler(debug_config, ctx);
      std::variant<std::unique_ptr<ir::Program>, ErrorCode> program_or_error =
          Build(paths, build_options, debug_handler, ctx);
      if (std::holds_alternative<ErrorCode>(program_or_error)) {
        return std::get<ErrorCode>(program_or_error);
      } else {
        return kNoError;
      }
    }
    case Command::kDoc: {
      flag_sets.doc_flags.Parse(args, ctx->stderr());
      std::vector<std::filesystem::path> paths = ArgsToPaths(args);
      DebugHandler debug_handler(debug_config, ctx);
      return Doc(paths, debug_handler, ctx);
    }
    case Command::kInterpret: {
      flag_sets.interpret_flags.Parse(args, ctx->stderr());
      std::vector<std::filesystem::path> paths = ArgsToPaths(args);
      DebugHandler debug_handler(debug_config, ctx);
      return Interpret(paths, build_options, interpret_options, debug_handler, ctx);
    }
    case Command::kRun: {
      flag_sets.run_flags.Parse(args, ctx->stderr());
      std::vector<std::filesystem::path> paths = ArgsToPaths(args);
      DebugHandler debug_handler(debug_config, ctx);
      return Run(paths, build_options, debug_handler, ctx);
    }
    default:
      common::fail("unexpected command");
  }
}

}  // namespace katara
}  // namespace cmd
