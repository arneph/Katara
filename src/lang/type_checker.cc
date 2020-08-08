//
//  type_checker.cc
//  Katara
//
//  Created by Arne Philipeit on 7/26/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "type_checker.h"

#include <string>

#include "lang/ast_util.h"

namespace lang {
namespace type_checker {

void TypeChecker::Check(pos::File *pos_file,
                        ast::File *ast_file,
                        types::TypeInfo *info,
                        std::vector<Error>& errors) {
    TypeChecker checker(pos_file, ast_file, info, errors);
    
    if (info->universe_ == nullptr) {
        checker.SetupUniverse();
    }
    checker.ResolveIdentifiers();
}

void TypeChecker::SetupUniverse() {
    auto universe = std::unique_ptr<types::Scope>(new types::Scope());
    universe->parent_ = nullptr;
    
    info_->universe_ = universe.get();
    info_->scope_unique_ptrs_.push_back(std::move(universe));
    
    SetupPredeclaredTypes();
    SetupPredeclaredConstants();
    SetupPredeclaredNil();
}

void TypeChecker::SetupPredeclaredTypes() {
    typedef struct{
        types::Basic::Kind kind_;
        std::string name_;
    } predeclared_type_t;
    auto predeclared_types = std::vector<predeclared_type_t>({
        {types::Basic::kBool, "bool"},
        {types::Basic::kInt, "int"},
        {types::Basic::kInt8, "int8"},
        {types::Basic::kInt16, "int16"},
        {types::Basic::kInt32, "int32"},
        {types::Basic::kInt64, "int64"},
        {types::Basic::kUint, "uint"},
        {types::Basic::kUint8, "uint8"},
        {types::Basic::kUint16, "uint16"},
        {types::Basic::kUint32, "uint32"},
        {types::Basic::kUint64, "uint64"},
        
        {types::Basic::kUntypedBool, "untyped bool"},
        {types::Basic::kUntypedInt, "untyped int"},
        {types::Basic::kUntypedNil, "untyped nil"},
    });
    for (auto predeclared_type : predeclared_types) {
        auto basic = std::unique_ptr<types::Basic>(new types::Basic(predeclared_type.kind_));
        
        basic_types_.insert({predeclared_type.kind_, basic.get()});

        auto it = std::find(predeclared_type.name_.begin(),
                            predeclared_type.name_.end(),
                            ' ');
        if (it != predeclared_type.name_.end()) {
            continue;
        }
        
        auto type_name = std::unique_ptr<types::TypeName>(new types::TypeName());
        type_name->parent_ = info_->universe_;
        type_name->position_ = pos::kNoPos;
        type_name->name_ = predeclared_type.name_;
        type_name->type_ = basic.get();
        
        info_->universe_->named_objects_.insert({predeclared_type.name_, type_name.get()});
        info_->type_unique_ptrs_.push_back(std::move(basic));
        info_->object_unique_ptrs_.push_back(std::move(type_name));
    }
}

void TypeChecker::SetupPredeclaredConstants() {
    typedef struct{
        types::Basic::Kind kind_;
        constant::Value value_;
        std::string name_;
    } predeclared_const_t;
    auto predeclared_consts = std::vector<predeclared_const_t>({
        {types::Basic::kUntypedBool, constant::Value(false), "false"},
        {types::Basic::kUntypedBool, constant::Value(true), "true"},
        {types::Basic::kUntypedInt, constant::Value(int64_t{0}), "iota"},
    });
    for (auto predeclared_const : predeclared_consts) {
        auto constant = std::unique_ptr<types::Constant>(new types::Constant());
        constant->parent_ = info_->universe_;
        constant->position_ = pos::kNoPos;
        constant->name_ = predeclared_const.name_;
        constant->type_ = basic_types_.at(predeclared_const.kind_);
        constant->value_ = predeclared_const.value_;
        
        
        info_->universe_->named_objects_.insert({predeclared_const.name_, constant.get()});
        info_->object_unique_ptrs_.push_back(std::move(constant));
    }
}

void TypeChecker::SetupPredeclaredNil() {
    auto nil = std::unique_ptr<types::Nil>(new types::Nil());
    nil->parent_ = info_->universe_;
    nil->position_ = pos::kNoPos;
    nil->name_ = "nil";
    nil->type_ = basic_types_.at(types::Basic::kUntypedNil);
    
    info_->universe_->named_objects_.insert({"nil", nil.get()});
    info_->object_unique_ptrs_.push_back(std::move(nil));
}

void TypeChecker::ResolveIdentifiers() {
    auto file_scope = std::unique_ptr<types::Scope>(new types::Scope());
    file_scope->parent_ = info_->universe_;
    
    auto file_scope_ptr = file_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(file_scope));
    info_->scopes_.insert({ast_file_, file_scope_ptr});
    
