//
//  main.cpp
//  Katara
//
//  Created by Arne Philipeit on 11/22/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>

#include "ir/prog.h"
#include "ir/func.h"
#include "ir/block.h"
#include "ir/instr.h"
#include "ir/value.h"

#include "ir_processors/scanner.h"
#include "ir_processors/parser.h"
#include "ir_processors/live_range_analyzer.h"
#include "ir_processors/register_allocator.h"

#include "x86_64/mc/linker.h"
#include "x86_64/prog.h"
#include "x86_64/func.h"
#include "x86_64/block.h"
#include "x86_64/instr.h"
#include "x86_64/instrs/al_instrs.h"
#include "x86_64/instrs/cf_instrs.h"
#include "x86_64/instrs/data_instrs.h"
#include "x86_64/ops.h"

long AddInts(long a, long b) {
    return a + b;
}

void PrintInt(long value) {
    printf("%ld\n", value);
}

void test_x86() {
    std::cout << "running x86-tests";
    
    uint8_t *add_ints_addr = (uint8_t *) &AddInts;
    uint8_t *print_int_addr = (uint8_t *) &PrintInt;
    
    x86_64::Linker *linker = new x86_64::Linker();
    linker->AddFuncAddr("addInts", add_ints_addr);
    linker->AddFuncAddr("printInt", print_int_addr);
    
    const char *str = "Hello world!\n";
    std::shared_ptr<x86_64::Imm64> str_c = std::make_shared<x86_64::Imm64>((int64_t) str);
    
    using namespace x86_64::literals;
    
    x86_64::ProgBuilder prog_builder;
    x86_64::FuncBuilder main_func_builder = prog_builder.AddFunc("main");
    
    // Prolog:
    {
        x86_64::BlockBuilder prolog_block_builder = main_func_builder.AddBlock();
        prolog_block_builder.AddInstr(std::make_unique<x86_64::Push>(x86_64::rbp));
        prolog_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rbp, x86_64::rsp));
    }
    
    // Fibonacci numbers:
    {
        x86_64::BlockBuilder start_block_builder = main_func_builder.AddBlock();
        start_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::r15b, 10_imm8));
        start_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::r12, 1_imm64));
        start_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::r13, 1_imm64));
        start_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rdi, x86_64::r12));
        start_block_builder.AddInstr(std::make_unique<x86_64::Call>("printInt"_f));
    }
    {
        x86_64::BlockBuilder loop_block_builder = main_func_builder.AddBlock();
        loop_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rdi, x86_64::r12));
        loop_block_builder.AddInstr(std::make_unique<x86_64::Call>("printInt"_f));
        loop_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::r14, x86_64::r12));
        loop_block_builder.AddInstr(std::make_unique<x86_64::Add>(x86_64::r14, x86_64::r13));
        loop_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::r13, x86_64::r12));
        loop_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::r12, x86_64::r14));
        loop_block_builder.AddInstr(std::make_unique<x86_64::Sub>(x86_64::r15b, 1_imm8));
        loop_block_builder.AddInstr(std::make_unique<x86_64::Jcc>(x86_64::Jcc::CondType::kAbove,
                                                               loop_block_builder.block()->GetBlockRef()));
    }
    
    // Hello world (Syscall test):
    {
        x86_64::BlockBuilder hello_block_builder = main_func_builder.AddBlock();
        hello_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rax, 0x2000004_imm64)); // write
        hello_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rdi, 1_imm32));         // stdout
        hello_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rsi, str_c));           // const char
        hello_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rdx, 13_imm32));        // size
        hello_block_builder.AddInstr(std::make_unique<x86_64::Syscall>());
        //hello_block_builder.AddInstr(std::make_unique<x86_64::Jmp>(hello_block_builder.block()->GetBlockRef()));
    }
        
    // Other tests:
    {
        x86_64::BlockBuilder test_block_builder = main_func_builder.AddBlock();
        test_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rdi, 1_imm32));
        test_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rsi, 2_imm32));
        test_block_builder.AddInstr(std::make_unique<x86_64::Call>("addInts"_f));
        test_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rdi, x86_64::rax));
        test_block_builder.AddInstr(std::make_unique<x86_64::Sub>(x86_64::rdi, x86_64::rax));
        test_block_builder.AddInstr(std::make_unique<x86_64::Add>(x86_64::rdi, x86_64::rax));
        test_block_builder.AddInstr(std::make_unique<x86_64::Add>(x86_64::rdi, 17_imm8));
        test_block_builder.AddInstr(std::make_unique<x86_64::Sub>(x86_64::rdi, 6_imm8));
        test_block_builder.AddInstr(std::make_unique<x86_64::Call>("printInt"_f));
        test_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rdi, 1233_imm32));
        test_block_builder.AddInstr(std::make_unique<x86_64::Sub>(x86_64::rdi, std::make_shared<x86_64::Imm32>(-1)));
        test_block_builder.AddInstr(std::make_unique<x86_64::Call>("printInt"_f));
    }
        
    // Epilog:
    {
        x86_64::BlockBuilder epilog_block_builder = main_func_builder.AddBlock();
        epilog_block_builder.AddInstr(std::make_unique<x86_64::Mov>(x86_64::rsp, x86_64::rbp));
        epilog_block_builder.AddInstr(std::make_unique<x86_64::Pop>(x86_64::rbp));
        epilog_block_builder.AddInstr(std::make_unique<x86_64::Ret>());
    }
    
    std::shared_ptr<x86_64::Prog> prog = prog_builder.prog();
    
    std::cout << prog->ToString() << std::endl << std::endl;
    
    uint8_t *base = (uint8_t *)mmap(NULL, 1<<12,
                                    PROT_EXEC | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS,
                                    0, 0);
    common::data code(base, 1<<12);
    
    int64_t size = prog->Encode(linker, code);
    linker->ApplyPatches();
    
    for (int64_t j = 0 ; j < size; j++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short) code[j] << " ";
    }
    std::cout << std::endl;
    
    void (*func)(void) = (void(*)(void)) base;
    func();
    
    std::cout << "completed x86-tests\n";
}

