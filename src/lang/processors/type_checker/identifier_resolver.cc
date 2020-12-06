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
        for (auto& decl : file->decls_) {
            if (auto gen_decl = dynamic_cast<ast::GenDecl *>(decl.get())) {
                AddDefinedObjectsFromGenDecl(gen_decl, package_->scope(), file);
            } else if (auto func_decl = dynamic_cast<ast::FuncDecl *>(decl.get())) {
                AddDefinedObjectFromFuncDecl(func_decl, package_->scope());
            } else {
                throw "unexpected declaration";
            }
        }
    }
    
    for (auto file : package_files_) {
        types::Scope *file_scope = info_->scopes().at(file);
        
        for (auto& decl : file->decls_) {
            if (auto gen_decl = dynamic_cast<ast::GenDecl *>(decl.get())) {
                ResolveIdentifiersInGenDecl(gen_decl, file_scope);
            } else if (auto func_decl = dynamic_cast<ast::FuncDecl *>(decl.get())) {
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
    switch (gen_decl->tok_) {
        case tokens::kImport:
            for (auto& spec : gen_decl->specs_) {
                AddDefinedObjectsFromImportSpec(static_cast<ast::ImportSpec *>(spec.get()), file);
            }
            return;
        case tokens::kConst:
            for (auto& spec : gen_decl->specs_) {
                AddDefinedObjectsFromConstSpec(static_cast<ast::ValueSpec *>(spec.get()),
                                               scope);
            }
            return;
        case tokens::kVar:
            for (auto& spec : gen_decl->specs_) {
                AddDefinedObjectsFromVarSpec(static_cast<ast::ValueSpec *>(spec.get()),
                                             scope);
            }
            return;
        case tokens::kType:
            for (auto& spec : gen_decl->specs_) {
                AddDefinedObjectFromTypeSpec(static_cast<ast::TypeSpec *>(spec.get()),
                                             scope);
            }
            return;
        default:
            throw "unexpected gen decl token";
    }
}

void IdentifierResolver::AddDefinedObjectsFromImportSpec(ast::ImportSpec *import_spec,
                                                         ast::File *file) {
    std::string name;
    std::string path = import_spec->path_->value_;
    path = path.substr(1, path.length()-2);
    
    // Create import set for file if not present yet.
    file_imports_[file];
    
    if (file_imports_.at(file).find(path) != file_imports_.at(file).end()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        import_spec->path_->start(),
                                        "can not import package twice: \"" + path + "\""));
        return;
    }
    types::Package *referenced_package = importer_(path);
    if (referenced_package == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        import_spec->path_->start(),
                                        "could not import package: \"" + path + "\""));
    }
    file_imports_.at(file).insert(path);
    
    info_builder_.AddImportToPackage(package_, referenced_package);
        
    if (import_spec->name_) {
        if (import_spec->name_->name_ == "_") {
            return;
        }
        name = import_spec->name_->name_;
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
    if (import_spec->name_) {
        info_builder_.SetDefinedObject(import_spec->name_.get(), package_name);
    } else {
        info_builder_.SetImplicitObject(import_spec, package_name);
    }
    AddObjectToScope(file_scope, package_name);
}

void IdentifierResolver::AddDefinedObjectsFromConstSpec(ast::ValueSpec *value_spec,
                                                        types::Scope *scope) {
    for (auto& ident : value_spec->names_) {
        if (ident->name_ == "_") {
            continue;
        }
        
        types::Constant *constant = info_builder_.CreateConstant(scope,
                                                                 package_,
                                                                 ident->start(),
                                                                 ident->name_);
        info_builder_.SetDefinedObject(ident.get(), constant);
        AddObjectToScope(scope, constant);
    }
}

void IdentifierResolver::AddDefinedObjectsFromVarSpec(ast::ValueSpec *value_spec,
                                                      types::Scope *scope) {
    for (auto& ident : value_spec->names_) {
        if (ident->name_ == "_") {
            continue;
        }
        
        types::Variable *variable = info_builder_.CreateVariable(scope,
                                                                 package_,
                                                                 ident->start(),
                                                                 ident->name_,
                                                                 /* is_embedded= */ false,
                                                                 /* is_field= */ false);
        info_builder_.SetDefinedObject(ident.get(), variable);
        AddObjectToScope(scope, variable);
    }
}