    for (auto& decl : ast_file_->decls_) {
        if (auto gen_decl = dynamic_cast<ast::GenDecl *>(decl.get())) {
            AddDefinedObjectsFromGenDecl(gen_decl, file_scope_ptr);
        } else if (auto func_decl = dynamic_cast<ast::FuncDecl *>(decl.get())) {
            AddDefinedObjectFromFuncDecl(func_decl, file_scope_ptr);
        } else {
            throw "unexpected declaration";
        }
    }
    
    for (auto& decl : ast_file_->decls_) {
        if (auto gen_decl = dynamic_cast<ast::GenDecl *>(decl.get())) {
            ResolveIdentifiersInGenDecl(gen_decl, file_scope_ptr);
        } else if (auto func_decl = dynamic_cast<ast::FuncDecl *>(decl.get())) {
            ResolveIdentifiersInFuncDecl(func_decl, file_scope_ptr);
        } else {
            throw "unexpected declaration";
        }
    }
}

void TypeChecker::AddObjectToScope(types::Object *object, types::Scope *scope) {
    if (info_->universe_->Lookup(object->name_)) {
        errors_.push_back(Error{
            {object->position_},
            "can not redefine predeclared identifier: " + object->name_
        });
        return;
    }
    
    if (object->name_.empty()) {
        scope->unnamed_objects_.insert(object);
        return;
    }
    
    auto it = scope->named_objects_.find(object->name_);
    if (it != scope->named_objects_.end()) {
        types::Object *other = it->second;
        errors_.push_back(Error{
            {other->position_, object->position_},
            "naming collision: " + object->name_
        });
        return;
    }
    
    scope->named_objects_.insert({object->name_, object});
}

void TypeChecker::AddDefinedObjectsFromGenDecl(ast::GenDecl *gen_decl, types::Scope *scope) {
    switch (gen_decl->tok_) {
        case token::kConst:
            for (auto& spec : gen_decl->specs_) {
                AddDefinedObjectsFromConstSpec(static_cast<ast::ValueSpec *>(spec.get()),
                                               scope);
            }
            return;
        case token::kVar:
            for (auto& spec : gen_decl->specs_) {
                AddDefinedObjectsFromVarSpec(static_cast<ast::ValueSpec *>(spec.get()),
                                             scope);
            }
            return;
        case token::kType:
            for (auto& spec : gen_decl->specs_) {
                AddDefinedObjectFromTypeSpec(static_cast<ast::TypeSpec *>(spec.get()),
                                             scope);
            }
            return;
        default:
            throw "unexpected gen decl token";
    }
}

void TypeChecker::AddDefinedObjectsFromConstSpec(ast::ValueSpec *value_spec, types::Scope *scope) {
    for (auto& ident : value_spec->names_) {
        if (ident->name_ == "_") {
            continue;
        }
        
        auto constant = std::unique_ptr<types::Constant>(new types::Constant());
        constant->parent_ = scope;
        constant->position_ = ident->start();
        constant->name_ = ident->name_;
        
        auto constant_ptr = constant.get();
        info_->object_unique_ptrs_.push_back(std::move(constant));
        info_->definitions_.insert({ident.get(), constant_ptr});
        
        AddObjectToScope(constant_ptr, scope);
    }
}

void TypeChecker::AddDefinedObjectsFromVarSpec(ast::ValueSpec *value_spec, types::Scope *scope) {
    for (auto& ident : value_spec->names_) {
        if (ident->name_ == "_") {
            continue;
        }
        
        auto variable = std::unique_ptr<types::Variable>(new types::Variable());
        variable->parent_ = scope;
        variable->position_ = ident->start();
        variable->name_ = ident->name_;
        variable->is_embedded_ = false;
        variable->is_field_ = false;
        
        auto variable_ptr = variable.get();
        info_->object_unique_ptrs_.push_back(std::move(variable));
        info_->definitions_.insert({ident.get(), variable_ptr});
        
        AddObjectToScope(variable_ptr, scope);
    }
}

void TypeChecker::AddDefinedObjectFromTypeSpec(ast::TypeSpec *type_spec, types::Scope *scope) {
    if (type_spec->name_->name_ == "_") {
        errors_.push_back(Error{
            {type_spec->name_->start()},
            "blank type name not allowed"
        });
        return;
    }
    
    auto type_name = std::unique_ptr<types::TypeName>(new types::TypeName());
    type_name->parent_ = scope;
    type_name->position_ = type_spec->name_->start();
    type_name->name_ = type_spec->name_->name_;
    
    auto type_name_ptr = type_name.get();
    info_->object_unique_ptrs_.push_back(std::move(type_name));
    info_->definitions_.insert({type_spec->name_.get(), type_name_ptr});
    
    AddObjectToScope(type_name_ptr, scope);
}

void TypeChecker::AddDefinedObjectFromFuncDecl(ast::FuncDecl *func_decl, types::Scope *scope) {
    if (func_decl->name_->name_ == "_") {
        errors_.push_back(Error{
            {func_decl->name_->start()},
            "blank func name not allowed"
        });
        return;
    }
    
    auto func = std::unique_ptr<types::Func>(new types::Func());
    func->parent_ = scope;
    func->position_ = func_decl->name_->start();
    func->name_ = func_decl->name_->name_;
    
    auto func_ptr = func.get();
    info_->object_unique_ptrs_.push_back(std::move(func));
    info_->definitions_.insert({func_decl->name_.get(), func_ptr});

    AddObjectToScope(func_ptr, scope);
}

void TypeChecker::ResolveIdentifiersInGenDecl(ast::GenDecl *gen_decl, types::Scope *scope) {
    switch (gen_decl->tok_) {
        case token::kConst:
        case token::kVar:
            for (auto& spec : gen_decl->specs_) {
                ResolveIdentifiersInValueSpec(static_cast<ast::ValueSpec *>(spec.get()),
                                             scope);
            }
            return;
        case token::kType:
            for (auto& spec : gen_decl->specs_) {
                ResolveIdentifiersInTypeSpec(static_cast<ast::TypeSpec *>(spec.get()),
                                            scope);
            }
            return;
        default:
            throw "unexpected gen decl token";
    }
}

void TypeChecker::ResolveIdentifiersInValueSpec(ast::ValueSpec *value_spec, types::Scope *scope) {
    if (value_spec->type_) {
        ResolveIdentifiersInExpr(value_spec->type_.get(), scope);
    }
    for (auto& value : value_spec->values_) {
        ResolveIdentifiersInExpr(value.get(), scope);
    }
}

void TypeChecker::ResolveIdentifiersInTypeSpec(ast::TypeSpec *type_spec, types::Scope *scope) {
    auto type_scope = std::unique_ptr<types::Scope>(new types::Scope);
    type_scope->parent_ = scope;
    
    auto type_scope_ptr = type_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(type_scope));
    info_->scopes_.insert({type_spec, type_scope_ptr});
    
