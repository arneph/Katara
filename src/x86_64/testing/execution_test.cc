//
//  execution_test.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 7/10/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <iomanip>
#include <iostream>
#include <memory>

#include "src/common/data_view/data_view.h"
#include "src/common/memory/memory.h"
#include "src/x86_64/block.h"
#include "src/x86_64/func.h"
#include "src/x86_64/instrs/arithmetic_logic_instrs.h"
#include "src/x86_64/instrs/control_flow_instrs.h"
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

  const int64_t buffer_size = 100;
  char buffer[buffer_size] = {0};
  x86_64::Imm buffer_c((int64_t(buffer)));

  x86_64::Program program;
  x86_64::Func* main_func = program.DefineFunc("main");

  // Prolog:
  {
    x86_64::Block* prolog_block = main_func->AddBlock();
    prolog_block->AddInstr<x86_64::Push>(x86_64::rbp);
    prolog_block->AddInstr<x86_64::Mov>(x86_64::rbp, x86_64::rsp);
    prolog_block->AddInstr<x86_64::Push>(x86_64::r12);
    prolog_block->AddInstr<x86_64::Push>(x86_64::r13);
    prolog_block->AddInstr<x86_64::Push>(x86_64::r14);
    prolog_block->AddInstr<x86_64::Push>(x86_64::r15);
  }

  // Fibonacci numbers:
  {
    x86_64::Block* start_block = main_func->AddBlock();
    start_block->AddInstr<x86_64::Mov>(x86_64::r15b, x86_64::Imm(int8_t{10}));
    start_block->AddInstr<x86_64::Mov>(x86_64::r12, x86_64::Imm(int64_t{1}));
    start_block->AddInstr<x86_64::Mov>(x86_64::r13, x86_64::Imm(int64_t{1}));
    start_block->AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::r12);
    start_block->AddInstr<x86_64::Call>(x86_64::FuncRef(kSymPrintIntAddr));
  }
  {
    x86_64::Block* loop_block = main_func->AddBlock();
    loop_block->AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::r12);
    loop_block->AddInstr<x86_64::Call>(x86_64::FuncRef(kSymPrintIntAddr));
    loop_block->AddInstr<x86_64::Mov>(x86_64::r14, x86_64::r12);
    loop_block->AddInstr<x86_64::Add>(x86_64::r14, x86_64::r13);
    loop_block->AddInstr<x86_64::Mov>(x86_64::r13, x86_64::r12);
    loop_block->AddInstr<x86_64::Mov>(x86_64::r12, x86_64::r14);
    loop_block->AddInstr<x86_64::Sub>(x86_64::r15b, x86_64::Imm(int8_t{1}));
    loop_block->AddInstr<x86_64::Jcc>(x86_64::InstrCond::kAbove, loop_block->GetBlockRef());
  }

  // Write syscall test:
  {
    x86_64::Block* hello_block = main_func->AddBlock();
    hello_block->AddInstr<x86_64::Mov>(x86_64::rax, x86_64::Imm(int64_t{0x2000004}));  // write
    hello_block->AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::Imm(int32_t{1}));          // stdout
    hello_block->AddInstr<x86_64::Mov>(x86_64::rsi, str_c);                            // const char
    hello_block->AddInstr<x86_64::Mov>(x86_64::rdx, x86_64::Imm(int32_t{13}));         // size
    hello_block->AddInstr<x86_64::Syscall>();
  }

  // Read syscall test:
  {
    x86_64::Block* hello_block = main_func->AddBlock();
    hello_block->AddInstr<x86_64::Mov>(x86_64::rax, x86_64::Imm(int64_t{0x2000003}));  // read
    hello_block->AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::Imm(int32_t{0}));          // stdin
    hello_block->AddInstr<x86_64::Mov>(x86_64::rsi, buffer_c);                         // const char
    hello_block->AddInstr<x86_64::Mov>(x86_64::rdx, x86_64::Imm(int32_t{buffer_size - 1}));  // size
    hello_block->AddInstr<x86_64::Syscall>();
  }

  // Addition & C calling convention test:
  {
    x86_64::Block* test_block = main_func->AddBlock();
    test_block->AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::Imm(int32_t{1}));
    test_block->AddInstr<x86_64::Mov>(x86_64::rsi, x86_64::Imm(int32_t{2}));
    test_block->AddInstr<x86_64::Call>(x86_64::FuncRef(kSymAddIntsAddr));
    test_block->AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::rax);
    test_block->AddInstr<x86_64::Sub>(x86_64::rdi, x86_64::rax);
    test_block->AddInstr<x86_64::Add>(x86_64::rdi, x86_64::rax);
    test_block->AddInstr<x86_64::Add>(x86_64::rdi, x86_64::Imm(int8_t{17}));
    test_block->AddInstr<x86_64::Sub>(x86_64::rdi, x86_64::Imm(int8_t{6}));
    test_block->AddInstr<x86_64::Call>(x86_64::FuncRef(kSymAddIntsAddr));
    test_block->AddInstr<x86_64::Mov>(x86_64::rdi, x86_64::Imm(int32_t{1233}));
    test_block->AddInstr<x86_64::Sub>(x86_64::rdi, x86_64::Imm(int32_t{-1}));
    test_block->AddInstr<x86_64::Call>(x86_64::FuncRef(kSymPrintIntAddr));
  }

  // Epilog:
  {
    x86_64::Block* epilog_block = main_func->AddBlock();
    epilog_block->AddInstr<x86_64::Pop>(x86_64::r15);
    epilog_block->AddInstr<x86_64::Pop>(x86_64::r14);
    epilog_block->AddInstr<x86_64::Pop>(x86_64::r13);
    epilog_block->AddInstr<x86_64::Pop>(x86_64::r12);
    epilog_block->AddInstr<x86_64::Mov>(x86_64::rsp, x86_64::rbp);
    epilog_block->AddInstr<x86_64::Pop>(x86_64::rbp);
    epilog_block->AddInstr<x86_64::Ret>();
  }

  std::cout << "BEGIN assembly\n";
  std::cout << program.ToString();
  std::cout << "END assembly\n";

  std::cout << "BEGIN memory allocation\n";
  common::Memory memory(common::Memory::kPageSize,
                        common::Memory::Permissions(common::Memory::Permissions::kRead |
                                                    common::Memory::Permissions::kWrite));
  common::DataView code = memory.data();
  std::cout << "END memory setup\n";

  std::cout << "BEGIN writing program\n";
  int64_t program_size = program.Encode(linker, code);
  linker.ApplyPatches();
  std::cout << "END writing program\n";

  std::cout << "BEGIN machine code\n";
  for (int64_t j = 0; j < program_size; j++) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short)code[j] << " ";
    if (j % 8 == 7 && j != program_size - 1) {
      std::cout << "\n";
    }
  }
  std::cout << "END machine code\n";

  std::cout << "BEGIN memory permission change\n";
  memory.ChangePermissions(common::Memory::Permissions::kExecute);
  std::cout << "END memory permission change\n";

  std::cout << "BEGIN program output\n" << std::flush;
  void (*func)(void) = (void (*)(void))code.base();
  func();
  std::cout << "END program output\n" << std::flush;

  std::cout << "BEGIN memory deallocation\n";
  memory.Free();
  std::cout << "END memory deallocation\n";

  std::cout << "BEGIN read buffer\n" << std::flush;
  std::cout << buffer;
  std::cout << "END read buffer\n" << std::flush;

  std::cout << "completed x86_64 execution test\n";

  return 0;
}
