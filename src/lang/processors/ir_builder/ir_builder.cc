//
//  ir_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 4/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "ir_builder.h"

#include <optional>

namespace lang {
namespace ir_builder {

std::unique_ptr<ir::Program> IRBuilder::TranslateProgram(packages::Package* main_package,
                                                         types::Info* type_info) {
  auto prog = std::make_unique<ir::Program>();
  auto builder = std::unique_ptr<IRBuilder>(new IRBuilder(type_info, prog));

  for (auto [file_name, file] : main_package->ast_package()->files()) {
    builder->PrepareDeclsInFile(file);
  }
  for (auto [file_name, file] : main_package->ast_package()->files()) {
    builder->BuildDeclsInFile(file);
  }

  return prog;
}

IRBuilder::IRBuilder(types::Info* type_info, std::unique_ptr<ir::Program>& program)
    : type_info_(type_info),
      type_builder_(type_info, program),
      expr_builder_(type_info, type_builder_, funcs_),
      stmt_builder_(type_info, type_builder_, expr_builder_),
      program_(program) {}

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
      throw "not implemented";
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
      throw "internal error: unexpected decl";
    }
  }
}

void IRBuilder::BuildFuncDecl(ast::FuncDecl* func_decl) {
  types::Func* types_func = static_cast<types::Func*>(type_info_->DefinitionOf(func_decl->name()));
  types::Signature* types_signature = static_cast<types::Signature*>(types_func->type());
  ir::Func* ir_func = funcs_.at(types_func);
  ir::Block* entry_block = ir_func->AddBlock();
  ir_func->set_entry_block_num(entry_block->number());
  Context ctx(ir_func);

  for (types::Variable* types_arg : types_signature->parameters()->variables()) {
    types::Type* types_arg_type = types_arg->type();
    const ir::Type* ir_arg_type = type_builder_.BuildType(types_arg_type);
    std::shared_ptr<ir::Computed> ir_arg =
        std::make_shared<ir::Computed>(ir_arg_type, ctx.func()->next_computed_number());
    ir_func->args().push_back(ir_arg);
  }
  if (types_signature->results() != nullptr) {
    for (types::Variable* types_result : types_signature->results()->variables()) {
      types::Type* types_result_type = types_result->type();
      const ir::Type* ir_result_type = type_builder_.BuildType(types_result_type);
      ir_func->result_types().push_back(ir_result_type);
    }
  }

  BuildPrologForFunc(types_func, ctx);
  stmt_builder_.BuildBlockStmt(func_decl->body(), ctx);
  if (ctx.block()->instrs().empty() ||
      ctx.block()->instrs().back()->instr_kind() != ir::InstrKind::kReturn) {
    BuildEpilogForFunc(ctx);
  }
}

void IRBuilder::BuildPrologForFunc(types::Func* types_func, Context& ctx) {
  types::Signature* signature = static_cast<types::Signature*>(types_func->type());
  for (types::Variable* param : signature->parameters()->variables()) {
    stmt_builder_.BuildVarDecl(param, ctx);
  }
  if (signature->results() != nullptr) {
    for (types::Variable* result : signature->results()->variables()) {
      if (result->name().empty()) {
        continue;
      }
      stmt_builder_.BuildVarDecl(result, ctx);
    }
  }
  // TODO: handle args and results
}

void IRBuilder::BuildEpilogForFunc(Context& ctx) {
  for (const Context* c = &ctx; c != nullptr; c = c->parent_ctx()) {
    for (auto [var, address] : c->var_addresses()) {
      stmt_builder_.BuildVarDeletion(var, ctx);
    }
  }
}

}  // namespace ir_builder
}  // namespace lang
