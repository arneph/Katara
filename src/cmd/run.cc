//
//  run.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "run.h"

#include <iomanip>
#include <sstream>
#include <variant>

#include "src/cmd/build.h"
#include "src/common/memory/memory.h"
#include "src/x86_64/machine_code/linker.h"

namespace cmd {
namespace {

void* MallocJump(int size) {
  void* p = malloc(size);
  return p;
}

void FreeJump(void* ptr) { free(ptr); }

}  // namespace

ErrorCode Run(Context* ctx) {
  std::variant<BuildResult, ErrorCode> build_result_or_error = Build(ctx);
  if (std::holds_alternative<ErrorCode>(build_result_or_error)) {
    return std::get<ErrorCode>(build_result_or_error);
  }
  BuildResult& build_result = std::get<BuildResult>(build_result_or_error);
  std::unique_ptr<x86_64::Program>& x86_64_program = build_result.x86_64_program;

  x86_64::Linker linker;
  linker.AddFuncAddr(x86_64_program->declared_funcs().at("malloc"), (uint8_t*)&MallocJump);
  linker.AddFuncAddr(x86_64_program->declared_funcs().at("free"), (uint8_t*)&FreeJump);

  common::Memory memory(common::Memory::kPageSize, common::Memory::kWrite);
  int64_t program_size = x86_64_program->Encode(linker, memory.data());
  linker.ApplyPatches();

  memory.ChangePermissions(common::Memory::kRead);
  if (ctx->generate_debug_info()) {
    std::ostringstream buffer;
    for (int64_t j = 0; j < program_size; j++) {
      buffer << std::hex << std::setfill('0') << std::setw(2) << (unsigned short)memory.data()[j]
             << " ";
      if (j % 8 == 7 && j != program_size - 1) {
        buffer << "\n";
      }
    }
    ctx->WriteToDebugFile(buffer.str(), /* subdir_name= */ "", "x86_64.hex.txt");
  }

  memory.ChangePermissions(common::Memory::kExecute);
  x86_64::Func* x86_64_main_func = x86_64_program->DefinedFuncWithName("main");
  int (*main_func)(void) = (int (*)(void))(linker.func_addrs().at(x86_64_main_func->func_num()));
  return ErrorCode(main_func());
}

}  // namespace cmd
