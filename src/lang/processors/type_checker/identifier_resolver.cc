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

types::Package * IdentifierResolver::CreatePackageAndResolveIdentifiers(
        std::string package_path,
        std::vector<ast::File *> package_files,
        std::function<types::Package *(std::string)> importer,
        types::InfoBuilder& info_builder,
        std::vector<issues::Issue> &issues) {
    IdentifierResolver resolver(package_path,
                                package_files,
                                importer,
                                info_builder,
                                issues);
    
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
    for (ast::File *file : package_files_) {
        info_builder_.CreateScope(file, package_->scope());
    }
}

void IdentifierResolver::ResolveIdentifiers() {
    for (ast::File *file : package_files_) {
        for (ast::Decl *decl : file->decls()) {
            if (ast::GenDecl *gen_decl = dynamic_cast<ast::GenDecl *>(decl)) {
                AddDefinedObjectsFromGenDecl(gen_decl, package_->scope(), file);
            } else if (ast::FuncDecl *func_decl = dynamic_cast<ast::FuncDecl *>(decl)) {
                AddDefinedObjectFromFuncDecl(func_decl, package_->scope());
            } else {
                throw "unexpected declaration";
            }
        }
    }
    
    for (ast::File *file : package_files_) {
        types::Scope *file_scope = info_->scopes().at(file);
        
        for (ast::Decl *decl : file->decls()) {
            if (ast::GenDecl *gen_decl = dynamic_cast<ast::GenDecl *>(decl)) {
                ResolveIdentifiersInGenDecl(gen_decl, file_scope);
            } else if (ast::FuncDecl *func_decl = dynamic_cast<ast::FuncDecl *>(decl)) {
                ResolveIdentifiersInFuncDecl(func_decl, file_scope);
            } else {
                throw "unexpected declaration";
            }
        }
    }
}

void IdentifierResolver::AddObjectToScope(types::Scope *scope, types::Object *object) {
    if (info_->universe()->Lookup(object->name())) {
        issues_.push_back(
            issues::Issue(issues::Origin::TypeChecker,
                          issues::Severity::Error,
                          object->position(),
                          "can not redefine predeclared identifier: " + object->name()));
        return;
        
    } else if (scope->named_objects().contains(object->name())) {
        types::Object *other = scope->named_objects().at(object->name());
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        {other->position(), object->position()},
                                        "naming collision: " + object->name()));
        return;
    }
    
    info_builder_.AddObjectToScope(scope, object);
}

void IdentifierResolver::AddDefinedObjectsFromGenDecl(ast::GenDecl *gen_decl,
                                               types::Scope *scope,
                                               ast::File *file) {
    switch (gen_decl->tok()) {
        case tokens::kImport:
            for (ast::Spec *spec : gen_decl->specs()) {
                AddDefinedObjectsFromImportSpec(static_cast<ast::ImportSpec *>(spec), file);
            }
            return;
        case tokens::kConst:
            for (ast::Spec *spec : gen_decl->specs()) {
                AddDefinedObjectsFromConstSpec(static_cast<ast::ValueSpec *>(spec), scope);
            }
            return;
        case tokens::kVar:
            for (ast::Spec *spec : gen_decl->specs()) {
                AddDefinedObjectsFromVarSpec(static_cast<ast::ValueSpec *>(spec), scope);
            }
            return;
        case tokens::kType:
            for (ast::Spec *spec : gen_decl->specs()) {
                AddDefinedObjectFromTypeSpec(static_cast<ast::TypeSpec *>(spec), scope);
            }
            return;
        default:
            throw "unexpected gen decl token";
    }
}

void IdentifierResolver::AddDefinedObjectsFromImportSpec(ast::ImportSpec *import_spec,
                                                         ast::File *file) {
    std::string name;
    std::string path = import_spec->path()->value();
    path = path.substr(1, path.length()-2);
    
    // Create import set for file if not present yet.
    file_imports_[file];
    
    if (file_imports_.at(file).find(path) != file_imports_.at(file).end()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        import_spec->path()->start(),
                                        "can not import package twice: \"" + path + "\""));
        return;
    }
    types::Package *referenced_package = importer_(path);
    if (referenced_package == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        import_spec->path()->start(),
                                        "could not import package: \"" + path + "\""));
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
            path = name.substr(pos+1);
        }
    }
    
    types::Scope *file_scope = info_->scopes().at(file);
    types::PackageName *package_name = info_builder_.CreatePackageName(file_scope,
                                                                       package_,
                                                                       import_spec->start(),
                                                                       name,
                                                                       referenced_package);
    if (import_spec->name() != nullptr) {
        info_builder_.SetDefinedObject(import_spec->name(), package_name);
    } else {
        info_builder_.SetImplicitObject(import_spec, package_name);
    }
    AddObjectToScope(file_scope, package_name);
}

