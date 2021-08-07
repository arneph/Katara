//
//  execution_test.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 7/10/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include <stdio.h>
#include <sys/mman.h>

#include <iomanip>
#include <iostream>
#include <memory>

#include "src/common/data.h"
#include "src/x86_64/block.h"
#include "src/x86_64/func.h"
#include "src/x86_64/instrs/al_instrs.h"
#include "src/x86_64/instrs/cf_instrs.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/machine_code/linker.h"
#include "src/x86_64/ops.h"
#include "src/x86_64/program.h"

long AddInts(long a, long b) { return a + b; }
void PrintInt(long value) { std::cout << std::dec << value << "\n" << std::flush; }

int main() {
  std::cout << "running x86_64 execution test\n";

  constexpr int kSymAddIntsAddr = 1234;
  constexpr int kSymPrintIntAddr = 1235;
  x86_64::Linker linker;
  linker.AddFuncAddr(kSymAddIntsAddr, (uint8_t*)&AddInts);
  linker.AddFuncAddr(kSymPrintIntAddr, (uint8_t*)&PrintInt);

  const char* str = "Hello world!\n";
  x86_64::Imm str_c((int64_t(str)));

  x86_64::ProgramBuilder prog_builder;
  x86_64::FuncBuilder main_func_builder = prog_builder.DefineFunc("main");

  // Prolog:
  {
    x86_64::BlockBuilder prolog_block_builder = main_func_builder.AddBlock();
    prolog_block_builder.AddInstr<x86_64::Push>(x86_64::rbp);
    prolog_block_builder.AddInstr<x86_64::Mov>(x86_64::rbp, x86_64::rsp);
  }

  // Fibonacci numbers:
  {
    x86_64::BlockBuilder start_block_builder = main_func_builder.AddBlock();
    start_block_builder.AddInstr<x86_64::Mov>(x86_64::r15b, x86_64::Imm(int8_t{10}));
    start_block_builder.AddInstr<x86_64::Mov>(x86_64::r12, x86_64::Imm(int64_t{1}));
    start_block_builder.AddInstr<x86_64::Mov>(x86_64::r13, x86_64::Imm(int64_t{1}));
    start_block_builder.AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::r12);
    start_block_builder.AddInstr<x86_64::Call>(x86_64::FuncRef(kSymPrintIntAddr));
  }
  {
    x86_64::BlockBuilder loop_block_builder = main_func_builder.AddBlock();
    loop_block_builder.AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::r12);
    loop_block_builder.AddInstr<x86_64::Call>(x86_64::FuncRef(kSymPrintIntAddr));
    loop_block_builder.AddInstr<x86_64::Mov>(x86_64::r14, x86_64::r12);
    loop_block_builder.AddInstr<x86_64::Add>(x86_64::r14, x86_64::r13);
    loop_block_builder.AddInstr<x86_64::Mov>(x86_64::r13, x86_64::r12);
    loop_block_builder.AddInstr<x86_64::Mov>(x86_64::r12, x86_64::r14);
    loop_block_builder.AddInstr<x86_64::Sub>(x86_64::r15b, x86_64::Imm(int8_t{1}));
    loop_block_builder.AddInstr<x86_64::Jcc>(x86_64::InstrCond::kAbove,
                                             loop_block_builder.block()->GetBlockRef());
  }

  // Hello world (Syscall test):
  {
    x86_64::BlockBuilder hello_block_builder = main_func_builder.AddBlock();
    hello_block_builder.AddInstr<x86_64::Mov>(x86_64::rax,
                                              x86_64::Imm(int64_t{0x2000004}));        // write
    hello_block_builder.AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::Imm(int32_t{1}));   // stdout
    hello_block_builder.AddInstr<x86_64::Mov>(x86_64::rsi, str_c);                     // const char
    hello_block_builder.AddInstr<x86_64::Mov>(x86_64::rdx, x86_64::Imm(int32_t{13}));  // size
    hello_block_builder.AddInstr<x86_64::Syscall>();
    // hello_block_builder.AddInstr(std::make_unique<x86_64::Jmp>(hello_block_builder.block()->GetBlockRef()));
  }

  // Addition & C calling convention test:
  {
    x86_64::BlockBuilder test_block_builder = main_func_builder.AddBlock();
    test_block_builder.AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::Imm(int32_t{1}));
    test_block_builder.AddInstr<x86_64::Mov>(x86_64::rsi, x86_64::Imm(int32_t{2}));
    test_block_builder.AddInstr<x86_64::Call>(x86_64::FuncRef(kSymAddIntsAddr));
    test_block_builder.AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::rax);
    test_block_builder.AddInstr<x86_64::Sub>(x86_64::rdi, x86_64::rax);
    test_block_builder.AddInstr<x86_64::Add>(x86_64::rdi, x86_64::rax);
    test_block_builder.AddInstr<x86_64::Add>(x86_64::rdi, x86_64::Imm(int8_t{17}));
    test_block_builder.AddInstr<x86_64::Sub>(x86_64::rdi, x86_64::Imm(int8_t{6}));
    test_block_builder.AddInstr<x86_64::Call>(x86_64::FuncRef(kSymAddIntsAddr));
    test_block_builder.AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::Imm(int32_t{1233}));
    test_block_builder.AddInstr<x86_64::Sub>(x86_64::rdi, x86_64::Imm(int32_t{-1}));
    test_block_builder.AddInstr<x86_64::Call>(x86_64::FuncRef(kSymPrintIntAddr));
  }

  // Epilog:
  {
    x86_64::BlockBuilder epilog_block_builder = main_func_builder.AddBlock();
    epilog_block_builder.AddInstr<x86_64::Mov>(x86_64::rsp, x86_64::rbp);
    epilog_block_builder.AddInstr<x86_64::Pop>(x86_64::rbp);
    epilog_block_builder.AddInstr<x86_64::Ret>();
  }

  std::unique_ptr<x86_64::Program> program = prog_builder.Build();

  std::cout << "BEGIN assembly\n";
  std::cout << program->ToString();
  std::cout << "END assembly\n";

  int64_t page_size = 1 << 12;
  uint8_t* base =
      (uint8_t*)mmap(NULL, page_size, PROT_EXEC | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  common::data code(base, page_size);

  int64_t program_size = program->Encode(linker, code);
  linker.ApplyPatches();

  std::cout << "BEGIN machine code\n";
  for (int64_t j = 0; j < program_size; j++) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short)code[j] << " ";
    if (j % 8 == 7 && j != program_size - 1) {
      std::cout << "\n";
    }
  }
  std::cout << "END machine code\n";

  std::cout << "BEGIN program output\n" << std::flush;
  void (*func)(void) = (void (*)(void))base;
  func();
  std::cout << "END program output\n" << std::flush;

  std::cout << "completed x86_64 execution test\n";

  return 0;
}