    if (type_spec->type_params_) {
        ResolveIdentifiersInTypeParamList(type_spec->type_params_.get(), type_scope_ptr);
    }
    ResolveIdentifiersInExpr(type_spec->type_.get(), type_scope_ptr);
}

void TypeChecker::ResolveIdentifiersInFuncDecl(ast::FuncDecl *func_decl, types::Scope *scope) {
    auto func_scope = std::unique_ptr<types::Scope>(new types::Scope);
    func_scope->parent_ = scope;
    
    auto func_scope_ptr = func_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(func_scope));
    info_->scopes_.insert({func_decl, func_scope_ptr});
    
    current_func_scope_ = func_scope_ptr;
    
    if (func_decl->receiver_) {
        ResolveIdentifiersInFuncReceiverFieldList(func_decl->receiver_.get(), func_scope_ptr);
    }
    if (func_decl->type_params_) {
        ResolveIdentifiersInTypeParamList(func_decl->type_params_.get(), func_scope_ptr);
    }
    ResolveIdentifiersInRegularFuncFieldList(func_decl->type_->params_.get(), func_scope_ptr);
    if (func_decl->type_->results_) {
        ResolveIdentifiersInRegularFuncFieldList(func_decl->type_->results_.get(), func_scope_ptr);
    }
    if (func_decl->body_) {
        ResolveIdentifiersInBlockStmt(func_decl->body_.get(), func_scope_ptr);
    }
    
    current_func_scope_ = nullptr;
}