void to_file(std::string text, std::filesystem::path out_file) {
    std::ofstream out_stream(out_file, std::ios::out);
    
    out_stream << text;
}

void run_ir_test(std::filesystem::path test_dir) {
    std::string test_name = test_dir.filename();
    std::cout << "testing " + test_name << "\n";
    
    std::filesystem::path in_file = test_dir.string() + "/" + test_name + ".ir.txt";
    std::filesystem::path out_file_base = test_dir.string() + "/" + test_name;
    
    if (!std::filesystem::exists(in_file)) {
        std::cout << "test file not found\n";
        return;
    }
    
    std::ifstream in_stream(in_file, std::ios::in);
    ir_proc::Scanner scanner(in_stream);
    ir::Prog *prog = ir_proc::Parser::Parse(scanner);
    
    std::cout << prog->ToString() << "\n";
    
    for (ir::Func *func : prog->funcs()) {
        vcg::Graph cfg = func->ToControlFlowGraph();
        vcg::Graph dom_tree = func->ToDominatorTree();
        
        to_file(cfg.ToVCGFormat(),
                out_file_base.string() + ".@" + std::to_string(func->number()) + ".cfg.vcg");
        to_file(dom_tree.ToVCGFormat(/*exclude_node_text=*/ false),
                out_file_base.string() + ".@" + std::to_string(func->number()) + ".dom.vcg");
    }
    
    for (ir::Func *func : prog->funcs()) {
        ir_proc::LiveRangeAnalyzer live_range_analyzer(func);
        
        ir_info::FuncLiveRangeInfo& func_live_range_info =
            live_range_analyzer.func_info();
        ir_info::InterferenceGraph& interference_graph =
            live_range_analyzer.interference_graph();
        
        ir_proc::RegisterAllocator
            register_allocator(func, interference_graph);
        register_allocator.AllocateRegisters();
        
        to_file(func_live_range_info.ToString(),
                out_file_base.string() + ".@" + std::to_string(func->number()) + ".live_range_info.txt");
        to_file(interference_graph.ToString(),
                out_file_base.string() + ".@" + std::to_string(func->number()) + ".interference_graph.txt");
        to_file(interference_graph.ToVCGGraph().ToVCGFormat(),
                out_file_base.string() + ".@" + std::to_string(func->number()) + ".interference_graph.vcg");
    }
    
    delete prog;
}

void test_ir() {
    std::filesystem::path ir_tests = "/Users/arne/Documents/Xcode Projects/Katara/Tests/ir-tests";
    
    std::cout << "running ir-tests\n";
    
    for (auto entry : std::filesystem::directory_iterator(ir_tests)) {
        if (!entry.is_directory()) {
            continue;
        }
        run_ir_test(entry.path());
    }
    
    std::cout << "completed ir-tests\n";
}

int main(int argc, const char * argv[]) {
    test_x86();
    test_ir();
    
    return 0;
}


