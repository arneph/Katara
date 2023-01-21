//
//  cmd.cc
//  Katara
//
//  Created by Arne Philipeit on 08/18/22.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "cmd.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "src/cmd/katara-ir/check.h"
#include "src/cmd/katara-ir/debug.h"
#include "src/cmd/katara-ir/format.h"
#include "src/cmd/katara-ir/interpret.h"
#include "src/cmd/version.h"
#include "src/common/flags/flags.h"
#include "src/common/logging/logging.h"

namespace cmd {
namespace katara_ir {

using ::common::flags::FlagSet;

namespace {

enum class Command {
  kCheck,
  kDebug,
  kFormat,
  kInterpret,
  kHelp,
  kVersion,
};

std::optional<Command> ParseCommand(std::string command) {
  if (command == "check") {
    return Command::kCheck;
  } else if (command == "debug") {
    return Command::kDebug;
  } else if (command == "format") {
    return Command::kFormat;
  } else if (command == "interpret") {
    return Command::kInterpret;
  } else if (command == "help") {
    return Command::kHelp;
  } else if (command == "version") {
    return Command::kVersion;
  } else {
    return std::nullopt;
  }
}

struct FlagSets {
  FlagSet check_flags;
  FlagSet debug_flags;
  FlagSet format_flags;
  FlagSet interpret_flags;
};

void GenerateFlagSets(InterpretOptions& interpret_options, DebugOptions& debug_options,
                      FlagSets& flag_sets) {
  flag_sets.format_flags = flag_sets.check_flags.CreateChild();
  flag_sets.interpret_flags = flag_sets.check_flags.CreateChild();
  flag_sets.interpret_flags.Add<bool>("sanitize",
                                      "If true, performs dynamic checks during interpretation.",
                                      interpret_options.sanitize);
  flag_sets.debug_flags = flag_sets.check_flags.CreateChild();
  flag_sets.debug_flags.Add<bool>("sanitize",
                                  "If true, performs dynamic checks during interpretation.",
                                  debug_options.sanitize);
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
      << "katara-ir is a tool to work with Katara intermediate representation.\n"
         "\n"
         "Usage:\n"
         "\n"
         "\tkatara-ir <command> [arguments]\n"
         "\n"
         "The commands are:\n"
         "\n"
         "\tcheck     check Katara IR files for syntactic and semantic correctness\n"
         "\tdebug     interpret a Katara IR file with a debugger\n"
         "\tformat    format Katara IR files\n"
         "\tinterpret interpret a Katara IR file\n"
         "\thelp      print this documentation or detailed documentation for another command\n"
         "\tversion   print Katara version\n"
         "\n";
}

void PrintHelpForCommand(std::string command, bool has_args, FlagSet* flags, Context* ctx) {
  *ctx->stdout() << "Usage: katara-ir " << command;
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
    case Command::kCheck:
      PrintHelpForCommand("check", /*has_args=*/true, &flag_sets.check_flags, ctx);
      break;
    case Command::kDebug:
      PrintHelpForCommand("debug", /*has_args=*/true, &flag_sets.debug_flags, ctx);
      break;
    case Command::kFormat:
      PrintHelpForCommand("format", /*has_args=*/true, &flag_sets.format_flags, ctx);
      break;
    case Command::kInterpret:
      PrintHelpForCommand("interpret", /*has_args=*/true, &flag_sets.interpret_flags, ctx);
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
    return ErrorCode::kNoError;
  }

  InterpretOptions interpret_options;
  DebugOptions debug_options;
  FlagSets flag_sets;
  GenerateFlagSets(interpret_options, debug_options, flag_sets);

  switch (*command) {
    case Command::kHelp:
      PrintHelpForArgs(args, flag_sets, ctx);
      return kNoError;
    case Command::kVersion:
      Version(ctx);
      return kNoError;
    case Command::kCheck: {
      flag_sets.check_flags.Parse(args, ctx->stderr());
      std::vector<std::filesystem::path> paths = ArgsToPaths(args);
      return Check(paths, ctx);
    }
    case Command::kDebug: {
      flag_sets.debug_flags.Parse(args, ctx->stderr());
      if (args.size() != 1) {
        *ctx->stderr() << "expected one argument\n";
        return ErrorCode::kMoreThanOneArgument;
      }
      std::vector<std::filesystem::path> paths = ArgsToPaths(args);
      return Debug(paths.front(), debug_options, ctx);
    }
    case Command::kFormat: {
      flag_sets.format_flags.Parse(args, ctx->stderr());
      std::vector<std::filesystem::path> paths = ArgsToPaths(args);
      return Format(paths, ctx);
    }
    case Command::kInterpret: {
      flag_sets.interpret_flags.Parse(args, ctx->stderr());
      if (args.size() != 1) {
        *ctx->stderr() << "expected one argument\n";
        return ErrorCode::kMoreThanOneArgument;
      }
      std::vector<std::filesystem::path> paths = ArgsToPaths(args);
      return Interpret(paths.front(), interpret_options, ctx);
    }
    default:
      common::fail("unexpected command");
  }
}

}  // namespace katara_ir
}  // namespace cmd
