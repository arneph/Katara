//
//  ir_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 4/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "ir_builder.h"

#include <optional>

#include "src/common/logging/logging.h"

namespace lang {
namespace ir_builder {

using ::common::logging::fail;
using ::lang::runtime::AddRuntimeFuncsToProgram;
using ::lang::runtime::RuntimeFuncs;

ProgramWithRuntime IRBuilder::TranslateProgram(packages::Package* main_package,
                                               types::Info* type_info) {
  auto program = std::make_unique<ir::Program>();
  RuntimeFuncs runtime = AddRuntimeFuncsToProgram(program.get());
  IRBuilder builder = IRBuilder(type_info, program.get(), runtime);

  for (auto [file_name, file] : main_package->ast_package()->files()) {
    builder.PrepareDeclsInFile(file);
  }
  for (auto [file_name, file] : main_package->ast_package()->files()) {
    builder.BuildDeclsInFile(file);
  }

  return ProgramWithRuntime{
      .program = std::move(program),
      .runtime = runtime,
  };
}

IRBuilder::IRBuilder(types::Info* type_info, ir::Program* program, RuntimeFuncs& runtime)
    : type_info_(type_info),
      type_builder_(type_info, program),
      value_builder_(type_builder_),
      expr_builder_(type_info, type_builder_, value_builder_, funcs_),
      stmt_builder_(type_info, type_builder_, value_builder_, expr_builder_),
      program_(program),
      runtime_(runtime) {}

void IRBuilder::PrepareDeclsInFile(ast::File* file) {
  for (auto decl : file->decls()) {
    if (decl->node_kind() == ast::NodeKind::kGenDecl) {
      ast::GenDecl* gen_decl = static_cast<ast::GenDecl*>(decl);
      if (gen_decl->tok() == tokens::kImport) {
        continue;
      } else {
        continue;  // TODO: implement
      }
    } else if (decl->node_kind() == ast::NodeKind::kFuncDecl) {
      PrepareFuncDecl(static_cast<ast::FuncDecl*>(decl));
    } else {
      fail("not implemented");
    }
  }
}

void IRBuilder::PrepareFuncDecl(ast::FuncDecl* func_decl) {
  types::Func* types_func = static_cast<types::Func*>(type_info_->DefinitionOf(func_decl->name()));
  ir::Func* ir_func = program_->AddFunc();
  ir_func->set_name(func_decl->name()->name());
  funcs_.insert({types_func, ir_func});
  if (ir_func->name() == "main") {
    program_->set_entry_func_num(ir_func->number());
  }
}

void IRBuilder::BuildDeclsInFile(ast::File* file) {
  for (auto decl : file->decls()) {
    if (decl->node_kind() == ast::NodeKind::kGenDecl) {
      ast::GenDecl* gen_decl = static_cast<ast::GenDecl*>(decl);
      if (gen_decl->tok() == tokens::kImport) {
        continue;
      } else {
        continue;  // TODO: implement
      }
    } else if (decl->node_kind() == ast::NodeKind::kFuncDecl) {
      BuildFuncDecl(static_cast<ast::FuncDecl*>(decl));
    } else {
      fail("unexpected decl");
    }
  }
}

void IRBuilder::BuildFuncDecl(ast::FuncDecl* func_decl) {
  types::Func* types_func = static_cast<types::Func*>(type_info_->DefinitionOf(func_decl->name()));
  types::Signature* types_signature = static_cast<types::Signature*>(types_func->type());
  ir::Func* ir_func = funcs_.at(types_func);
  ir::Block* entry_block = ir_func->AddBlock();
  ir_func->set_entry_block_num(entry_block->number());
  ASTContext ast_ctx;
  IRContext ir_ctx(ir_func, entry_block);

  BuildFuncParameters(types_signature->parameters(), ast_ctx, ir_ctx);
  BuildFuncResults(types_signature->results(), ast_ctx, ir_ctx);

  stmt_builder_.BuildBlockStmt(func_decl->body(), ast_ctx, ir_ctx);
  if (!ir_ctx.Completed()) {
    stmt_builder_.BuildVarDeletionsForASTContext(&ast_ctx, ir_ctx);
    ir_ctx.block()->instrs().push_back(std::make_unique<ir::ReturnInstr>());
  }
}

void IRBuilder::BuildFuncParameters(types::Tuple* parameters, ASTContext& ast_ctx,
                                    IRContext& ir_ctx) {
  for (types::Variable* var : parameters->variables()) {
    types::Type* types_type = var->type();
    const ir::Type* ir_type = type_builder_.BuildType(types_type);
    std::shared_ptr<ir::Computed> ir_func_arg =
        std::make_shared<ir::Computed>(ir_type, ir_ctx.func()->next_computed_number());
    ir_ctx.func()->args().push_back(ir_func_arg);
    stmt_builder_.BuildVarDecl(var, ast_ctx, ir_ctx);
    std::shared_ptr<ir::Value> address = ast_ctx.LookupAddressOfVar(var);
    ir_ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(address, ir_func_arg));
  }
}

void IRBuilder::BuildFuncResults(types::Tuple* results, ASTContext& ast_ctx, IRContext& ir_ctx) {
  if (results == nullptr) {
    return;
  }
  for (types::Variable* types_result : results->variables()) {
    types::Type* types_result_type = types_result->type();
    const ir::Type* ir_result_type = type_builder_.BuildType(types_result_type);
    ir_ctx.func()->result_types().push_back(ir_result_type);
    if (!types_result->name().empty()) {
      stmt_builder_.BuildVarDecl(types_result, ast_ctx, ir_ctx);
    }
  }
}

}  // namespace ir_builder
}  // namespace lang
