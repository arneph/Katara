//
//  run.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "run.h"

#include <sys/mman.h>

#include <iomanip>

#include "src/cmd/build.h"
#include "src/x86_64/machine_code/linker.h"

namespace cmd {

int Run(const std::vector<std::string> args, std::istream& in, std::ostream& out,
        std::ostream& err) {
  auto [ir_program, x86_64_program, exit_code] = Build(args, err);
  if (exit_code) {
    return exit_code;
  }

  // ir_interpreter::Interpreter interpreter(ir_program.get());
  // interpreter.run();

  // return static_cast<int>(interpreter.exit_code());

  x86_64::Linker linker;
  linker.AddFuncAddr(x86_64_program->declared_funcs().at("malloc"), (uint8_t*)&malloc);
  linker.AddFuncAddr(x86_64_program->declared_funcs().at("free"), (uint8_t*)&free);

  int64_t page_size = 1 << 12;
  uint8_t* base =
      (uint8_t*)mmap(NULL, page_size, PROT_EXEC | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  common::DataView code(base, page_size);

  int64_t program_size = x86_64_program->Encode(linker, code);
  linker.ApplyPatches();

  out << "BEGIN machine code\n";
  for (int64_t j = 0; j < program_size; j++) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short)code[j] << " ";
    if (j % 8 == 7 && j != program_size - 1) {
      out << "\n";
    }
  }
  out << "END machine code\n";

  x86_64::Func* x86_64_main_func = x86_64_program->DefinedFuncWithName("main");
  int (*main_func)(void) = (int (*)(void))(linker.func_addrs().at(x86_64_main_func->func_num()));
  return main_func();
}

}  // namespace cmd
