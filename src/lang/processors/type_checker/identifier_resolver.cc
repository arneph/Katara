//
//  identifier_resolver.cc
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "identifier_resolver.h"

#include "lang/representation/ast/ast_util.h"

namespace lang {
namespace type_checker {

types::Package* IdentifierResolver::CreatePackageAndResolveIdentifiers(
    std::string package_path, std::vector<ast::File*> package_files,
    std::function<types::Package*(std::string)> importer, types::InfoBuilder& info_builder,
    issues::IssueTracker& issues) {
  IdentifierResolver resolver(package_path, package_files, importer, info_builder, issues);

  resolver.CreatePackage();
  resolver.CreateFileScopes();
  resolver.ResolveIdentifiers();

  return resolver.package_;
}

void IdentifierResolver::CreatePackage() {
  std::string package_name = package_path_;
  if (auto pos = package_name.find_last_of('/'); pos != std::string::npos) {
    package_name = package_name.substr(pos + 1);
  }

  package_ = info_builder_.CreatePackage(package_path_, package_name);
}

void IdentifierResolver::CreateFileScopes() {
  for (ast::File* file : package_files_) {
    info_builder_.CreateScope(file, package_->scope());
  }
}

void IdentifierResolver::ResolveIdentifiers() {
  for (ast::File* file : package_files_) {
    for (ast::Decl* decl : file->decls()) {
      switch (decl->node_kind()) {
        case ast::NodeKind::kGenDecl:
          AddDefinedObjectsFromGenDecl(static_cast<ast::GenDecl*>(decl), package_->scope(), file);
          break;
        case ast::NodeKind::kFuncDecl:
          AddDefinedObjectFromFuncDecl(static_cast<ast::FuncDecl*>(decl), package_->scope());
          break;
        default:
          throw "unexpected declaration";
      }
    }
  }

  for (ast::File* file : package_files_) {
    types::Scope* file_scope = info_->scopes().at(file);
    for (ast::Decl* decl : file->decls()) {
      switch (decl->node_kind()) {
        case ast::NodeKind::kGenDecl:
          ResolveIdentifiersInGenDecl(static_cast<ast::GenDecl*>(decl), file_scope);
          break;
        case ast::NodeKind::kFuncDecl:
          ResolveIdentifiersInFuncDecl(static_cast<ast::FuncDecl*>(decl), file_scope);
          break;
        default:
          throw "unexpected declaration";
      }
    }
  }
}

void IdentifierResolver::AddObjectToScope(types::Scope* scope, types::Object* object) {
  if (info_->universe()->Lookup(object->name())) {
    issues_.Add(issues::kRedefinitionOfPredeclaredIdent, object->position(),
                "can not redefine predeclared identifier: " + object->name());
    return;

  } else if (scope->named_objects().contains(object->name())) {
    types::Object* other = scope->named_objects().at(object->name());
    issues_.Add(issues::kRedefinitionOfIdent, {other->position(), object->position()},
                "can not redefine identifier: " + object->name());
    return;
  }

  info_builder_.AddObjectToScope(scope, object);
}

void IdentifierResolver::AddDefinedObjectsFromGenDecl(ast::GenDecl* gen_decl, types::Scope* scope,
                                                      ast::File* file) {
  switch (gen_decl->tok()) {
    case tokens::kImport:
      for (ast::Spec* spec : gen_decl->specs()) {
        AddDefinedObjectsFromImportSpec(static_cast<ast::ImportSpec*>(spec), file);
      }
      return;
    case tokens::kConst:
      for (ast::Spec* spec : gen_decl->specs()) {
        AddDefinedObjectsFromConstSpec(static_cast<ast::ValueSpec*>(spec), scope);
      }
      return;
    case tokens::kVar:
      for (ast::Spec* spec : gen_decl->specs()) {
        AddDefinedObjectsFromVarSpec(static_cast<ast::ValueSpec*>(spec), scope);
      }
      return;
    case tokens::kType:
      for (ast::Spec* spec : gen_decl->specs()) {
        AddDefinedObjectFromTypeSpec(static_cast<ast::TypeSpec*>(spec), scope);
      }
      return;
    default:
      throw "unexpected gen decl token";
  }
}

void IdentifierResolver::AddDefinedObjectsFromImportSpec(ast::ImportSpec* import_spec,
                                                         ast::File* file) {
  std::string name;
  std::string path = import_spec->path()->value();
  path = path.substr(1, path.length() - 2);

  // Create import set for file if not present yet.
  file_imports_[file];

  if (file_imports_.at(file).find(path) != file_imports_.at(file).end()) {
    issues_.Add(issues::kPackageImportedTwice, import_spec->path()->start(),
                "can not import package twice: \"" + path + "\"");
    return;
  }
  types::Package* referenced_package = importer_(path);
  if (referenced_package == nullptr) {
    issues_.Add(issues::kPackageCouldNotBeImported, import_spec->path()->start(),
                "could not import package: \"" + path + "\"");
  }
  file_imports_.at(file).insert(path);

  info_builder_.AddImportToPackage(package_, referenced_package);

  if (import_spec->name() != nullptr) {
    if (import_spec->name()->name() == "_") {
      return;
    }
    name = import_spec->name()->name();
  } else {
    name = path;
    if (auto pos = name.find_last_of('/'); pos != std::string::npos) {
      path = name.substr(pos + 1);
    }
  }

  types::Scope* file_scope = info_->scopes().at(file);
  types::PackageName* package_name = info_builder_.CreatePackageName(
      file_scope, package_, import_spec->start(), name, referenced_package);
  if (import_spec->name() != nullptr) {
    info_builder_.SetDefinedObject(import_spec->name(), package_name);
  } else {
    info_builder_.SetImplicitObject(import_spec, package_name);
  }
  AddObjectToScope(file_scope, package_name);
}

void IdentifierResolver::AddDefinedObjectsFromConstSpec(ast::ValueSpec* value_spec,
                                                        types::Scope* scope) {
  for (ast::Ident* ident : value_spec->names()) {
    if (ident->name() == "_") {
      continue;
    }

    types::Constant* constant =
        info_builder_.CreateConstant(scope, package_, ident->start(), ident->name());
    info_builder_.SetDefinedObject(ident, constant);
    AddObjectToScope(scope, constant);
  }
}

void IdentifierResolver::AddDefinedObjectsFromVarSpec(ast::ValueSpec* value_spec,
                                                      types::Scope* scope) {
  for (ast::Ident* ident : value_spec->names()) {
    if (ident->name() == "_") {
      continue;
    }

    types::Variable* variable =
        info_builder_.CreateVariable(scope, package_, ident->start(), ident->name(),
                                     /* is_embedded= */ false,
                                     /* is_field= */ false);
    info_builder_.SetDefinedObject(ident, variable);
    AddObjectToScope(scope, variable);
  }
}

void IdentifierResolver::AddDefinedObjectFromTypeSpec(ast::TypeSpec* type_spec,
                                                      types::Scope* scope) {
  if (type_spec->name()->name() == "_") {
    issues_.Add(issues::kForbiddenBlankTypeName, type_spec->name()->start(),
                "blank type name not allowed");
    return;
  }

  bool is_alias = type_spec->assign() != pos::kNoPos;
  types::TypeName* type_name = info_builder_.CreateTypeNameForNamedType(
      scope, package_, type_spec->name()->start(), type_spec->name()->name(), is_alias);
  info_builder_.SetDefinedObject(type_spec->name(), type_name);
  AddObjectToScope(scope, type_name);
}

void IdentifierResolver::AddDefinedObjectFromFuncDecl(ast::FuncDecl* func_decl,
                                                      types::Scope* scope) {
  if (func_decl->name()->name() == "_") {
    issues_.Add(issues::kForbiddenBlankFuncName, func_decl->name()->start(),
                "blank func name not allowed");
    return;
  }

  types::Func* func = info_builder_.CreateFunc(scope, package_, func_decl->name()->start(),
                                               func_decl->name()->name());
  info_builder_.SetDefinedObject(func_decl->name(), func);
  if (func_decl->kind() == ast::FuncDecl::Kind::kFunc) {
    AddObjectToScope(scope, func);
  }
}

void IdentifierResolver::ResolveIdentifiersInGenDecl(ast::GenDecl* gen_decl, types::Scope* scope) {
  switch (gen_decl->tok()) {
    case tokens::kImport:
      return;
    case tokens::kConst:
    case tokens::kVar:
      for (ast::Spec* spec : gen_decl->specs()) {
        ResolveIdentifiersInValueSpec(static_cast<ast::ValueSpec*>(spec), scope);
      }
      return;
    case tokens::kType:
      for (ast::Spec* spec : gen_decl->specs()) {
        ResolveIdentifiersInTypeSpec(static_cast<ast::TypeSpec*>(spec), scope);
      }
      return;
    default:
      throw "unexpected gen decl token";
  }
}

void IdentifierResolver::ResolveIdentifiersInValueSpec(ast::ValueSpec* value_spec,
                                                       types::Scope* scope) {
  if (value_spec->type()) {
    ResolveIdentifiersInExpr(value_spec->type(), scope);
  }
  for (ast::Expr* value : value_spec->values()) {
    ResolveIdentifiersInExpr(value, scope);
  }
}

void IdentifierResolver::ResolveIdentifiersInTypeSpec(ast::TypeSpec* type_spec,
                                                      types::Scope* scope) {
  types::Scope* type_scope = info_builder_.CreateScope(type_spec, scope);

  if (type_spec->type_params()) {
    ResolveIdentifiersInTypeParamList(type_spec->type_params(), type_scope);
  }
  ResolveIdentifiersInExpr(type_spec->type(), type_scope);
}

void IdentifierResolver::ResolveIdentifiersInFuncDecl(ast::FuncDecl* func_decl,
                                                      types::Scope* scope) {
  types::Scope* func_scope = info_builder_.CreateScope(func_decl, scope);

  if (func_decl->kind() == ast::FuncDecl::Kind::kInstanceMethod) {
    ResolveIdentifiersInExprReceiver(func_decl->expr_receiver(), func_scope);
  }
  if (func_decl->kind() == ast::FuncDecl::Kind::kTypeMethod) {
    ResolveIdentifiersInTypeReceiver(func_decl->type_receiver(), func_scope);
  }
  if (func_decl->type_params() != nullptr) {
    ResolveIdentifiersInTypeParamList(func_decl->type_params(), func_scope);
  }
  ResolveIdentifiersInRegularFuncFieldList(func_decl->func_type()->params(), func_scope);
  if (func_decl->func_type()->results() != nullptr) {
    ResolveIdentifiersInRegularFuncFieldList(func_decl->func_type()->results(), func_scope);
  }
  if (func_decl->body() != nullptr) {
    ResolveIdentifiersInBlockStmt(func_decl->body(), func_scope);
  }
}

void IdentifierResolver::ResolveIdentifiersInTypeParamList(ast::TypeParamList* type_param_list,
                                                           types::Scope* scope) {
  for (ast::TypeParam* type_param : type_param_list->params()) {
    if (type_param->type() == nullptr) {
      continue;
    }
    ResolveIdentifiersInExpr(type_param->type(), scope);
  }
  for (ast::TypeParam* type_param : type_param_list->params()) {
    if (type_param->name()->name() == "_") {
      issues_.Add(issues::kForbiddenBlankTypeParameterName, type_param->name()->start(),
                  "blank type parameter name not allowed");
      continue;
    }

    types::TypeName* type_name = info_builder_.CreateTypeNameForTypeParameter(
        scope, package_, type_param->name()->start(), type_param->name()->name());
    info_builder_.SetDefinedObject(type_param->name(), type_name);
    AddObjectToScope(scope, type_name);
  }
}

void IdentifierResolver::ResolveIdentifiersInExprReceiver(ast::ExprReceiver* expr_receiver,
                                                          types::Scope* scope) {
  ResolveIdentifier(expr_receiver->type_name(), scope);

  if (!expr_receiver->type_parameter_names().empty()) {
    for (ast::Ident* type_param_name : expr_receiver->type_parameter_names()) {
      types::TypeName* type_name = info_builder_.CreateTypeNameForTypeParameter(
          scope, package_, type_param_name->start(), type_param_name->name());
      info_builder_.SetDefinedObject(type_param_name, type_name);
      AddObjectToScope(scope, type_name);
    }
  }

  if (expr_receiver->name() != nullptr) {
    types::Variable* variable = info_builder_.CreateVariable(
        scope, package_, expr_receiver->name()->start(), expr_receiver->name()->name(),
        /* is_embedded= */ false,
        /* is_field= */ false);
    info_builder_.SetDefinedObject(expr_receiver->name(), variable);
    AddObjectToScope(scope, variable);
  } else {
    types::Variable* variable =
        info_builder_.CreateVariable(scope, package_, expr_receiver->start(), "",
                                     /* is_embedded= */ false,
                                     /* is_field= */ false);
    info_builder_.SetImplicitObject(expr_receiver, variable);
  }
}

void IdentifierResolver::ResolveIdentifiersInTypeReceiver(ast::TypeReceiver* type_receiver,
                                                          types::Scope* scope) {
  ResolveIdentifier(type_receiver->type_name(), scope);

  if (!type_receiver->type_parameter_names().empty()) {
    for (ast::Ident* type_param_name : type_receiver->type_parameter_names()) {
      types::TypeName* type_name = info_builder_.CreateTypeNameForTypeParameter(
          scope, package_, type_param_name->start(), type_param_name->name());
      info_builder_.SetDefinedObject(type_param_name, type_name);
      AddObjectToScope(scope, type_name);
    }
  }
}

void IdentifierResolver::ResolveIdentifiersInRegularFuncFieldList(ast::FieldList* field_list,
                                                                  types::Scope* scope) {
  for (ast::Field* field : field_list->fields()) {
    ResolveIdentifiersInExpr(field->type(), scope);
  }
  for (ast::Field* field : field_list->fields()) {
    for (ast::Ident* name : field->names()) {
      types::Variable* variable =
          info_builder_.CreateVariable(scope, package_, name->start(), name->name(),
                                       /* is_embedded= */ false,
                                       /* is_field= */ false);
      info_builder_.SetDefinedObject(name, variable);
      AddObjectToScope(scope, variable);
    }
    if (field->names().empty()) {
      types::Variable* variable =
          info_builder_.CreateVariable(scope, package_, field->type()->start(),
                                       /* name= */ "",
                                       /* is_embedded= */ false,
                                       /* is_field= */ false);
      info_builder_.SetImplicitObject(field, variable);
      AddObjectToScope(scope, variable);
    }
  }
}

void IdentifierResolver::ResolveIdentifiersInStmt(ast::Stmt* stmt, types::Scope* scope) {
  switch (stmt->node_kind()) {
    case ast::NodeKind::kBlockStmt:
      ResolveIdentifiersInBlockStmt(static_cast<ast::BlockStmt*>(stmt), scope);
      break;
    case ast::NodeKind::kDeclStmt:
      ResolveIdentifiersInDeclStmt(static_cast<ast::DeclStmt*>(stmt), scope);
      break;
    case ast::NodeKind::kAssignStmt:
      ResolveIdentifiersInAssignStmt(static_cast<ast::AssignStmt*>(stmt), scope);
      break;
    case ast::NodeKind::kExprStmt:
      ResolveIdentifiersInExpr(static_cast<ast::ExprStmt*>(stmt)->x(), scope);
      break;
    case ast::NodeKind::kIncDecStmt:
      ResolveIdentifiersInExpr(static_cast<ast::IncDecStmt*>(stmt)->x(), scope);
      break;
    case ast::NodeKind::kReturnStmt:
      for (ast::Expr* expr : static_cast<ast::ReturnStmt*>(stmt)->results()) {
        ResolveIdentifiersInExpr(expr, scope);
      }
      break;
    case ast::NodeKind::kIfStmt:
      ResolveIdentifiersInIfStmt(static_cast<ast::IfStmt*>(stmt), scope);
      break;
    case ast::NodeKind::kExprSwitchStmt:
      ResolveIdentifiersInExprSwitchStmt(static_cast<ast::ExprSwitchStmt*>(stmt), scope);
      break;
    case ast::NodeKind::kTypeSwitchStmt:
      ResolveIdentifiersInTypeSwitchStmt(static_cast<ast::TypeSwitchStmt*>(stmt), scope);
      break;
    case ast::NodeKind::kForStmt:
      ResolveIdentifiersInForStmt(static_cast<ast::ForStmt*>(stmt), scope);
      break;
    case ast::NodeKind::kLabeledStmt:
      ResolveIdentifiersInStmt(static_cast<ast::LabeledStmt*>(stmt)->stmt(), scope);
      break;
    case ast::NodeKind::kBranchStmt:
      ResolveIdentifiersInBranchStmt(static_cast<ast::BranchStmt*>(stmt), scope);
      break;
    default:
      throw "unexpected AST stmt";
  }
}

void IdentifierResolver::ResolveIdentifiersInBlockStmt(ast::BlockStmt* body, types::Scope* scope) {
  for (ast::Stmt* stmt : body->stmts()) {
    if (stmt->node_kind() != ast::NodeKind::kLabeledStmt) {
      continue;
    }
    ast::LabeledStmt* labeled_stmt = static_cast<ast::LabeledStmt*>(stmt);
    types::Label* label = info_builder_.CreateLabel(scope, package_, labeled_stmt->start(),
                                                    labeled_stmt->label()->name());
    info_builder_.SetDefinedObject(labeled_stmt->label(), label);
    AddObjectToScope(scope, label);
  }
  for (ast::Stmt* stmt : body->stmts()) {
    ResolveIdentifiersInStmt(stmt, scope);
  }
}

void IdentifierResolver::ResolveIdentifiersInDeclStmt(ast::DeclStmt* decl_stmt,
                                                      types::Scope* scope) {
  switch (decl_stmt->decl()->tok()) {
    case tokens::kConst:
      for (ast::Spec* spec : decl_stmt->decl()->specs()) {
        ast::ValueSpec* value_spec = static_cast<ast::ValueSpec*>(spec);
        ResolveIdentifiersInValueSpec(value_spec, scope);
        AddDefinedObjectsFromConstSpec(value_spec, scope);
      }
      return;
    case tokens::kVar:
      for (ast::Spec* spec : decl_stmt->decl()->specs()) {
        ast::ValueSpec* value_spec = static_cast<ast::ValueSpec*>(spec);
        ResolveIdentifiersInValueSpec(value_spec, scope);
        AddDefinedObjectsFromVarSpec(value_spec, scope);
      }
      return;
    case tokens::kType:
      for (ast::Spec* spec : decl_stmt->decl()->specs()) {
        ast::TypeSpec* type_spec = static_cast<ast::TypeSpec*>(spec);
        AddDefinedObjectFromTypeSpec(type_spec, scope);
        ResolveIdentifiersInTypeSpec(type_spec, scope);
      }
      return;
    default:
      throw "unexpected gen decl token";
  }
}

void IdentifierResolver::ResolveIdentifiersInAssignStmt(ast::AssignStmt* assign_stmt,
                                                        types::Scope* scope) {
  for (ast::Expr* expr : assign_stmt->rhs()) {
    ResolveIdentifiersInExpr(expr, scope);
  }
  for (ast::Expr* expr : assign_stmt->lhs()) {
    if (assign_stmt->tok() == tokens::kDefine && expr->node_kind() == ast::NodeKind::kIdent) {
      ast::Ident* ident = static_cast<ast::Ident*>(expr);
      const types::Scope* defining_scope = nullptr;
      scope->Lookup(ident->name(), defining_scope);
      if (defining_scope != scope) {
        types::Variable* variable =
            info_builder_.CreateVariable(scope, package_, ident->start(), ident->name(),
                                         /* is_embedded= */ false,
                                         /* is_field= */ false);
        info_builder_.SetDefinedObject(ident, variable);
        AddObjectToScope(scope, variable);
      }
    }
    ResolveIdentifiersInExpr(expr, scope);
  }
}

void IdentifierResolver::ResolveIdentifiersInIfStmt(ast::IfStmt* if_stmt, types::Scope* scope) {
  types::Scope* if_scope = info_builder_.CreateScope(if_stmt, scope);

  if (if_stmt->init_stmt()) {
    ResolveIdentifiersInStmt(if_stmt->init_stmt(), if_scope);
  }
  ResolveIdentifiersInExpr(if_stmt->cond_expr(), if_scope);
  ResolveIdentifiersInBlockStmt(if_stmt->body(), if_scope);
  if (if_stmt->else_stmt()) {
    ResolveIdentifiersInStmt(if_stmt->else_stmt(), scope);
  }
}

void IdentifierResolver::ResolveIdentifiersInExprSwitchStmt(ast::ExprSwitchStmt* switch_stmt,
                                                            types::Scope* scope) {
  types::Scope* switch_scope = info_builder_.CreateScope(switch_stmt, scope);

  if (switch_stmt->init_stmt()) {
    ResolveIdentifiersInStmt(switch_stmt->init_stmt(), switch_scope);
  }
  if (switch_stmt->tag_expr()) {
    ResolveIdentifiersInExpr(switch_stmt->tag_expr(), switch_scope);
  }
  for (ast::Stmt* stmt : switch_stmt->body()->stmts()) {
    ast::CaseClause* case_clause = static_cast<ast::CaseClause*>(stmt);
    ResolveIdentifiersInCaseClause(case_clause, switch_scope);
  }
}

void IdentifierResolver::ResolveIdentifiersInTypeSwitchStmt(ast::TypeSwitchStmt* switch_stmt,
                                                            types::Scope* scope) {
  types::Scope* switch_scope = info_builder_.CreateScope(switch_stmt, scope);

  ResolveIdentifiersInExpr(switch_stmt->tag_expr(), scope);
  for (ast::Stmt* stmt : switch_stmt->body()->stmts()) {
    ast::CaseClause* case_clause = static_cast<ast::CaseClause*>(stmt);
    ResolveIdentifiersInCaseClause(case_clause, switch_scope, switch_stmt->var());
  }
}

void IdentifierResolver::ResolveIdentifiersInCaseClause(ast::CaseClause* case_clause,
                                                        types::Scope* scope,
                                                        ast::Ident* type_switch_var) {
  types::Scope* case_scope = info_builder_.CreateScope(case_clause, scope);

  for (ast::Expr* expr : case_clause->cond_vals()) {
    ResolveIdentifiersInExpr(expr, case_scope);
  }
  if (type_switch_var != nullptr) {
    types::Variable* variable = info_builder_.CreateVariable(
        case_scope, package_, type_switch_var->start(), type_switch_var->name(),
        /* is_embedded= */ false,
        /* is_field= */ false);
    info_builder_.SetImplicitObject(case_clause, variable);
    AddObjectToScope(case_scope, variable);
  }
  for (ast::Stmt* stmt : case_clause->body()) {
    if (stmt->node_kind() != ast::NodeKind::kLabeledStmt) {
      continue;
    }
    ast::LabeledStmt* labeled_stmt = static_cast<ast::LabeledStmt*>(stmt);
    types::Label* label = info_builder_.CreateLabel(case_scope, package_, labeled_stmt->start(),
                                                    labeled_stmt->label()->name());
    info_builder_.SetDefinedObject(labeled_stmt->label(), label);
    AddObjectToScope(case_scope, label);
  }
  for (ast::Stmt* stmt : case_clause->body()) {
    ResolveIdentifiersInStmt(stmt, case_scope);
  }
}

void IdentifierResolver::ResolveIdentifiersInForStmt(ast::ForStmt* for_stmt, types::Scope* scope) {
  types::Scope* for_scope = info_builder_.CreateScope(for_stmt, scope);

  if (for_stmt->init_stmt()) {
    ResolveIdentifiersInStmt(for_stmt->init_stmt(), for_scope);
  }
  if (for_stmt->cond_expr()) {
    ResolveIdentifiersInExpr(for_stmt->cond_expr(), for_scope);
  }
  if (for_stmt->post_stmt()) {
    // Note: parser already checks that post_stmt does not define variables.
    ResolveIdentifiersInStmt(for_stmt->post_stmt(), for_scope);
  }
  ResolveIdentifiersInBlockStmt(for_stmt->body(), for_scope);
}

void IdentifierResolver::ResolveIdentifiersInBranchStmt(ast::BranchStmt* branch_stmt,
                                                        types::Scope* scope) {
  if (!branch_stmt->label()) {
    return;
  }
  const types::Scope* defining_scope;
  types::Object* obj = scope->Lookup(branch_stmt->label()->name(), defining_scope);
  if (obj->object_kind() != types::ObjectKind::kLabel) {
    issues_.Add(issues::kUnresolvedBranchStmtLabel, branch_stmt->label()->start(),
                "branch statement does not refer to known label");
    return;
  }
  ResolveIdentifiersInExpr(branch_stmt->label(), scope);
}

void IdentifierResolver::ResolveIdentifiersInExpr(ast::Expr* expr, types::Scope* scope) {
  switch (expr->node_kind()) {
    case ast::NodeKind::kUnaryExpr:
      ResolveIdentifiersInExpr(static_cast<ast::UnaryExpr*>(expr)->x(), scope);
      break;
    case ast::NodeKind::kBinaryExpr:
      ResolveIdentifiersInExpr(static_cast<ast::BinaryExpr*>(expr)->x(), scope);
      ResolveIdentifiersInExpr(static_cast<ast::BinaryExpr*>(expr)->y(), scope);
      break;
    case ast::NodeKind::kCompareExpr:
      for (ast::Expr* operand : static_cast<ast::CompareExpr*>(expr)->operands()) {
        ResolveIdentifiersInExpr(operand, scope);
      }
      break;
    case ast::NodeKind::kParenExpr:
      ResolveIdentifiersInExpr(static_cast<ast::ParenExpr*>(expr)->x(), scope);
      break;
    case ast::NodeKind::kSelectionExpr:
      ResolveIdentifiersInSelectionExpr(static_cast<ast::SelectionExpr*>(expr), scope);
      break;
    case ast::NodeKind::kTypeAssertExpr: {
      ast::TypeAssertExpr* type_assert_expr = static_cast<ast::TypeAssertExpr*>(expr);
      ResolveIdentifiersInExpr(type_assert_expr->x(), scope);
      if (type_assert_expr->type()) {
        ResolveIdentifiersInExpr(type_assert_expr->type(), scope);
      }
      break;
    }
    case ast::NodeKind::kIndexExpr:
      ResolveIdentifiersInExpr(static_cast<ast::IndexExpr*>(expr)->accessed(), scope);
      ResolveIdentifiersInExpr(static_cast<ast::IndexExpr*>(expr)->index(), scope);
      break;
    case ast::NodeKind::kCallExpr: {
      ast::CallExpr* call_expr = static_cast<ast::CallExpr*>(expr);
      ResolveIdentifiersInExpr(call_expr->func(), scope);
      for (ast::Expr* type_arg : call_expr->type_args()) {
        ResolveIdentifiersInExpr(type_arg, scope);
      }
      for (ast::Expr* arg : call_expr->args()) {
        ResolveIdentifiersInExpr(arg, scope);
      }
      break;
    }
    case ast::NodeKind::kFuncLit:
      ResolveIdentifiersInFuncLit(static_cast<ast::FuncLit*>(expr), scope);
      break;
    case ast::NodeKind::kCompositeLit:
      ResolveIdentifiersInCompositeLit(static_cast<ast::CompositeLit*>(expr), scope);
      break;
    case ast::NodeKind::kArrayType: {
      ast::ArrayType* array_type = static_cast<ast::ArrayType*>(expr);
      if (array_type->len()) {
        ResolveIdentifiersInExpr(array_type->len(), scope);
      }
      ResolveIdentifiersInExpr(array_type->element_type(), scope);
      break;
    }
    case ast::NodeKind::kFuncType:
      ResolveIdentifiersInFuncType(static_cast<ast::FuncType*>(expr), scope);
      break;
    case ast::NodeKind::kInterfaceType:
      ResolveIdentifiersInInterfaceType(static_cast<ast::InterfaceType*>(expr), scope);
      break;
    case ast::NodeKind::kStructType:
      ResolveIdentifiersInStructType(static_cast<ast::StructType*>(expr), scope);
      break;
    case ast::NodeKind::kTypeInstance:
      ResolveIdentifiersInExpr(static_cast<ast::TypeInstance*>(expr)->type(), scope);
      for (ast::Expr* type_arg : static_cast<ast::TypeInstance*>(expr)->type_args()) {
        ResolveIdentifiersInExpr(type_arg, scope);
      }
      break;
    case ast::NodeKind::kBasicLit:
      break;
    case ast::NodeKind::kIdent:
      ResolveIdentifier(static_cast<ast::Ident*>(expr), scope);
      break;
    default:
      throw "unexpected AST expr";
  }
}

void IdentifierResolver::ResolveIdentifiersInSelectionExpr(ast::SelectionExpr* sel,
                                                           types::Scope* scope) {
  ResolveIdentifiersInExpr(sel->accessed(), scope);
  if (sel->selection()->name() == "_") {
    issues_.Add(issues::kForbiddenBlankSelectionName, sel->selection()->start(),
                "blank selection name not allowed");
    return;
  }

  if (sel->accessed()->node_kind() != ast::NodeKind::kIdent) {
    return;
  }
  auto it = info_->uses().find(static_cast<ast::Ident*>(sel->accessed()));
  if (it == info_->uses().end() || it->second->object_kind() != types::ObjectKind::kPackageName) {
    return;
  }
  types::PackageName* pkg_name = static_cast<types::PackageName*>(it->second);
  if (pkg_name->referenced_package() == nullptr) {
    return;
  }
  ast::Ident* selected_ident = sel->selection();
  ResolveIdentifier(selected_ident, pkg_name->referenced_package()->scope());
}

void IdentifierResolver::ResolveIdentifiersInFuncLit(ast::FuncLit* func_lit, types::Scope* scope) {
  types::Func* func = info_builder_.CreateFunc(scope, package_, func_lit->start(),
                                               /* name= */ "");
  info_builder_.SetImplicitObject(func_lit, func);
  AddObjectToScope(scope, func);

  types::Scope* func_scope = info_builder_.CreateScope(func_lit, scope);

  ResolveIdentifiersInRegularFuncFieldList(func_lit->type()->params(), func_scope);
  if (func_lit->type()->results()) {
    ResolveIdentifiersInRegularFuncFieldList(func_lit->type()->results(), func_scope);
  }
  ResolveIdentifiersInBlockStmt(func_lit->body(), func_scope);
}

void IdentifierResolver::ResolveIdentifiersInCompositeLit(ast::CompositeLit* composite_lit,
                                                          types::Scope* scope) {
  ResolveIdentifiersInExpr(composite_lit->type(), scope);
  for (ast::Expr* value : composite_lit->values()) {
    ast::Expr* expr = value;
    if (expr->node_kind() == ast::NodeKind::kKeyValueExpr) {
      expr = static_cast<ast::KeyValueExpr*>(value)->value();
    }
    ResolveIdentifiersInExpr(expr, scope);
  }
}

void IdentifierResolver::ResolveIdentifiersInFuncType(ast::FuncType* func_type,
                                                      types::Scope* scope) {
  types::Scope* func_scope = info_builder_.CreateScope(func_type, scope);

  ResolveIdentifiersInRegularFuncFieldList(func_type->params(), func_scope);
  if (func_type->results()) {
    ResolveIdentifiersInRegularFuncFieldList(func_type->results(), func_scope);
  }
}

void IdentifierResolver::ResolveIdentifiersInInterfaceType(ast::InterfaceType* interface_type,
                                                           types::Scope* scope) {
  types::Scope* interface_scope = info_builder_.CreateScope(interface_type, scope);

  for (ast::MethodSpec* method_spec : interface_type->methods()) {
    types::Scope* method_scope = info_builder_.CreateScope(method_spec, interface_scope);

    if (method_spec->instance_type_param() != nullptr) {
      ast::Ident* name = method_spec->instance_type_param();
      types::TypeName* instance_type_param = info_builder_.CreateTypeNameForTypeParameter(
          method_scope, package_, name->start(), name->name());
      info_builder_.SetDefinedObject(name, instance_type_param);
      AddObjectToScope(method_scope, instance_type_param);
    }

    ResolveIdentifiersInRegularFuncFieldList(method_spec->params(), method_scope);
    if (method_spec->results()) {
      ResolveIdentifiersInRegularFuncFieldList(method_spec->results(), method_scope);
    }
  }
  for (ast::MethodSpec* method_spec : interface_type->methods()) {
    types::Func* method = info_builder_.CreateFunc(
        interface_scope, package_, method_spec->name()->start(), method_spec->name()->name());
    info_builder_.SetDefinedObject(method_spec->name(), method);
    AddObjectToScope(interface_scope, method);
  }
}

void IdentifierResolver::ResolveIdentifiersInStructType(ast::StructType* struct_type,
                                                        types::Scope* scope) {
  types::Scope* struct_scope = info_builder_.CreateScope(struct_type, scope);

  for (ast::Field* field : struct_type->fields()->fields()) {
    ResolveIdentifiersInExpr(field->type(), scope);
  }
  for (ast::Field* field : struct_type->fields()->fields()) {
    if (field->names().empty()) {
      ast::Expr* type = field->type();
      if (type->node_kind() == ast::NodeKind::kUnaryExpr) {
        ast::UnaryExpr* ptr_type = static_cast<ast::UnaryExpr*>(type);
        if (ptr_type->op() != tokens::kMul && ptr_type->op() != tokens::kRem) {
          issues_.Add(issues::kForbiddenEmbeddedFieldType, type->start(),
                      "expected embedded field to be defined type or pointer to defined type");
          continue;
        }
        type = ptr_type->x();
      }
      if (type->node_kind() == ast::NodeKind::kTypeInstance) {
        type = static_cast<ast::TypeInstance*>(type)->type();
      }
      if (type->node_kind() != ast::NodeKind::kIdent) {
        issues_.Add(issues::kForbiddenEmbeddedFieldType, type->start(),
                    "expected embedded field to be defined type or pointer to defined type");
        continue;
      }
      ast::Ident* defined_type = static_cast<ast::Ident*>(type);
      types::Variable* variable = info_builder_.CreateVariable(
          struct_scope, package_, field->type()->start(), defined_type->name(),
          /* is_embedded= */ true,
          /* is_field= */ true);
      info_builder_.SetImplicitObject(field, variable);
      AddObjectToScope(struct_scope, variable);
    } else {
      for (ast::Ident* name : field->names()) {
        types::Variable* variable =
            info_builder_.CreateVariable(struct_scope, package_, name->start(), name->name(),
                                         /* is_embedded= */ false,
                                         /* is_field= */ true);
        info_builder_.SetDefinedObject(name, variable);
        AddObjectToScope(struct_scope, variable);
      }
    }
  }
}

void IdentifierResolver::ResolveIdentifier(ast::Ident* ident, types::Scope* scope) {
  if (ident->name() == "_") {
    return;
  }
  types::Object* object = scope->Lookup(ident->name());
  if (object == nullptr) {
    issues_.Add(issues::kUnresolvedIdentifier, ident->start(),
                "could not resolve identifier: " + ident->name());
  }
  info_builder_.SetUsedObject(ident, object);
}

}  // namespace type_checker
}  // namespace lang