void TypeChecker::ResolveIdentifiersInTypeParamList(ast::TypeParamList *type_param_list,
                                                   types::Scope *scope) {
    for (auto& type_param : type_param_list->params_) {
        if (!type_param->type_) {
            continue;
        }
        ResolveIdentifiersInExpr(type_param->type_.get(), scope);
    }
    for (auto& type_param : type_param_list->params_) {
        if (type_param->name_->name_ == "_") {
            errors_.push_back(Error{
                {type_param->name_->start()},
                "blank type parameter name not allowed"
            });
            continue;
        }
        
        auto type_name = std::unique_ptr<types::TypeName>(new types::TypeName());
        type_name->parent_ = scope;
        type_name->position_ = type_param->name_->start();
        type_name->name_ = type_param->name_->name_;
        
        auto type_name_ptr = type_name.get();
        info_->object_unique_ptrs_.push_back(std::move(type_name));
        info_->definitions_.insert({type_param->name_.get(), type_name_ptr});
        
        AddObjectToScope(type_name_ptr, scope);
    }
}

void TypeChecker::ResolveIdentifiersInFuncReceiverFieldList(ast::FieldList *field_list,
                                                           types::Scope *scope) {
    if (field_list->fields_.size() != 1 ||
        (field_list->fields_.size() > 0 &&
         field_list->fields_.at(0)->names_.size() > 1)) {
        errors_.push_back(Error{
            {field_list->start()},
            "expected one receiver"
        });
        if (field_list->fields_.size() == 0) {
            return;
        }
    }
    ast::Field *field = field_list->fields_.at(0).get();
    ast::Expr *type = field->type_.get();
    if (auto ptr_type = dynamic_cast<ast::UnaryExpr *>(type)) {
        if (ptr_type->op_ != token::kMul &&
            ptr_type->op_ != token::kRem) {
            errors_.push_back(Error{
                {type->start()},
                "expected receiver of defined type or pointer to defined type"
            });
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
        errors_.push_back(Error{
            {type->start()},
            "expected receiver of defined type or pointer to defined type"
        });
    } else {
        ResolveIdentifier(defined_type, scope);
    }
    
    if (type_args) {
        for (auto& type_arg : type_args->args_) {
            ast::Ident *ident = dynamic_cast<ast::Ident*>(type_arg.get());
            if (ident == nullptr) {
                errors_.push_back(Error{
                    {type->start()},
                    "expected type name definition as type argument to receiver type"
                });
                continue;
            }
            
            auto type_name = std::unique_ptr<types::TypeName>(new types::TypeName());
            type_name->parent_ = scope;
            type_name->position_ = ident->start();
            type_name->name_ = ident->name_;
            
            auto type_name_ptr = type_name.get();
            info_->object_unique_ptrs_.push_back(std::move(type_name));
            info_->definitions_.insert({ident, type_name_ptr});
            
            AddObjectToScope(type_name_ptr, scope);
        }
    }
    
    if (field->names_.size() == 0 ||
        field->names_.at(0)->name_ == "_") {
        return;
    }
    
    auto variable = std::unique_ptr<types::Variable>(new types::Variable());
    variable->parent_ = scope;
    variable->position_ = field->names_.at(0)->start();
    variable->name_ = field->names_.at(0)->name_;
    variable->is_embedded_ = false;
    variable->is_field_ = false;
    
    auto variable_ptr = variable.get();
    info_->object_unique_ptrs_.push_back(std::move(variable));
    info_->definitions_.insert({field->names_.at(0).get(), variable_ptr});
    
    AddObjectToScope(variable_ptr, scope);
}

void TypeChecker::ResolveIdentifiersInRegularFuncFieldList(ast::FieldList *field_list,
                                                          types::Scope *scope) {
    for (auto& field : field_list->fields_) {
        ResolveIdentifiersInExpr(field->type_.get(), scope);
    }
    for (auto& field : field_list->fields_) {
        for (auto& name : field->names_) {
            auto variable = std::unique_ptr<types::Variable>(new types::Variable());
            variable->parent_ = scope;
            variable->position_ = name->start();
            variable->name_ = name->name_;
            variable->is_embedded_ = false;
            variable->is_field_ = false;
            
            auto variable_ptr = variable.get();
            info_->object_unique_ptrs_.push_back(std::move(variable));
            info_->definitions_.insert({name.get(), variable_ptr});
            
            AddObjectToScope(variable_ptr, scope);
        }
    }
}

void TypeChecker::ResolveIdentifiersInStmt(ast::Stmt *stmt, types::Scope *scope) {
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

void TypeChecker::ResolveIdentifiersInBlockStmt(ast::BlockStmt *body, types::Scope *scope) {
    for (auto& stmt : body->stmts_) {
        auto labeled_stmt = dynamic_cast<ast::LabeledStmt *>(stmt.get());
        if (labeled_stmt == nullptr) {
            continue;
        }
        
        auto label = std::unique_ptr<types::Label>(new types::Label);
        label->parent_ = scope;
        label->position_ = labeled_stmt->start();
        label->name_ = labeled_stmt->label_->name_;
        
        auto label_ptr = label.get();
        info_->object_unique_ptrs_.push_back(std::move(label));
        info_->definitions_.insert({labeled_stmt->label_.get(), label_ptr});
        
        AddObjectToScope(label_ptr, scope);
    }
    for (auto& stmt : body->stmts_) {
        ResolveIdentifiersInStmt(stmt.get(), scope);
    }
}

void TypeChecker::ResolveIdentifiersInDeclStmt(ast::DeclStmt *decl_stmt, types::Scope *scope) {
    switch (decl_stmt->decl_->tok_) {
        case token::kConst:
            for (auto& spec : decl_stmt->decl_->specs_) {
                auto value_spec = static_cast<ast::ValueSpec *>(spec.get());
                ResolveIdentifiersInValueSpec(value_spec, scope);
                AddDefinedObjectsFromConstSpec(value_spec, scope);
            }
            return;
        case token::kVar:
            for (auto& spec : decl_stmt->decl_->specs_) {
                auto value_spec = static_cast<ast::ValueSpec *>(spec.get());
                ResolveIdentifiersInValueSpec(value_spec, scope);
                AddDefinedObjectsFromVarSpec(value_spec, scope);
            }
            return;
        case token::kType:
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

void TypeChecker::ResolveIdentifiersInAssignStmt(ast::AssignStmt *assign_stmt, types::Scope *scope) {
    for (auto& expr : assign_stmt->rhs_) {
        ResolveIdentifiersInExpr(expr.get(), scope);
    }
    for (auto& expr : assign_stmt->lhs_) {
        if (assign_stmt->tok_ == token::kDefine) {
            auto ident = dynamic_cast<ast::Ident *>(expr.get());
            if (ident != nullptr) {
                const types::Scope *defining_scope = nullptr;
                scope->Lookup(ident->name_, defining_scope);
                if (defining_scope != scope) {
                    auto variable = std::unique_ptr<types::Variable>(new types::Variable());
                    variable->parent_ = scope;
                    variable->position_ = ident->start();
                    variable->name_ = ident->name_;
                    variable->is_embedded_ = false;
                    variable->is_field_ = false;
                    
                    auto variable_ptr = variable.get();
                    info_->object_unique_ptrs_.push_back(std::move(variable));
                    info_->definitions_.insert({ident, variable_ptr});
                    
                    AddObjectToScope(variable_ptr, scope);
                }
            }
        }
        ResolveIdentifiersInExpr(expr.get(), scope);
    }
}

void TypeChecker::ResolveIdentifiersInIfStmt(ast::IfStmt *if_stmt, types::Scope *scope) {
    auto if_scope = std::unique_ptr<types::Scope>(new types::Scope);
    if_scope->parent_ = scope;
    
    auto if_scope_ptr = if_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(if_scope));
    info_->scopes_.insert({if_stmt, if_scope_ptr});
    
    if (if_stmt->init_) {
        ResolveIdentifiersInStmt(if_stmt->init_.get(), if_scope_ptr);
    }
    ResolveIdentifiersInExpr(if_stmt->cond_.get(), if_scope_ptr);
    ResolveIdentifiersInBlockStmt(if_stmt->body_.get(), if_scope_ptr);
    if (if_stmt->else_) {
        ResolveIdentifiersInStmt(if_stmt->else_.get(), scope);
    }
}

void TypeChecker::ResolveIdentifiersInSwitchStmt(ast::SwitchStmt *switch_stmt, types::Scope *scope) {
    auto switch_scope = std::unique_ptr<types::Scope>(new types::Scope);
    switch_scope->parent_ = scope;
    
    auto switch_scope_ptr = switch_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(switch_scope));
    info_->scopes_.insert({switch_stmt, switch_scope_ptr});
    
    if (switch_stmt->init_) {
        ResolveIdentifiersInStmt(switch_stmt->init_.get(), switch_scope_ptr);
    }
    if (switch_stmt->tag_) {
        ResolveIdentifiersInExpr(switch_stmt->tag_.get(), switch_scope_ptr);
    }
    if (ast::IsTypeSwitchStmt(switch_stmt)) {
        ast::Ident *ident = nullptr;
        if (switch_stmt->init_) {
            auto assign_stmt = static_cast<ast::AssignStmt *>(switch_stmt->init_.get());
            ident = static_cast<ast::Ident *>(assign_stmt->lhs_.at(0).get());
        }
        for (auto& stmt : switch_stmt->body_->stmts_) {
            auto case_clause = static_cast<ast::CaseClause *>(stmt.get());
            ResolveIdentifiersInCaseClause(case_clause, switch_scope_ptr, ident);
        }
    } else {
        for (auto& stmt : switch_stmt->body_->stmts_) {
            auto case_clause = static_cast<ast::CaseClause *>(stmt.get());
            ResolveIdentifiersInCaseClause(case_clause, switch_scope_ptr);
        }
    }
}

void TypeChecker::ResolveIdentifiersInCaseClause(ast::CaseClause *case_clause,
                                                 types::Scope *scope,
                                                 ast::Ident *type_switch_var_ident) {
    auto case_scope = std::unique_ptr<types::Scope>(new types::Scope);
    case_scope->parent_ = scope;
    
    auto case_scope_ptr = case_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(case_scope));
    info_->scopes_.insert({case_clause, case_scope_ptr});
    
    for (auto& expr : case_clause->cond_vals_) {
        ResolveIdentifiersInExpr(expr.get(), case_scope_ptr);
    }
    if (type_switch_var_ident != nullptr) {
        auto variable = std::unique_ptr<types::Variable>(new types::Variable());
        variable->parent_ = case_scope_ptr;
        variable->position_ = type_switch_var_ident->start();
        variable->name_ = type_switch_var_ident->name_;
        variable->is_embedded_ = false;
        variable->is_field_ = false;
        
        auto variable_ptr = variable.get();
        info_->object_unique_ptrs_.push_back(std::move(variable));
        info_->implicits_.insert({case_clause, variable_ptr});
        
        AddObjectToScope(variable_ptr, scope);
    }
    for (auto& stmt : case_clause->body_) {
        auto labeled_stmt = dynamic_cast<ast::LabeledStmt *>(stmt.get());
        if (labeled_stmt == nullptr) {
            continue;
        }
        
        auto label = std::unique_ptr<types::Label>(new types::Label);
        label->parent_ = case_scope_ptr;
        label->position_ = labeled_stmt->start();
        label->name_ = labeled_stmt->label_->name_;
        
        auto label_ptr = label.get();
        info_->object_unique_ptrs_.push_back(std::move(label));
        info_->definitions_.insert({labeled_stmt->label_.get(), label_ptr});
        
        AddObjectToScope(label_ptr, case_scope_ptr);
    }
    for (auto& stmt : case_clause->body_) {
        ResolveIdentifiersInStmt(stmt.get(), case_scope_ptr);
    }
}

void TypeChecker::ResolveIdentifiersInForStmt(ast::ForStmt *for_stmt, types::Scope *scope) {
    auto for_scope = std::unique_ptr<types::Scope>(new types::Scope);
    for_scope->parent_ = scope;
    
    auto for_scope_ptr = for_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(for_scope));
    info_->scopes_.insert({for_stmt, for_scope_ptr});

    if (for_stmt->init_) {
        ResolveIdentifiersInStmt(for_stmt->init_.get(), for_scope_ptr);
    }
    if (for_stmt->cond_) {
        ResolveIdentifiersInExpr(for_stmt->cond_.get(), for_scope_ptr);
    }
    if (for_stmt->post_) {
        if (auto assign_stmt = dynamic_cast<ast::AssignStmt *>(for_stmt->post_.get())) {
            if (assign_stmt->tok_ == token::kDefine) {
                errors_.push_back(Error{
                    {assign_stmt->start()},
                    "post statements of for loops can not define variables"
                });
            }
        }
        ResolveIdentifiersInStmt(for_stmt->post_.get(), for_scope_ptr);
    }
    ResolveIdentifiersInBlockStmt(for_stmt->body_.get(), for_scope_ptr);
}