void IdentifierResolver::AddDefinedObjectFromTypeSpec(ast::TypeSpec *type_spec,
                                                      types::Scope *scope) {
    if (type_spec->name_->name_ == "_") {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_spec->name_->start(),
                                        "blank type name not allowed"));
        return;
    }
    
    bool is_alias = type_spec->assign_ != pos::kNoPos;
    types::TypeName *type_name = info_builder_.CreateTypeNameForNamedType(scope,
                                                                          package_,
                                                                          type_spec->name_->start(),
                                                                          type_spec->name_->name_,
                                                                          is_alias);
    info_builder_.SetDefinedObject(type_spec->name_.get(), type_name);
    AddObjectToScope(scope, type_name);
}

void IdentifierResolver::AddDefinedObjectFromFuncDecl(ast::FuncDecl *func_decl,
                                                      types::Scope *scope) {
    if (func_decl->name_->name_ == "_") {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        func_decl->name_->start(),
                                        "blank func name not allowed"));
        return;
    }
    
    types::Func *func = info_builder_.CreateFunc(scope,
                                                 package_,
                                                 func_decl->name_->start(),
                                                 func_decl->name_->name_);
    info_builder_.SetDefinedObject(func_decl->name_.get(), func);
    if (!func_decl->receiver_) {
        AddObjectToScope(scope, func);
    }
}

void IdentifierResolver::ResolveIdentifiersInGenDecl(ast::GenDecl *gen_decl, types::Scope *scope) {
    switch (gen_decl->tok_) {
        case tokens::kImport:
            return;
        case tokens::kConst:
        case tokens::kVar:
            for (auto& spec : gen_decl->specs_) {
                ResolveIdentifiersInValueSpec(static_cast<ast::ValueSpec *>(spec.get()),
                                              scope);
            }
            return;
        case tokens::kType:
            for (auto& spec : gen_decl->specs_) {
                ResolveIdentifiersInTypeSpec(static_cast<ast::TypeSpec *>(spec.get()),
                                             scope);
            }
            return;
        default:
            throw "unexpected gen decl token";
    }
}