void IdentifierResolver::AddDefinedObjectsFromConstSpec(ast::ValueSpec *value_spec,
                                                        types::Scope *scope) {
    for (ast::Ident *ident : value_spec->names()) {
        if (ident->name() == "_") {
            continue;
        }
        
        types::Constant *constant = info_builder_.CreateConstant(scope,
                                                                 package_,
                                                                 ident->start(),
                                                                 ident->name());
        info_builder_.SetDefinedObject(ident, constant);
        AddObjectToScope(scope, constant);
    }
}

void IdentifierResolver::AddDefinedObjectsFromVarSpec(ast::ValueSpec *value_spec,
                                                      types::Scope *scope) {
    for (ast::Ident *ident : value_spec->names()) {
        if (ident->name() == "_") {
            continue;
        }
        
        types::Variable *variable = info_builder_.CreateVariable(scope,
                                                                 package_,
                                                                 ident->start(),
                                                                 ident->name(),
                                                                 /* is_embedded= */ false,
                                                                 /* is_field= */ false);
        info_builder_.SetDefinedObject(ident, variable);
        AddObjectToScope(scope, variable);
    }
}

void IdentifierResolver::AddDefinedObjectFromTypeSpec(ast::TypeSpec *type_spec,
                                                      types::Scope *scope) {
    if (type_spec->name()->name() == "_") {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_spec->name()->start(),
                                        "blank type name not allowed"));
        return;
    }
    
    bool is_alias = type_spec->assign() != pos::kNoPos;
    types::TypeName *type_name = info_builder_.CreateTypeNameForNamedType(scope,
                                                                          package_,
                                                                          type_spec->name()->start(),
                                                                          type_spec->name()->name(),
                                                                          is_alias);
    info_builder_.SetDefinedObject(type_spec->name(), type_name);
    AddObjectToScope(scope, type_name);
}

void IdentifierResolver::AddDefinedObjectFromFuncDecl(ast::FuncDecl *func_decl,
                                                      types::Scope *scope) {
    if (func_decl->name()->name() == "_") {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        func_decl->name()->start(),
                                        "blank func name not allowed"));
        return;
    }
    
    types::Func *func = info_builder_.CreateFunc(scope,
                                                 package_,
                                                 func_decl->name()->start(),
                                                 func_decl->name()->name());
    info_builder_.SetDefinedObject(func_decl->name(), func);
    if (func_decl->kind() == ast::FuncDecl::Kind::kFunc) {
        AddObjectToScope(scope, func);
    }
}

void IdentifierResolver::ResolveIdentifiersInGenDecl(ast::GenDecl *gen_decl, types::Scope *scope) {
    switch (gen_decl->tok()) {
        case tokens::kImport:
            return;
        case tokens::kConst:
        case tokens::kVar:
            for (ast::Spec *spec : gen_decl->specs()) {
                ResolveIdentifiersInValueSpec(static_cast<ast::ValueSpec *>(spec), scope);
            }
            return;
        case tokens::kType:
            for (ast::Spec *spec : gen_decl->specs()) {
                ResolveIdentifiersInTypeSpec(static_cast<ast::TypeSpec *>(spec), scope);
            }
            return;
        default:
            throw "unexpected gen decl token";
    }
}