void TypeChecker::ResolveIdentifiersInBranchStmt(ast::BranchStmt *branch_stmt, types::Scope *scope) {
    if (!branch_stmt->label_) {
        return;
    }
    const types::Scope* defining_scope;
    types::Object *obj = scope->Lookup(branch_stmt->label_->name_, defining_scope);
    if (dynamic_cast<types::Label *>(obj) == nullptr) {
        errors_.push_back(Error{
            {branch_stmt->label_->start()},
            "branch statement does not refer to known label"
        });
        return;
    }
    ResolveIdentifiersInExpr(branch_stmt->label_.get(), scope);
}

void TypeChecker::ResolveIdentifiersInExpr(ast::Expr *expr, types::Scope *scope) {
    if (ast::UnaryExpr *unary_expr = dynamic_cast<ast::UnaryExpr *>(expr)) {
        ResolveIdentifiersInExpr(unary_expr->x_.get(), scope);
    } else if (ast::BinaryExpr *binary_expr = dynamic_cast<ast::BinaryExpr *>(expr)) {
        ResolveIdentifiersInExpr(binary_expr->x_.get(), scope);
        ResolveIdentifiersInExpr(binary_expr->y_.get(), scope);
    } else if (ast::ParenExpr *paren_expr = dynamic_cast<ast::ParenExpr *>(expr)) {
        ResolveIdentifiersInExpr(paren_expr->x_.get(), scope);
    } else if (ast::SelectionExpr *selection_expr = dynamic_cast<ast::SelectionExpr *>(expr)) {
        ResolveIdentifiersInExpr(selection_expr->accessed_.get(), scope);
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

void TypeChecker::ResolveIdentifiersInFuncLit(ast::FuncLit *func_lit, types::Scope *scope) {
    auto func = std::unique_ptr<types::Func>(new types::Func());
    func->parent_ = scope;
    func->position_ = func_lit->start();
    func->name_ = "";
    
    auto func_ptr = func.get();
    info_->object_unique_ptrs_.push_back(std::move(func));
    
    AddObjectToScope(func_ptr, scope);
    
    auto func_scope = std::unique_ptr<types::Scope>(new types::Scope);
    func_scope->parent_ = scope;
    
    auto func_scope_ptr = func_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(func_scope));
    info_->scopes_.insert({func_lit, func_scope_ptr});
    
    auto enclosing_func_scope = current_func_scope_;
    current_func_scope_ = func_scope_ptr;
    
    ResolveIdentifiersInRegularFuncFieldList(func_lit->type_->params_.get(), func_scope_ptr);
    if (func_lit->type_->results_) {
        ResolveIdentifiersInRegularFuncFieldList(func_lit->type_->results_.get(), func_scope_ptr);
    }
    ResolveIdentifiersInBlockStmt(func_lit->body_.get(), func_scope_ptr);
    
    current_func_scope_ = enclosing_func_scope;
}

void TypeChecker::ResolveIdentifiersInCompositeLit(ast::CompositeLit *composite_lit,
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

void TypeChecker::ResolveIdentifiersInFuncType(ast::FuncType *func_type,
                                               types::Scope *scope) {
    auto func_scope = std::unique_ptr<types::Scope>(new types::Scope);
    func_scope->parent_ = scope;
    
    auto func_scope_ptr = func_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(func_scope));
    info_->scopes_.insert({func_type, func_scope_ptr});
    
    ResolveIdentifiersInRegularFuncFieldList(func_type->params_.get(), func_scope_ptr);
    if (func_type->results_) {
        ResolveIdentifiersInRegularFuncFieldList(func_type->results_.get(), func_scope_ptr);
    }
}

void TypeChecker::ResolveIdentifiersInInterfaceType(ast::InterfaceType *interface_type,
                                                    types::Scope *scope) {
    auto interface_scope = std::unique_ptr<types::Scope>(new types::Scope);
    interface_scope->parent_ = scope;
    
    auto interface_scope_ptr = interface_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(interface_scope));
    info_->scopes_.insert({interface_type, interface_scope_ptr});

    for (auto& method_spec : interface_type->methods_) {
        auto method_scope = std::unique_ptr<types::Scope>(new types::Scope);
        method_scope->parent_ = interface_scope_ptr;
        
        auto method_scope_ptr = method_scope.get();
        info_->scope_unique_ptrs_.push_back(std::move(method_scope));
        info_->scopes_.insert({method_spec.get(), method_scope_ptr});
        
        ResolveIdentifiersInRegularFuncFieldList(method_spec->params_.get(),
                                                 method_scope_ptr);
        if (method_spec->results_) {
            ResolveIdentifiersInRegularFuncFieldList(method_spec->results_.get(),
                                                     method_scope_ptr);
        }
    }
    for (auto& method_spec : interface_type->methods_) {
        auto method = std::unique_ptr<types::Func>(new types::Func());
        method->parent_ = interface_scope_ptr;
        method->position_ = method_spec->start();
        method->name_ = method_spec->name_->name_;
        
        auto method_ptr = method.get();
        info_->object_unique_ptrs_.push_back(std::move(method));
        info_->definitions_.insert({method_spec->name_.get(), method_ptr});
        
        AddObjectToScope(method_ptr, interface_scope_ptr);
    }
}