void IdentifierResolver::ResolveIdentifiersInValueSpec(ast::ValueSpec *value_spec,
                                                       types::Scope *scope) {
    if (value_spec->type_) {
        ResolveIdentifiersInExpr(value_spec->type_.get(), scope);
    }
    for (auto& value : value_spec->values_) {
        ResolveIdentifiersInExpr(value.get(), scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInTypeSpec(ast::TypeSpec *type_spec,
                                                      types::Scope *scope) {
    types::Scope *type_scope = info_builder_.CreateScope(type_spec, scope);
    
    if (type_spec->type_params_) {
        ResolveIdentifiersInTypeParamList(type_spec->type_params_.get(), type_scope);
    }
    ResolveIdentifiersInExpr(type_spec->type_.get(), type_scope);
}

void IdentifierResolver::ResolveIdentifiersInFuncDecl(ast::FuncDecl *func_decl,
                                                      types::Scope *scope) {
    types::Scope *func_scope = info_builder_.CreateScope(func_decl, scope);
    
    if (func_decl->receiver_) {
        ResolveIdentifiersInFuncReceiverFieldList(func_decl->receiver_.get(), func_scope);
    }
    if (func_decl->type_params_) {
        ResolveIdentifiersInTypeParamList(func_decl->type_params_.get(), func_scope);
    }
    ResolveIdentifiersInRegularFuncFieldList(func_decl->type_->params_.get(), func_scope);
    if (func_decl->type_->results_) {
        ResolveIdentifiersInRegularFuncFieldList(func_decl->type_->results_.get(), func_scope);
    }
    if (func_decl->body_) {
        ResolveIdentifiersInBlockStmt(func_decl->body_.get(), func_scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInTypeParamList(ast::TypeParamList *type_param_list,
                                                    types::Scope *scope) {
    for (auto& type_param : type_param_list->params_) {
        if (!type_param->type_) {
            continue;
        }
        ResolveIdentifiersInExpr(type_param->type_.get(), scope);
    }
    for (auto& type_param : type_param_list->params_) {
        if (type_param->name_->name_ == "_") {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            type_param->name_->start(),
                                            "blank type parameter name not allowed"));
            continue;
        }
        
        types::TypeName *type_name =
            info_builder_.CreateTypeNameForTypeParameter(scope,
                                                         package_,
                                                         type_param->name_->start(),
                                                         type_param->name_->name_);
        info_builder_.SetDefinedObject(type_param->name_.get(), type_name);
        AddObjectToScope(scope, type_name);
    }
}

void IdentifierResolver::ResolveIdentifiersInFuncReceiverFieldList(ast::FieldList *field_list,
                                                                   types::Scope *scope) {
    if (field_list->fields_.size() != 1 ||
        (field_list->fields_.size() > 0 &&
         field_list->fields_.at(0)->names_.size() > 1)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        field_list->start(),
                                        "expected one receiver"));
        if (field_list->fields_.size() == 0) {
            return;
        }
    }
    ast::Field *field = field_list->fields_.at(0).get();
    ast::Expr *type = field->type_.get();
    if (auto ptr_type = dynamic_cast<ast::UnaryExpr *>(type)) {
        if (ptr_type->op_ != tokens::kMul &&
            ptr_type->op_ != tokens::kRem) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            type->start(),
                                            "expected receiver of defined type or pointer to "
                                            "defined type"));
        }
        type = ptr_type->x_.get();
    }
    ast::TypeArgList *type_args = nullptr;
    if (auto type_instance = dynamic_cast<ast::TypeInstance *>(type)) {
        type_args = type_args = type_instance->type_args_.get();
        type = type_instance->type_.get();
    }
    ast::Ident *defined_type = dynamic_cast<ast::Ident *>(type);
    if (defined_type == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type->start(),
                                        "expected receiver of defined type or pointer to "
                                        "defined type"));
    } else {
        ResolveIdentifier(defined_type, scope);
    }
    
    if (type_args) {
        for (auto& type_arg : type_args->args_) {
            ast::Ident *ident = dynamic_cast<ast::Ident*>(type_arg.get());
            if (ident == nullptr) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                type->start(),
                                                "expected type name definition as type argument to "
                                                "receiver type"));
                continue;
            }
            
            types::TypeName *type_name =
                info_builder_.CreateTypeNameForTypeParameter(scope,
                                                             package_,
                                                             ident->start(),
                                                             ident->name_);
            info_builder_.SetDefinedObject(ident, type_name);
            AddObjectToScope(scope, type_name);
        }
    }
    
    if (field->names_.size() == 0 ||
        field->names_.at(0)->name_ == "_") {
        return;
    }
    
    types::Variable *variable = info_builder_.CreateVariable(scope,
                                                             package_,
                                                             field->names_.at(0)->start(),
                                                             field->names_.at(0)->name_,
                                                             /* is_embedded= */ false,
                                                             /* is_field= */ false);
    info_builder_.SetDefinedObject(field->names_.at(0).get(), variable);
    AddObjectToScope(scope, variable);
}