void IdentifierResolver::ResolveIdentifiersInValueSpec(ast::ValueSpec *value_spec,
                                                       types::Scope *scope) {
    if (value_spec->type()) {
        ResolveIdentifiersInExpr(value_spec->type(), scope);
    }
    for (ast::Expr *value : value_spec->values()) {
        ResolveIdentifiersInExpr(value, scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInTypeSpec(ast::TypeSpec *type_spec,
                                                      types::Scope *scope) {
    types::Scope *type_scope = info_builder_.CreateScope(type_spec, scope);
    
    if (type_spec->type_params()) {
        ResolveIdentifiersInTypeParamList(type_spec->type_params(), type_scope);
    }
    ResolveIdentifiersInExpr(type_spec->type(), type_scope);
}

void IdentifierResolver::ResolveIdentifiersInFuncDecl(ast::FuncDecl *func_decl,
                                                      types::Scope *scope) {
    types::Scope *func_scope = info_builder_.CreateScope(func_decl, scope);
    
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

void IdentifierResolver::ResolveIdentifiersInTypeParamList(ast::TypeParamList *type_param_list,
                                                           types::Scope *scope) {
    for (ast::TypeParam *type_param : type_param_list->params()) {
        if (type_param->type() == nullptr) {
            continue;
        }
        ResolveIdentifiersInExpr(type_param->type(), scope);
    }
    for (ast::TypeParam *type_param : type_param_list->params()) {
        if (type_param->name()->name() == "_") {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            type_param->name()->start(),
                                            "blank type parameter name not allowed"));
            continue;
        }
        
        types::TypeName *type_name =
            info_builder_.CreateTypeNameForTypeParameter(scope,
                                                         package_,
                                                         type_param->name()->start(),
                                                         type_param->name()->name());
        info_builder_.SetDefinedObject(type_param->name(), type_name);
        AddObjectToScope(scope, type_name);
    }
}

void IdentifierResolver::ResolveIdentifiersInExprReceiver(ast::ExprReceiver *expr_receiver,
                                                          types::Scope *scope) {
    ResolveIdentifier(expr_receiver->type_name(), scope);
    
    if (!expr_receiver->type_parameter_names().empty()) {
        for (ast::Ident *type_param_name : expr_receiver->type_parameter_names()) {
            types::TypeName *type_name =
                info_builder_.CreateTypeNameForTypeParameter(scope,
                                                             package_,
                                                             type_param_name->start(),
                                                             type_param_name->name());
            info_builder_.SetDefinedObject(type_param_name, type_name);
            AddObjectToScope(scope, type_name);
        }
    }
    
    if (expr_receiver->name() != nullptr) {
        types::Variable *variable = info_builder_.CreateVariable(scope,
                                                                 package_,
                                                                 expr_receiver->name()->start(),
                                                                 expr_receiver->name()->name(),
                                                                 /* is_embedded= */ false,
                                                                 /* is_field= */ false);
        info_builder_.SetDefinedObject(expr_receiver->name(), variable);
        AddObjectToScope(scope, variable);
    } else {
        types::Variable *variable = info_builder_.CreateVariable(scope,
                                                                 package_,
                                                                 expr_receiver->start(),
                                                                 "",
                                                                 /* is_embedded= */ false,
                                                                 /* is_field= */ false);
        info_builder_.SetImplicitObject(expr_receiver, variable);
    }
}

void IdentifierResolver::ResolveIdentifiersInTypeReceiver(ast::TypeReceiver *type_receiver,
                                                          types::Scope *scope) {
    ResolveIdentifier(type_receiver->type_name(), scope);
    
    if (!type_receiver->type_parameter_names().empty()) {
        for (ast::Ident *type_param_name : type_receiver->type_parameter_names()) {
            types::TypeName *type_name =
            info_builder_.CreateTypeNameForTypeParameter(scope,
                                                         package_,
                                                         type_param_name->start(),
                                                         type_param_name->name());
            info_builder_.SetDefinedObject(type_param_name, type_name);
            AddObjectToScope(scope, type_name);
        }
    }
}

void IdentifierResolver::ResolveIdentifiersInRegularFuncFieldList(ast::FieldList *field_list,
                                                                  types::Scope *scope) {
    for (ast::Field *field : field_list->fields()) {
        ResolveIdentifiersInExpr(field->type(), scope);
    }
    for (ast::Field *field : field_list->fields()) {
        for (ast::Ident *name : field->names()) {
            types::Variable *variable = info_builder_.CreateVariable(scope,
                                                                     package_,
                                                                     name->start(),
                                                                     name->name(),
                                                                     /* is_embedded= */ false,
                                                                     /* is_field= */ false);
            info_builder_.SetDefinedObject(name, variable);
            AddObjectToScope(scope, variable);
        }
        if (field->names().empty()) {
            types::Variable *variable = info_builder_.CreateVariable(scope,
                                                                     package_,
                                                                     field->type()->start(),
                                                                     /* name= */ "",
                                                                     /* is_embedded= */ false,
                                                                     /* is_field= */ false);
            info_builder_.SetImplicitObject(field, variable);
            AddObjectToScope(scope, variable);
        }
    }
}

void IdentifierResolver::ResolveIdentifiersInStmt(ast::Stmt *stmt, types::Scope *scope) {
    if (ast::BlockStmt *block_stmt = dynamic_cast<ast::BlockStmt *>(stmt)) {
        ResolveIdentifiersInBlockStmt(block_stmt, scope);
    } else if (ast::DeclStmt *decl_stmt = dynamic_cast<ast::DeclStmt *>(stmt)) {
        ResolveIdentifiersInDeclStmt(decl_stmt, scope);
    } else if (ast::AssignStmt *assign_stmt = dynamic_cast<ast::AssignStmt *>(stmt)) {
        ResolveIdentifiersInAssignStmt(assign_stmt, scope);
    } else if (ast::ExprStmt *expr_stmt = dynamic_cast<ast::ExprStmt *>(stmt)) {
        ResolveIdentifiersInExpr(expr_stmt->x(), scope);
    } else if (ast::IncDecStmt *inc_dec_stmt = dynamic_cast<ast::IncDecStmt *>(stmt)) {
        ResolveIdentifiersInExpr(inc_dec_stmt->x(), scope);
    } else if (ast::ReturnStmt *return_stmt = dynamic_cast<ast::ReturnStmt *>(stmt)) {
        for (ast::Expr *expr : return_stmt->results()) {
            ResolveIdentifiersInExpr(expr, scope);
        }
    } else if (ast::IfStmt *if_stmt = dynamic_cast<ast::IfStmt *>(stmt)) {
        ResolveIdentifiersInIfStmt(if_stmt, scope);
    } else if (ast::ExprSwitchStmt *expr_switch_stmt = dynamic_cast<ast::ExprSwitchStmt *>(stmt)) {
        ResolveIdentifiersInExprSwitchStmt(expr_switch_stmt, scope);
    } else if (ast::TypeSwitchStmt *type_switch_stmt = dynamic_cast<ast::TypeSwitchStmt *>(stmt)) {
        ResolveIdentifiersInTypeSwitchStmt(type_switch_stmt, scope);
    } else if (ast::ForStmt *for_stmt = dynamic_cast<ast::ForStmt *>(stmt)) {
        ResolveIdentifiersInForStmt(for_stmt, scope);
    } else if (ast::LabeledStmt *labeled_stmt = dynamic_cast<ast::LabeledStmt *>(stmt)) {
        ResolveIdentifiersInStmt(labeled_stmt->stmt(), scope);
    } else if (ast::BranchStmt *branch_stmt = dynamic_cast<ast::BranchStmt *>(stmt)) {
        ResolveIdentifiersInBranchStmt(branch_stmt, scope);
    } else {
        throw "unexpected AST stmt";
    }
}

void IdentifierResolver::ResolveIdentifiersInBlockStmt(ast::BlockStmt *body, types::Scope *scope) {
    for (ast::Stmt *stmt : body->stmts()) {
        ast::LabeledStmt *labeled_stmt = dynamic_cast<ast::LabeledStmt *>(stmt);
        if (labeled_stmt == nullptr) {
            continue;
        }
        
        types::Label *label = info_builder_.CreateLabel(scope,
                                                        package_,
                                                        labeled_stmt->start(),
                                                        labeled_stmt->label()->name());
        info_builder_.SetDefinedObject(labeled_stmt->label(), label);
        AddObjectToScope(scope, label);
    }
    for (ast::Stmt *stmt : body->stmts()) {
        ResolveIdentifiersInStmt(stmt, scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInDeclStmt(ast::DeclStmt *decl_stmt,
                                                      types::Scope *scope) {
    switch (decl_stmt->decl()->tok()) {
        case tokens::kConst:
            for (ast::Spec *spec : decl_stmt->decl()->specs()) {
                ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec);
                ResolveIdentifiersInValueSpec(value_spec, scope);
                AddDefinedObjectsFromConstSpec(value_spec, scope);
            }
            return;
        case tokens::kVar:
            for (ast::Spec *spec : decl_stmt->decl()->specs()) {
                ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec);
                ResolveIdentifiersInValueSpec(value_spec, scope);
                AddDefinedObjectsFromVarSpec(value_spec, scope);
            }
            return;
        case tokens::kType:
            for (ast::Spec *spec : decl_stmt->decl()->specs()) {
                ast::TypeSpec *type_spec = static_cast<ast::TypeSpec *>(spec);
                AddDefinedObjectFromTypeSpec(type_spec, scope);
                ResolveIdentifiersInTypeSpec(type_spec, scope);
            }
            return;
        default:
            throw "unexpected gen decl token";
    }
}

void IdentifierResolver::ResolveIdentifiersInAssignStmt(ast::AssignStmt *assign_stmt,
                                                        types::Scope *scope) {
    for (ast::Expr *expr : assign_stmt->rhs()) {
        ResolveIdentifiersInExpr(expr, scope);
    }
    for (ast::Expr *expr : assign_stmt->lhs()) {
        if (assign_stmt->tok() == tokens::kDefine) {
            ast::Ident *ident = dynamic_cast<ast::Ident *>(expr);
            if (ident != nullptr) {
                const types::Scope *defining_scope = nullptr;
                scope->Lookup(ident->name(), defining_scope);
                if (defining_scope != scope) {
                    types::Variable *variable =
                        info_builder_.CreateVariable(scope,
                                                     package_,
                                                     ident->start(),
                                                     ident->name(),
                                                     /* is_embedded= */ false,
                                                     /* is_field= */ false);
                    info_builder_.SetDefinedObject(ident, variable);
                    AddObjectToScope(scope, variable);
                }
            }
        }
        ResolveIdentifiersInExpr(expr, scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInIfStmt(ast::IfStmt *if_stmt, types::Scope *scope) {
    types::Scope *if_scope = info_builder_.CreateScope(if_stmt, scope);
    
    if (if_stmt->init_stmt()) {
        ResolveIdentifiersInStmt(if_stmt->init_stmt(), if_scope);
    }
    ResolveIdentifiersInExpr(if_stmt->cond_expr(), if_scope);
    ResolveIdentifiersInBlockStmt(if_stmt->body(), if_scope);
    if (if_stmt->else_stmt()) {
        ResolveIdentifiersInStmt(if_stmt->else_stmt(), scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInExprSwitchStmt(ast::ExprSwitchStmt *switch_stmt,
                                                            types::Scope *scope) {
    types::Scope *switch_scope = info_builder_.CreateScope(switch_stmt, scope);
    
    if (switch_stmt->init_stmt()) {
        ResolveIdentifiersInStmt(switch_stmt->init_stmt(), switch_scope);
    }
    if (switch_stmt->tag_expr()) {
        ResolveIdentifiersInExpr(switch_stmt->tag_expr(), switch_scope);
    }
    for (ast::Stmt *stmt : switch_stmt->body()->stmts()) {
        ast::CaseClause *case_clause = static_cast<ast::CaseClause *>(stmt);
        ResolveIdentifiersInCaseClause(case_clause, switch_scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInTypeSwitchStmt(ast::TypeSwitchStmt *switch_stmt,
                                                            types::Scope *scope) {
    types::Scope *switch_scope = info_builder_.CreateScope(switch_stmt, scope);
    
    ResolveIdentifiersInExpr(switch_stmt->tag_expr(), scope);
    for (ast::Stmt *stmt : switch_stmt->body()->stmts()) {
        ast::CaseClause *case_clause = static_cast<ast::CaseClause *>(stmt);
        ResolveIdentifiersInCaseClause(case_clause, switch_scope, switch_stmt->var());
    }
}

void IdentifierResolver::ResolveIdentifiersInCaseClause(ast::CaseClause *case_clause,
                                                        types::Scope *scope,
                                                        ast::Ident *type_switch_var) {
    types::Scope *case_scope = info_builder_.CreateScope(case_clause, scope);
        
    for (ast::Expr *expr : case_clause->cond_vals()) {
        ResolveIdentifiersInExpr(expr, case_scope);
    }
    if (type_switch_var != nullptr) {
        types::Variable *variable = info_builder_.CreateVariable(case_scope,
                                                                 package_,
                                                                 type_switch_var->start(),
                                                                 type_switch_var->name(),
                                                                 /* is_embedded= */ false,
                                                                 /* is_field= */ false);
        info_builder_.SetImplicitObject(case_clause, variable);
        AddObjectToScope(case_scope, variable);
    }
    for (ast::Stmt *stmt : case_clause->body()) {
        ast::LabeledStmt *labeled_stmt = dynamic_cast<ast::LabeledStmt *>(stmt);
        if (labeled_stmt == nullptr) {
            continue;
        }
        
        types::Label *label = info_builder_.CreateLabel(case_scope,
                                                        package_,
                                                        labeled_stmt->start(),
                                                        labeled_stmt->label()->name());
        info_builder_.SetDefinedObject(labeled_stmt->label(), label);
        AddObjectToScope(case_scope, label);
    }
    for (ast::Stmt *stmt : case_clause->body()) {
        ResolveIdentifiersInStmt(stmt, case_scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInForStmt(ast::ForStmt *for_stmt, types::Scope *scope) {
    types::Scope *for_scope = info_builder_.CreateScope(for_stmt, scope);
    
    if (for_stmt->init_stmt()) {
        ResolveIdentifiersInStmt(for_stmt->init_stmt(), for_scope);
    }
    if (for_stmt->cond_expr()) {
        ResolveIdentifiersInExpr(for_stmt->cond_expr(), for_scope);
    }
    if (for_stmt->post_stmt()) {
        if (ast::AssignStmt *assign_stmt = dynamic_cast<ast::AssignStmt *>(for_stmt->post_stmt())) {
            if (assign_stmt->tok() == tokens::kDefine) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                assign_stmt->start(),
                                                "post statements of for loops can not define "
                                                "variables"));
            }
        }
        ResolveIdentifiersInStmt(for_stmt->post_stmt(), for_scope);
    }
    ResolveIdentifiersInBlockStmt(for_stmt->body(), for_scope);
}

void IdentifierResolver::ResolveIdentifiersInBranchStmt(ast::BranchStmt *branch_stmt,
                                                        types::Scope *scope) {
    if (!branch_stmt->label()) {
        return;
    }
    const types::Scope* defining_scope;
    types::Object *obj = scope->Lookup(branch_stmt->label()->name(), defining_scope);
    if (dynamic_cast<types::Label *>(obj) == nullptr) {
        issues_.push_back(
                          issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        branch_stmt->label()->start(),
                                        "branch statement does not refer to known label"));
        return;
    }
    ResolveIdentifiersInExpr(branch_stmt->label(), scope);
}

void IdentifierResolver::ResolveIdentifiersInExpr(ast::Expr *expr, types::Scope *scope) {
    if (ast::UnaryExpr *unary_expr = dynamic_cast<ast::UnaryExpr *>(expr)) {
        ResolveIdentifiersInExpr(unary_expr->x(), scope);
    } else if (ast::BinaryExpr *binary_expr = dynamic_cast<ast::BinaryExpr *>(expr)) {
        ResolveIdentifiersInExpr(binary_expr->x(), scope);
        ResolveIdentifiersInExpr(binary_expr->y(), scope);
    } else if (ast::CompareExpr *compare_expr = dynamic_cast<ast::CompareExpr *>(expr)) {
        for (ast::Expr *operand : compare_expr->operands()) {
            ResolveIdentifiersInExpr(operand, scope);
        }
    } else if (ast::ParenExpr *paren_expr = dynamic_cast<ast::ParenExpr *>(expr)) {
        ResolveIdentifiersInExpr(paren_expr->x(), scope);
    } else if (ast::SelectionExpr *selection_expr = dynamic_cast<ast::SelectionExpr *>(expr)) {
        ResolveIdentifiersInSelectionExpr(selection_expr, scope);
    } else if (ast::TypeAssertExpr *type_assert_expr = dynamic_cast<ast::TypeAssertExpr *>(expr)) {
        ResolveIdentifiersInExpr(type_assert_expr->x(), scope);
        if (type_assert_expr->type()) {
            ResolveIdentifiersInExpr(type_assert_expr->type(), scope);
        }
    } else if (ast::IndexExpr *index_expr = dynamic_cast<ast::IndexExpr *>(expr)) {
        ResolveIdentifiersInExpr(index_expr->accessed(), scope);
        ResolveIdentifiersInExpr(index_expr->index(), scope);
    } else if (ast::CallExpr *call_expr = dynamic_cast<ast::CallExpr *>(expr)) {
        ResolveIdentifiersInExpr(call_expr->func(), scope);
        for (ast::Expr *type_arg : call_expr->type_args()) {
            ResolveIdentifiersInExpr(type_arg, scope);
        }
        for (ast::Expr *arg : call_expr->args()) {
            ResolveIdentifiersInExpr(arg, scope);
        }
    } else if (ast::FuncLit *func_lit = dynamic_cast<ast::FuncLit *>(expr)) {
        ResolveIdentifiersInFuncLit(func_lit, scope);
    } else if (ast::CompositeLit *composite_lit = dynamic_cast<ast::CompositeLit *>(expr)) {
        ResolveIdentifiersInCompositeLit(composite_lit, scope);
    } else if (ast::ArrayType *array_type = dynamic_cast<ast::ArrayType *>(expr)) {
        if (array_type->len()) {
            ResolveIdentifiersInExpr(array_type->len(), scope);
        }
        ResolveIdentifiersInExpr(array_type->element_type(), scope);
    } else if (ast::FuncType *func_type = dynamic_cast<ast::FuncType *>(expr)) {
        ResolveIdentifiersInFuncType(func_type, scope);
    } else if (ast::InterfaceType *interface_type = dynamic_cast<ast::InterfaceType *>(expr)) {
        ResolveIdentifiersInInterfaceType(interface_type, scope);
    } else if (ast::StructType *struct_type = dynamic_cast<ast::StructType *>(expr)) {
        ResolveIdentifiersInStructType(struct_type, scope);
    } else if (ast::TypeInstance *type_instance = dynamic_cast<ast::TypeInstance *>(expr)) {
        ResolveIdentifiersInExpr(type_instance->type(), scope);
        for (ast::Expr *type_arg : type_instance->type_args()) {
            ResolveIdentifiersInExpr(type_arg, scope);
        }
    } else if (ast::BasicLit *basic_lit = dynamic_cast<ast::BasicLit *>(expr)) {
        return;
    } else if (ast::Ident *ident = dynamic_cast<ast::Ident *>(expr)) {
        ResolveIdentifier(ident, scope);
    } else {
        throw "unexpected AST expr";
    }
}

void IdentifierResolver::ResolveIdentifiersInSelectionExpr(ast::SelectionExpr *sel,
                                                           types::Scope *scope) {
    ResolveIdentifiersInExpr(sel->accessed(), scope);
    if (sel->selection()->name() == "_") {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        sel->selection()->start(),
                                        "can not select underscore"));
        return;
    }
    
    ast::Ident *accessed_ident = dynamic_cast<ast::Ident *>(sel->accessed());
    if (accessed_ident == nullptr) {
        return;
    }
    auto it = info_->uses().find(accessed_ident);
    if (it == info_->uses().end()) {
        return;
    }
    types::Object *accessed_object = it->second;
    types::PackageName *pkg_name = dynamic_cast<types::PackageName *>(accessed_object);
    if (pkg_name == nullptr || pkg_name->referenced_package() == nullptr) {
        return;
    }
    
    ast::Ident *selected_ident = sel->selection();
    ResolveIdentifier(selected_ident,
                      pkg_name->referenced_package()->scope());
}

void IdentifierResolver::ResolveIdentifiersInFuncLit(ast::FuncLit *func_lit, types::Scope *scope) {
    types::Func *func = info_builder_.CreateFunc(scope,
                                                 package_,
                                                 func_lit->start(),
                                                 /* name= */ "");
    info_builder_.SetImplicitObject(func_lit, func);
    AddObjectToScope(scope, func);
    
    types::Scope *func_scope = info_builder_.CreateScope(func_lit, scope);
    
    ResolveIdentifiersInRegularFuncFieldList(func_lit->type()->params(), func_scope);
    if (func_lit->type()->results()) {
        ResolveIdentifiersInRegularFuncFieldList(func_lit->type()->results(), func_scope);
    }
    ResolveIdentifiersInBlockStmt(func_lit->body(), func_scope);
}

void IdentifierResolver::ResolveIdentifiersInCompositeLit(ast::CompositeLit *composite_lit,
                                                   types::Scope *scope) {
    ResolveIdentifiersInExpr(composite_lit->type(), scope);
    for (ast::Expr *value : composite_lit->values()) {
        ast::Expr *expr = value;
        if (ast::KeyValueExpr *key_value_expr = dynamic_cast<ast::KeyValueExpr *>(value)) {
            expr = key_value_expr->value();
        }
        ResolveIdentifiersInExpr(expr, scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInFuncType(ast::FuncType *func_type,
                                                      types::Scope *scope) {
    types::Scope *func_scope = info_builder_.CreateScope(func_type, scope);
    
    ResolveIdentifiersInRegularFuncFieldList(func_type->params(), func_scope);
    if (func_type->results()) {
        ResolveIdentifiersInRegularFuncFieldList(func_type->results(), func_scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInInterfaceType(ast::InterfaceType *interface_type,
                                                           types::Scope *scope) {
    types::Scope *interface_scope = info_builder_.CreateScope(interface_type, scope);
    
    for (ast::MethodSpec *method_spec : interface_type->methods()) {
        types::Scope *method_scope = info_builder_.CreateScope(method_spec, interface_scope);
        
        if (method_spec->instance_type_param() != nullptr) {
            ast::Ident *name = method_spec->instance_type_param();
            types::TypeName *instance_type_param = info_builder_.CreateTypeNameForTypeParameter(method_scope,
                                                             package_,
                                                             name->start(),
                                                             name->name());
            info_builder_.SetDefinedObject(name, instance_type_param);
            AddObjectToScope(method_scope, instance_type_param);
        }
        
        ResolveIdentifiersInRegularFuncFieldList(method_spec->params(),
                                                 method_scope);
        if (method_spec->results()) {
            ResolveIdentifiersInRegularFuncFieldList(method_spec->results(),
                                                     method_scope);
        }
    }
    for (ast::MethodSpec *method_spec : interface_type->methods()) {
        types::Func *method = info_builder_.CreateFunc(interface_scope,
                                                       package_,
                                                       method_spec->start(),
                                                       method_spec->name()->name());
        info_builder_.SetDefinedObject(method_spec->name(), method);
        AddObjectToScope(interface_scope, method);
    }
}

void IdentifierResolver::ResolveIdentifiersInStructType(ast::StructType *struct_type,
                                                        types::Scope *scope) {
    types::Scope *struct_scope = info_builder_.CreateScope(struct_type, scope);
    
    for (ast::Field *field : struct_type->fields()->fields()) {
        ResolveIdentifiersInExpr(field->type(), scope);
    }
    for (ast::Field *field : struct_type->fields()->fields()) {
        if (field->names().empty()) {
            ast::Expr *type = field->type();
            if (ast::UnaryExpr *ptr_type = dynamic_cast<ast::UnaryExpr *>(type)) {
                if (ptr_type->op() != tokens::kMul &&
                    ptr_type->op() != tokens::kRem) {
                    issues_.push_back(
                                      issues::Issue(issues::Origin::TypeChecker,
                                                    issues::Severity::Error,
                                                    type->start(),
                                                    "expected embedded field to be defined type or "
                                                    "pointer to defined type"));
                    continue;
                }
                type = ptr_type->x();
            }
            if (ast::TypeInstance *type_instance = dynamic_cast<ast::TypeInstance *>(type)) {
                type = type_instance->type();
            }
            ast::Ident *defined_type = dynamic_cast<ast::Ident *>(type);
            if (defined_type == nullptr) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                type->start(),
                                                "expected embdedded field to be defined type or "
                                                "pointer to defined type"));
                continue;
            }
            
            types::Variable *variable = info_builder_.CreateVariable(struct_scope,
                                                                     package_,
                                                                     field->type()->start(),
                                                                     defined_type->name(),
                                                                     /* is_embedded= */ true,
                                                                     /* is_field= */ true);
            info_builder_.SetImplicitObject(field, variable);
            AddObjectToScope(struct_scope, variable);
        } else {
            for (ast::Ident *name : field->names()) {
                types::Variable *variable = info_builder_.CreateVariable(struct_scope,
                                                                         package_,
                                                                         name->start(),
                                                                         name->name(),
                                                                         /* is_embedded= */ false,
                                                                         /* is_field= */ true);
                info_builder_.SetDefinedObject(name, variable);
                AddObjectToScope(struct_scope, variable);
            }
        }
    }
}

void IdentifierResolver::ResolveIdentifier(ast::Ident *ident,
                                           types::Scope *scope) {
    if (ident->name() == "_") {
        return;
    }
    types::Object *object = scope->Lookup(ident->name());
    if (object == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Fatal,
                                        ident->start(),
                                        "could not resolve identifier: " + ident->name()));
    }
    info_builder_.SetUsedObject(ident, object);
}

}
}