void TypeChecker::ResolveIdentifiersInStructType(ast::StructType *struct_type,
                                                 types::Scope *scope) {
    auto struct_scope = std::unique_ptr<types::Scope>(new types::Scope);
    struct_scope->parent_ = scope;
    
    auto struct_scope_ptr = struct_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(struct_scope));
    info_->scopes_.insert({struct_type, struct_scope_ptr});
    
    for (auto& field : struct_type->fields_->fields_) {
        ResolveIdentifiersInExpr(field->type_.get(), scope);
    }
    for (auto& field : struct_type->fields_->fields_) {
        if (field->names_.empty()) {
            ast::Expr *type = field->type_.get();
            if (auto ptr_type = dynamic_cast<ast::UnaryExpr *>(type)) {
                if (ptr_type->op_ != token::kMul &&
                    ptr_type->op_ != token::kRem) {
                    errors_.push_back(Error{
                        {type->start()},
                        "expected embedded field to be defined type or pointer to defined type"
                    });
                    continue;
                }
                type = ptr_type->x_.get();
            }
            if (auto type_instance = dynamic_cast<ast::TypeInstance *>(type)) {
                type = type_instance->type_.get();
            }
            ast::Ident *defined_type = dynamic_cast<ast::Ident *>(type);
            if (defined_type == nullptr) {
                errors_.push_back(Error{
                    {type->start()},
                    "expected embdedded field to be defined type or pointer to defined type"
                });
                continue;
            }
            
            auto variable = std::unique_ptr<types::Variable>(new types::Variable());
            variable->parent_ = struct_scope_ptr;
            variable->position_ = field->type_->start();
            variable->name_ = defined_type->name_;
            variable->is_embedded_ = true;
            variable->is_field_ = true;
            
            auto variable_ptr = variable.get();
            info_->object_unique_ptrs_.push_back(std::move(variable));
            info_->implicits_.insert({field.get(), variable_ptr});
            
            AddObjectToScope(variable_ptr, struct_scope_ptr);
        } else {
            for (auto& name : field->names_) {
                auto variable = std::unique_ptr<types::Variable>(new types::Variable());
                variable->parent_ = struct_scope_ptr;
                variable->position_ = name->start();
                variable->name_ = name->name_;
                variable->is_embedded_ = false;
                variable->is_field_ = true;
                
                auto variable_ptr = variable.get();
                info_->object_unique_ptrs_.push_back(std::move(variable));
                info_->definitions_.insert({name.get(), variable_ptr});
                
                AddObjectToScope(variable_ptr, struct_scope_ptr);
            }
        }
    }
}

void TypeChecker::ResolveIdentifier(ast::Ident *ident,
                                    types::Scope *scope) {
    if (ident->name_ == "_") {
        return;
    }
    types::Object *obj = scope->Lookup(ident->name_);
    if (obj == nullptr) {
        errors_.push_back(Error{
            {ident->start()},
            "could not resolve identifier"
        });
    }
    info_->uses_.insert({ident, obj});
}

}
}