void IdentifierResolver::ResolveIdentifiersInRegularFuncFieldList(ast::FieldList *field_list,
                                                           types::Scope *scope) {
    for (auto& field : field_list->fields_) {
        ResolveIdentifiersInExpr(field->type_.get(), scope);
    }
    for (auto& field : field_list->fields_) {
        for (auto& name : field->names_) {
            types::Variable *variable = info_builder_.CreateVariable(scope,
                                                                     package_,
                                                                     name->start(),
                                                                     name->name_,
                                                                     /* is_embedded= */ false,
                                                                     /* is_field= */ false);
            info_builder_.SetDefinedObject(name.get(), variable);
            AddObjectToScope(scope, variable);
        }
        if (field->names_.empty()) {
            types::Variable *variable = info_builder_.CreateVariable(scope,
                                                                     package_,
                                                                     field->type_->start(),
                                                                     /* name= */ "",
                                                                     /* is_embedded= */ false,
                                                                     /* is_field= */ false);
            info_builder_.SetImplicitObject(field.get(), variable);
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
        ResolveIdentifiersInExpr(expr_stmt->x_.get(), scope);
    } else if (ast::IncDecStmt *inc_dec_stmt = dynamic_cast<ast::IncDecStmt *>(stmt)) {
        ResolveIdentifiersInExpr(inc_dec_stmt->x_.get(), scope);
    } else if (ast::ReturnStmt *return_stmt = dynamic_cast<ast::ReturnStmt *>(stmt)) {
        for (auto& expr : return_stmt->results_) {
            ResolveIdentifiersInExpr(expr.get(), scope);
        }
    } else if (ast::IfStmt *if_stmt = dynamic_cast<ast::IfStmt *>(stmt)) {
        ResolveIdentifiersInIfStmt(if_stmt, scope);
    } else if (ast::SwitchStmt *switch_stmt = dynamic_cast<ast::SwitchStmt *>(stmt)) {
        ResolveIdentifiersInSwitchStmt(switch_stmt, scope);
    } else if (ast::ForStmt *for_stmt = dynamic_cast<ast::ForStmt *>(stmt)) {
        ResolveIdentifiersInForStmt(for_stmt, scope);
    } else if (ast::LabeledStmt *labeled_stmt = dynamic_cast<ast::LabeledStmt *>(stmt)) {
        ResolveIdentifiersInStmt(labeled_stmt->stmt_.get(), scope);
    } else if (ast::BranchStmt *branch_stmt = dynamic_cast<ast::BranchStmt *>(stmt)) {
        ResolveIdentifiersInBranchStmt(branch_stmt, scope);
    } else {
        throw "unexpected AST stmt";
    }
}

void IdentifierResolver::ResolveIdentifiersInBlockStmt(ast::BlockStmt *body, types::Scope *scope) {
    for (auto& stmt : body->stmts_) {
        auto labeled_stmt = dynamic_cast<ast::LabeledStmt *>(stmt.get());
        if (labeled_stmt == nullptr) {
            continue;
        }
        
        types::Label *label = info_builder_.CreateLabel(scope,
                                                        package_,
                                                        labeled_stmt->start(),
                                                        labeled_stmt->label_->name_);
        info_builder_.SetDefinedObject(labeled_stmt->label_.get(), label);
        AddObjectToScope(scope, label);
    }
    for (auto& stmt : body->stmts_) {
        ResolveIdentifiersInStmt(stmt.get(), scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInDeclStmt(ast::DeclStmt *decl_stmt,
                                                      types::Scope *scope) {
    switch (decl_stmt->decl_->tok_) {
        case tokens::kConst:
            for (auto& spec : decl_stmt->decl_->specs_) {
                auto value_spec = static_cast<ast::ValueSpec *>(spec.get());
                ResolveIdentifiersInValueSpec(value_spec, scope);
                AddDefinedObjectsFromConstSpec(value_spec, scope);
            }
            return;
        case tokens::kVar:
            for (auto& spec : decl_stmt->decl_->specs_) {
                auto value_spec = static_cast<ast::ValueSpec *>(spec.get());
                ResolveIdentifiersInValueSpec(value_spec, scope);
                AddDefinedObjectsFromVarSpec(value_spec, scope);
            }
            return;
        case tokens::kType:
            for (auto& spec : decl_stmt->decl_->specs_) {
                auto type_spec = static_cast<ast::TypeSpec *>(spec.get());
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
    for (auto& expr : assign_stmt->rhs_) {
        ResolveIdentifiersInExpr(expr.get(), scope);
    }
    for (auto& expr : assign_stmt->lhs_) {
        if (assign_stmt->tok_ == tokens::kDefine) {
            auto ident = dynamic_cast<ast::Ident *>(expr.get());
            if (ident != nullptr) {
                const types::Scope *defining_scope = nullptr;
                scope->Lookup(ident->name_, defining_scope);
                if (defining_scope != scope) {
                    types::Variable *variable =
                        info_builder_.CreateVariable(scope,
                                                     package_,
                                                     ident->start(),
                                                     ident->name_,
                                                     /* is_embedded= */ false,
                                                     /* is_field= */ false);
                    info_builder_.SetDefinedObject(ident, variable);
                    AddObjectToScope(scope, variable);
                }
            }
        }
        ResolveIdentifiersInExpr(expr.get(), scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInIfStmt(ast::IfStmt *if_stmt, types::Scope *scope) {
    types::Scope *if_scope = info_builder_.CreateScope(if_stmt, scope);
    
    if (if_stmt->init_) {
        ResolveIdentifiersInStmt(if_stmt->init_.get(), if_scope);
    }
    ResolveIdentifiersInExpr(if_stmt->cond_.get(), if_scope);
    ResolveIdentifiersInBlockStmt(if_stmt->body_.get(), if_scope);
    if (if_stmt->else_) {
        ResolveIdentifiersInStmt(if_stmt->else_.get(), scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInSwitchStmt(ast::SwitchStmt *switch_stmt,
                                                        types::Scope *scope) {
    types::Scope *switch_scope = info_builder_.CreateScope(switch_stmt, scope);
    
    if (switch_stmt->init_) {
        ResolveIdentifiersInStmt(switch_stmt->init_.get(), switch_scope);
    }
    if (switch_stmt->tag_) {
        ResolveIdentifiersInExpr(switch_stmt->tag_.get(), switch_scope);
    }
    if (ast::IsTypeSwitchStmt(switch_stmt)) {
        ast::Ident *ident = nullptr;
        if (switch_stmt->init_) {
            auto assign_stmt = static_cast<ast::AssignStmt *>(switch_stmt->init_.get());
            ident = static_cast<ast::Ident *>(assign_stmt->lhs_.at(0).get());
        }
        for (auto& stmt : switch_stmt->body_->stmts_) {
            auto case_clause = static_cast<ast::CaseClause *>(stmt.get());
            ResolveIdentifiersInCaseClause(case_clause, switch_scope, ident);
        }
    } else {
        for (auto& stmt : switch_stmt->body_->stmts_) {
            auto case_clause = static_cast<ast::CaseClause *>(stmt.get());
            ResolveIdentifiersInCaseClause(case_clause, switch_scope);
        }
    }
}

void IdentifierResolver::ResolveIdentifiersInCaseClause(ast::CaseClause *case_clause,
                                                        types::Scope *scope,
                                                        ast::Ident *type_switch_var_ident) {
    types::Scope *case_scope = info_builder_.CreateScope(case_clause, scope);
        
    for (auto& expr : case_clause->cond_vals_) {
        ResolveIdentifiersInExpr(expr.get(), case_scope);
    }
    if (type_switch_var_ident != nullptr) {
        types::Variable *variable = info_builder_.CreateVariable(case_scope,
                                                                 package_,
                                                                 type_switch_var_ident->start(),
                                                                 type_switch_var_ident->name_,
                                                                 /* is_embedded= */ false,
                                                                 /* is_field= */ false);
        info_builder_.SetImplicitObject(case_clause, variable);
        AddObjectToScope(case_scope, variable);
    }
    for (auto& stmt : case_clause->body_) {
        auto labeled_stmt = dynamic_cast<ast::LabeledStmt *>(stmt.get());
        if (labeled_stmt == nullptr) {
            continue;
        }
        
        types::Label *label = info_builder_.CreateLabel(case_scope,
                                                        package_,
                                                        labeled_stmt->start(),
                                                        labeled_stmt->label_->name_);
        info_builder_.SetDefinedObject(labeled_stmt->label_.get(), label);
        AddObjectToScope(case_scope, label);
    }
    for (auto& stmt : case_clause->body_) {
        ResolveIdentifiersInStmt(stmt.get(), case_scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInForStmt(ast::ForStmt *for_stmt, types::Scope *scope) {
    types::Scope *for_scope = info_builder_.CreateScope(for_stmt, scope);
    
    if (for_stmt->init_) {
        ResolveIdentifiersInStmt(for_stmt->init_.get(), for_scope);
    }
    if (for_stmt->cond_) {
        ResolveIdentifiersInExpr(for_stmt->cond_.get(), for_scope);
    }
    if (for_stmt->post_) {
        if (auto assign_stmt = dynamic_cast<ast::AssignStmt *>(for_stmt->post_.get())) {
            if (assign_stmt->tok_ == tokens::kDefine) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                assign_stmt->start(),
                                                "post statements of for loops can not define "
                                                "variables"));
            }
        }
        ResolveIdentifiersInStmt(for_stmt->post_.get(), for_scope);
    }
    ResolveIdentifiersInBlockStmt(for_stmt->body_.get(), for_scope);
}

void IdentifierResolver::ResolveIdentifiersInBranchStmt(ast::BranchStmt *branch_stmt,
                                                        types::Scope *scope) {
    if (!branch_stmt->label_) {
        return;
    }
    const types::Scope* defining_scope;
    types::Object *obj = scope->Lookup(branch_stmt->label_->name_, defining_scope);
    if (dynamic_cast<types::Label *>(obj) == nullptr) {
        issues_.push_back(
                          issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        branch_stmt->label_->start(),
                                        "branch statement does not refer to known label"));
        return;
    }
    ResolveIdentifiersInExpr(branch_stmt->label_.get(), scope);
}

void IdentifierResolver::ResolveIdentifiersInExpr(ast::Expr *expr, types::Scope *scope) {
    if (ast::UnaryExpr *unary_expr = dynamic_cast<ast::UnaryExpr *>(expr)) {
        ResolveIdentifiersInExpr(unary_expr->x_.get(), scope);
    } else if (ast::BinaryExpr *binary_expr = dynamic_cast<ast::BinaryExpr *>(expr)) {
        ResolveIdentifiersInExpr(binary_expr->x_.get(), scope);
        ResolveIdentifiersInExpr(binary_expr->y_.get(), scope);
    } else if (ast::ParenExpr *paren_expr = dynamic_cast<ast::ParenExpr *>(expr)) {
        ResolveIdentifiersInExpr(paren_expr->x_.get(), scope);
    } else if (ast::SelectionExpr *selection_expr = dynamic_cast<ast::SelectionExpr *>(expr)) {
        ResolveIdentifiersInSelectionExpr(selection_expr, scope);
    } else if (ast::TypeAssertExpr *type_assert_expr = dynamic_cast<ast::TypeAssertExpr *>(expr)) {
        ResolveIdentifiersInExpr(type_assert_expr->x_.get(), scope);
        if (type_assert_expr->type_) {
            ResolveIdentifiersInExpr(type_assert_expr->type_.get(), scope);
        }
    } else if (ast::IndexExpr *index_expr = dynamic_cast<ast::IndexExpr *>(expr)) {
        ResolveIdentifiersInExpr(index_expr->accessed_.get(), scope);
        ResolveIdentifiersInExpr(index_expr->index_.get(), scope);
    } else if (ast::CallExpr *call_expr = dynamic_cast<ast::CallExpr *>(expr)) {
        ResolveIdentifiersInExpr(call_expr->func_.get(), scope);
        if (call_expr->type_args_) {
            for (auto& expr : call_expr->type_args_->args_) {
                ResolveIdentifiersInExpr(expr.get(), scope);
            }
        }
        for (auto& expr : call_expr->args_) {
            ResolveIdentifiersInExpr(expr.get(), scope);
        }
    } else if (ast::FuncLit *func_lit = dynamic_cast<ast::FuncLit *>(expr)) {
        ResolveIdentifiersInFuncLit(func_lit, scope);
    } else if (ast::CompositeLit *composite_lit = dynamic_cast<ast::CompositeLit *>(expr)) {
        ResolveIdentifiersInCompositeLit(composite_lit, scope);
    } else if (ast::ArrayType *array_type = dynamic_cast<ast::ArrayType *>(expr)) {
        if (array_type->len_) {
            ResolveIdentifiersInExpr(array_type->len_.get(), scope);
        }
        ResolveIdentifiersInExpr(array_type->element_type_.get(), scope);
    } else if (ast::FuncType *func_type = dynamic_cast<ast::FuncType *>(expr)) {
        ResolveIdentifiersInFuncType(func_type, scope);
    } else if (ast::InterfaceType *interface_type = dynamic_cast<ast::InterfaceType *>(expr)) {
        ResolveIdentifiersInInterfaceType(interface_type, scope);
    } else if (ast::StructType *struct_type = dynamic_cast<ast::StructType *>(expr)) {
        ResolveIdentifiersInStructType(struct_type, scope);
    } else if (ast::TypeInstance *type_instance = dynamic_cast<ast::TypeInstance *>(expr)) {
        ResolveIdentifiersInExpr(type_instance->type_.get(), scope);
        for (auto& expr : type_instance->type_args_->args_) {
            ResolveIdentifiersInExpr(expr.get(), scope);
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
    ResolveIdentifiersInExpr(sel->accessed_.get(), scope);
    if (sel->selection_->name_ == "_") {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        sel->selection_->start(),
                                        "can not select underscore"));
        return;
    }
    
    ast::Ident *accessed_ident = dynamic_cast<ast::Ident *>(sel->accessed_.get());
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
    
    ast::Ident *selected_ident = sel->selection_.get();
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
    
    ResolveIdentifiersInRegularFuncFieldList(func_lit->type_->params_.get(), func_scope);
    if (func_lit->type_->results_) {
        ResolveIdentifiersInRegularFuncFieldList(func_lit->type_->results_.get(), func_scope);
    }
    ResolveIdentifiersInBlockStmt(func_lit->body_.get(), func_scope);
}

void IdentifierResolver::ResolveIdentifiersInCompositeLit(ast::CompositeLit *composite_lit,
                                                   types::Scope *scope) {
    ResolveIdentifiersInExpr(composite_lit->type_.get(), scope);
    for (auto& value : composite_lit->values_) {
        ast::Expr *expr = value.get();
        if (auto key_value_expr = dynamic_cast<ast::KeyValueExpr *>(value.get())) {
            expr = key_value_expr->value_.get();
        }
        ResolveIdentifiersInExpr(expr, scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInFuncType(ast::FuncType *func_type,
                                                      types::Scope *scope) {
    types::Scope *func_scope = info_builder_.CreateScope(func_type, scope);
    
    ResolveIdentifiersInRegularFuncFieldList(func_type->params_.get(), func_scope);
    if (func_type->results_) {
        ResolveIdentifiersInRegularFuncFieldList(func_type->results_.get(), func_scope);
    }
}

void IdentifierResolver::ResolveIdentifiersInInterfaceType(ast::InterfaceType *interface_type,
                                                           types::Scope *scope) {
    types::Scope *interface_scope = info_builder_.CreateScope(interface_type, scope);
    
    for (auto& method_spec : interface_type->methods_) {
        types::Scope *method_scope = info_builder_.CreateScope(method_spec.get(), interface_scope);
        
        ResolveIdentifiersInRegularFuncFieldList(method_spec->params_.get(),
                                                 method_scope);
        if (method_spec->results_) {
            ResolveIdentifiersInRegularFuncFieldList(method_spec->results_.get(),
                                                     method_scope);
        }
    }
    for (auto& method_spec : interface_type->methods_) {
        types::Func *method = info_builder_.CreateFunc(interface_scope,
                                                       package_,
                                                       method_spec->start(),
                                                       method_spec->name_->name_);
        info_builder_.SetDefinedObject(method_spec->name_.get(), method);
        AddObjectToScope(interface_scope, method);
    }
}

void IdentifierResolver::ResolveIdentifiersInStructType(ast::StructType *struct_type,
                                                        types::Scope *scope) {
    types::Scope *struct_scope = info_builder_.CreateScope(struct_type, scope);
    
    for (auto& field : struct_type->fields_->fields_) {
        ResolveIdentifiersInExpr(field->type_.get(), scope);
    }
    for (auto& field : struct_type->fields_->fields_) {
        if (field->names_.empty()) {
            ast::Expr *type = field->type_.get();
            if (auto ptr_type = dynamic_cast<ast::UnaryExpr *>(type)) {
                if (ptr_type->op_ != tokens::kMul &&
                    ptr_type->op_ != tokens::kRem) {
                    issues_.push_back(
                                      issues::Issue(issues::Origin::TypeChecker,
                                                    issues::Severity::Error,
                                                    type->start(),
                                                    "expected embedded field to be defined type or "
                                                    "pointer to defined type"));
                    continue;
                }
                type = ptr_type->x_.get();
            }
            if (auto type_instance = dynamic_cast<ast::TypeInstance *>(type)) {
                type = type_instance->type_.get();
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
                                                                     field->type_->start(),
                                                                     defined_type->name_,
                                                                     /* is_embedded= */ true,
                                                                     /* is_field= */ true);
            info_builder_.SetImplicitObject(field.get(), variable);
            AddObjectToScope(struct_scope, variable);
        } else {
            for (auto& name : field->names_) {
                types::Variable *variable = info_builder_.CreateVariable(struct_scope,
                                                                         package_,
                                                                         name->start(),
                                                                         name->name_,
                                                                         /* is_embedded= */ false,
                                                                         /* is_field= */ true);
                info_builder_.SetDefinedObject(name.get(), variable);
                AddObjectToScope(struct_scope, variable);
            }
        }
    }
}

void IdentifierResolver::ResolveIdentifier(ast::Ident *ident,
                                           types::Scope *scope) {
    if (ident->name_ == "_") {
        return;
    }
    types::Object *object = scope->Lookup(ident->name_);
    if (object == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Fatal,
                                        ident->start(),
                                        "could not resolve identifier: " + ident->name_));
    }
    info_builder_.SetUsedObject(ident, object);
}

}
}
