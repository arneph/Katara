//
//  package_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "package_handler.h"

#include "lang/representation/tokens/tokens.h"
#include "lang/representation/ast/ast_util.h"

#include "lang/processors/type_checker/type_handler.h"
#include "lang/processors/type_checker/constant_handler.h"
#include "lang/processors/type_checker/variable_handler.h"
#include "lang/processors/type_checker/stmt_handler.h"

namespace lang {
namespace type_checker {

bool PackageHandler::ProcessPackage(std::vector<ast::File *> package_files,
                                    types::Package *package,
                                    types::InfoBuilder& info_builder,
                                    std::vector<issues::Issue> &issues) {
    PackageHandler manager(package_files, package, info_builder, issues);
    
    manager.FindActions();
    
    std::vector<Action *> ordered_actions = manager.FindActionOrder();
    
    return manager.ExecuteActions(ordered_actions);
}

PackageHandler::PackageHandler(std::vector<ast::File *> package_files,
                               types::Package *package,
                               types::InfoBuilder& info_builder,
                               std::vector<issues::Issue> &issues)
    : package_files_(package_files), package_(package),
      info_(info_builder.info()), info_builder_(info_builder), issues_(issues) {}

PackageHandler::Action *
PackageHandler::CreateAction(std::unordered_set<types::Object *> prerequisites,
                             types::Object *defined_object,
                             std::function<bool ()> executor) {
    return CreateAction(prerequisites,
                        std::unordered_set<types::Object *>{defined_object},
                        executor);
}

PackageHandler::Action *
PackageHandler::CreateAction(std::unordered_set<types::Object *> prerequisites,
                             std::unordered_set<types::Object *> defined_objects,
                             std::function<bool ()> executor) {
    std::unique_ptr<Action> action = std::make_unique<Action>(prerequisites,
                                                              defined_objects,
                                                              executor);
    Action *action_ptr = action.get();
    actions_.push_back(std::move(action));
    return action_ptr;
}

void PackageHandler::FindActions() {
    for (ast::File *file : package_files_) {
        for (ast::Decl *decl : file->decls()) {
            if (ast::GenDecl *gen_decl = dynamic_cast<ast::GenDecl *>(decl)) {
                switch (gen_decl->tok()) {
                    case tokens::kImport:
                        break;
                    case tokens::kType:
                        FindActionsForTypeDecl(gen_decl);
                        break;
                    case tokens::kConst:
                        FindActionsForConstDecl(gen_decl);
                        break;
                    case tokens::kVar:
                        FindActionsForVarDecl(gen_decl);
                        break;
                    default:
                        throw "internal error: unexpected lang::ast::GenDecl";
                }
            } else if (ast::FuncDecl *func_decl = dynamic_cast<ast::FuncDecl *>(decl)) {
                FindActionsForFuncDecl(func_decl);
            }
        }
    }
}

void PackageHandler::FindActionsForTypeDecl(ast::GenDecl *type_decl) {
    for (ast::Spec *spec : type_decl->specs()) {
        ast::TypeSpec *type_spec = static_cast<ast::TypeSpec *>(spec);
        types::TypeName *type_name =
            static_cast<types::TypeName *>(info_->definitions().at(type_spec->name()));
        
        std::unordered_set<types::Object *> defined_objects;
        defined_objects.insert(type_name);
        std::unordered_set<types::Object *> param_prerequisites;
        if (type_spec->type_params()) {
            for (ast::TypeParam *type_param_expr : type_spec->type_params()->params()) {
                ast::Ident *type_param_name = type_param_expr->name();
                types::Object *type_param = info_->DefinitionOf(type_param_name);
                defined_objects.insert(type_param);
            }
            
            param_prerequisites = FindPrerequisites(type_spec->type_params());
            for (types::Object *prerequisite : param_prerequisites) {
                if (dynamic_cast<types::TypeName *>(prerequisite) == nullptr &&
                    dynamic_cast<types::Constant *>(prerequisite) == nullptr) {
                    issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                    issues::Severity::Error,
                                                    {type_name->position(), prerequisite->position()},
                                                    "type can only depend on types and constants"));
                }
            }
        }
        
        std::unordered_set<types::Object *> underlying_prerequisites =
            FindPrerequisites(type_spec->type());
        for (types::Object *prerequisite : underlying_prerequisites) {
            if (dynamic_cast<types::TypeName *>(prerequisite) == nullptr &&
                dynamic_cast<types::Constant *>(prerequisite) == nullptr) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                {type_name->position(), prerequisite->position()},
                                                "type can only depend on types and constants"));
            }
        }
        
        Action *param_action = CreateAction(param_prerequisites,
                                            defined_objects,
                                            [=]() -> bool {
            if (type_spec->type_params() == nullptr) {
                return true;
            }
            return TypeHandler::ProcessTypeParametersOfTypeName(type_name,
                                                                type_spec,
                                                                info_builder_,
                                                                issues_);
        });
        Action *underlying_action = CreateAction(underlying_prerequisites,
                                                 std::unordered_set<types::Object *>{},
                                                 [=]() -> bool {
            return TypeHandler::ProcessUnderlyingTypeOfTypeName(type_name,
                                                                type_spec,
                                                                info_builder_,
                                                                issues_);
        });
        const_and_type_actions_.push_back(param_action);
        const_and_type_actions_.push_back(underlying_action);
    }
}

void PackageHandler::FindActionsForConstDecl(ast::GenDecl *const_decl) {
    int64_t iota = 0;
    for (ast::Spec *spec : const_decl->specs()) {
        ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec);
        
        for (size_t i = 0; i < value_spec->names().size(); i++) {
            ast::Ident *name = value_spec->names().at(i);
            types::Constant *constant =
                static_cast<types::Constant *>(info_->definitions().at(name));
            
            ast::Expr *type_expr = nullptr;
            ast::Expr *value = nullptr;
            std::unordered_set<types::Object *> prerequisites;
            if (value_spec->type()) {
                type_expr = value_spec->type();
                std::unordered_set<types::Object *> type_prerequisites =
                    FindPrerequisites(type_expr);
                prerequisites.insert(type_prerequisites.begin(),
                                     type_prerequisites.end());
            }
            if (value_spec->values().size() > i) {
                value = value_spec->values().at(i);
                std::unordered_set<types::Object *> value_prerequisites =
                    FindPrerequisites(value);
                prerequisites.insert(value_prerequisites.begin(),
                                     value_prerequisites.end());
            }
            for (types::Object *prerequisite : prerequisites) {
                if (dynamic_cast<types::TypeName *>(prerequisite) == nullptr &&
                    dynamic_cast<types::Constant *>(prerequisite) == nullptr) {
                    issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                    issues::Severity::Error,
                                                    {constant->position(),
                                                     prerequisite->position()},
                                                    "constant can only depend on types and "
                                                    "constants"));
                }
            }
            
            Action *action = CreateAction(prerequisites,
                                          constant,
                                          [=]() -> bool {
                types::Type *type = nullptr;
                if (type_expr != nullptr) {
                    if (!TypeHandler::ProcessTypeExpr(type_expr, info_builder_, issues_)) {
                        return false;
                    }
                    type = info_->TypeOf(type_expr);
                }
                
                return ConstantHandler::ProcessConstant(constant,
                                                        type,
                                                        value,
                                                        iota,
                                                        info_builder_,
                                                        issues_);
            });
            const_and_type_actions_.push_back(action);
        }
        iota++;
    }
}

void PackageHandler::FindActionsForVarDecl(ast::GenDecl *var_decl) {
    for (ast::Spec *spec : var_decl->specs()) {
        ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec);
        
        ast::Expr *type_expr = nullptr;
        std::unordered_set<types::Object *> type_prerequisites;
        if (value_spec->type()) {
            type_expr = value_spec->type();
            type_prerequisites = FindPrerequisites(type_expr);
            for (types::Object *prerequisite : type_prerequisites) {
                if (dynamic_cast<types::TypeName *>(prerequisite) == nullptr &&
                    dynamic_cast<types::Constant *>(prerequisite) == nullptr) {
                    issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                    issues::Severity::Error,
                                                    prerequisite->position(),
                                                    "type can only depend on types and "
                                                    "constants"));
                }
            }
        }
        
        if (value_spec->names().size() > 1 && value_spec->values().size() == 1) {
            std::vector<types::Variable *> variables;
            std::unordered_set<types::Object *> objects;
            for (ast::Ident *name : value_spec->names()) {
                types::Variable *variable =
                    static_cast<types::Variable *>(info_->definitions().at(name));
                
                variables.push_back(variable);
                objects.insert(variable);
            }
            
            ast::Expr *value = value_spec->values().at(0);
            std::unordered_set<types::Object *> prerequisites = FindPrerequisites(value);
            prerequisites.insert(type_prerequisites.begin(),
                                 type_prerequisites.end());
            
            Action *action = CreateAction(prerequisites,
                                          objects,
                                          [=]() -> bool {
                types::Type *type = nullptr;
                if (type_expr != nullptr) {
                    if (!TypeHandler::ProcessTypeExpr(type_expr, info_builder_, issues_)) {
                        return false;
                    }
                    type = info_->TypeOf(type_expr);
                }
                
                return VariableHandler::ProcessVariables(variables,
                                                         type,
                                                         value,
                                                         info_builder_,
                                                         issues_);
            });
            variable_and_func_decl_actions_.push_back(action);
            
        } else {
            for (size_t i = 0; i < value_spec->names().size(); i++) {
                ast::Ident *name = value_spec->names().at(i);
                types::Variable *variable =
                    static_cast<types::Variable *>(info_->definitions().at(name));
                
                ast::Expr *value = nullptr;
                std::unordered_set<types::Object *> prerequisites;
                prerequisites.insert(type_prerequisites.begin(),
                                     type_prerequisites.end());
                if (value_spec->values().size() > i) {
                    value = value_spec->values().at(i);
                    std::unordered_set<types::Object *> value_prerequisites
                        = FindPrerequisites(value);
                    prerequisites.insert(value_prerequisites.begin(),
                                         value_prerequisites.end());
                }
                
                Action *action = CreateAction(prerequisites,
                                              variable,
                                              [=]() -> bool {
                    types::Type *type = nullptr;
                    if (type_expr != nullptr) {
                        if (!TypeHandler::ProcessTypeExpr(type_expr, info_builder_, issues_)) {
                            return false;
                        }
                        type = info_->TypeOf(type_expr);
                    }
                    
                    return VariableHandler::ProcessVariable(variable,
                                                            type,
                                                            value,
                                                            info_builder_,
                                                            issues_);
                });
                variable_and_func_decl_actions_.push_back(action);
            }
        }
    }
}

void PackageHandler::FindActionsForFuncDecl(ast::FuncDecl *func_decl) {
    ast::Ident *name = func_decl->name();
    ast::BlockStmt *body = func_decl->body();
    types::Func *func = static_cast<types::Func *>(info_->definitions().at(name));
    
    std::unordered_set<types::Object *> prerequisites = FindPrerequisites(func_decl);
    
    Action *decl_action = CreateAction(prerequisites,
                                       func,
                                       [=]() -> bool {
        return TypeHandler::ProcessFuncDecl(func,
                                            func_decl,
                                            info_builder_,
                                            issues_);
    });
    Action *body_action = CreateAction(prerequisites,
                                       func,
                                       [=]() -> bool {
        types::Signature *signature = static_cast<types::Signature *>(func->type());
        StmtHandler::ProcessFuncBody(body,
                                     signature->results(),
                                     info_builder_,
                                     issues_);
        return true;
    });
    variable_and_func_decl_actions_.push_back(decl_action);
    func_body_actions_.push_back(body_action);
}

std::unordered_set<types::Object *> PackageHandler::FindPrerequisites(ast::Node *node) {
    std::unordered_set<types::Object *> objects;
    ast::WalkFunction walker =
    ast::WalkFunction([&](ast::Node *node) -> ast::WalkFunction {
        if (node == nullptr) {
            return walker;
        }
        ast::Ident *ident = dynamic_cast<ast::Ident *>(node);
        if (ident == nullptr) {
            return walker;
        }
        auto it = info_->uses().find(ident);
        if (it == info_->uses().end() ||
            it->second->parent() != package_->scope()) {
            return walker;
        }
        objects.insert(it->second);
        return walker;
    });
    ast::Walk(node, walker);
    return objects;
}

std::vector<PackageHandler::Action *> PackageHandler::FindActionOrder() {
    std::unordered_set<types::Object *> defined_objects;
    
    std::vector<Action *> ordered_const_and_type_actions =
        FindActionOrderForActions(const_and_type_actions_,
                                  defined_objects);

    std::vector<Action *> ordered_variable_and_func_decl_actions =
        FindActionOrderForActions(variable_and_func_decl_actions_,
                                  defined_objects);

    std::vector<Action *> ordered_func_body_actions =
        FindActionOrderForActions(func_body_actions_,
                                  defined_objects);

    std::vector<Action *> ordered_actions;
    ordered_actions.reserve(ordered_const_and_type_actions.size() +
                            ordered_variable_and_func_decl_actions.size() +
                            ordered_func_body_actions.size());
    ordered_actions.insert(ordered_actions.end(),
                           ordered_const_and_type_actions.begin(),
                           ordered_const_and_type_actions.end());
    ordered_actions.insert(ordered_actions.end(),
                           ordered_variable_and_func_decl_actions.begin(),
                           ordered_variable_and_func_decl_actions.end());
    ordered_actions.insert(ordered_actions.end(),
                           ordered_func_body_actions.begin(),
                           ordered_func_body_actions.end());
    
    return ordered_actions;
}

std::vector<PackageHandler::Action *>
PackageHandler::FindActionOrderForActions(const std::vector<Action *>& actions,
                                          std::unordered_set<types::Object *>& defined_objects) {
    std::unordered_set<Action *> completed_actions;
    std::vector<Action *> ordered_actions;
    ordered_actions.reserve(actions.size());
    
    while (ordered_actions.size() < actions.size()) {
        bool made_progress = false;
        
        for (Action *action : actions) {
            if (completed_actions.contains(action)) {
                continue;
            }
            
            bool is_possible = true;
            for (types::Object *prerequisite : action->prerequisites()) {
                if (!defined_objects.contains(prerequisite)) {
                    is_possible = false;
                    break;
                }
            }
            
            if (is_possible) {
                made_progress = true;
                defined_objects.insert(action->defined_objects().begin(),
                                       action->defined_objects().end());
                completed_actions.insert(action);
                ordered_actions.push_back(action);
            }
        }
        
        if (!made_progress) {
            ReportLoopInActions(actions);
            return {};
        }
    }
    
    return ordered_actions;
}

void PackageHandler::ReportLoopInActions(const std::vector<Action *>& actions) {
    std::unordered_set<types::Object *> loop_members;
    for (Action *action : actions) {
        std::vector<Action *> stack{action};
        
        loop_members = FindLoop(actions, stack);
        if (!loop_members.empty()) {
            break;
        }
    }
    if (loop_members.empty()) {
        for (Action *action : actions) {
            loop_members.insert(action->defined_objects().begin(),
                                action->defined_objects().end());
        }
    }
    
    std::vector<pos::pos_t> loop_member_positions;
    std::string message = "encountered dependency loop involving: ";
    for (types::Object *loop_member : loop_members) {
        loop_member_positions.push_back(loop_member->position());
        if (loop_member_positions.size() > 1) {
            message += ", ";
        }
        message += loop_member->name();
    }
    
    issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                    issues::Severity::Error,
                                    loop_member_positions,
                                    message));
}

std::unordered_set<types::Object *>
PackageHandler::FindLoop(const std::vector<Action *>& actions,
                         std::vector<Action *> stack) {
    Action *current_action = stack.back();
    
    for (types::Object *prerequisite_object : current_action->prerequisites()) {
        int i = 0;
        for (; i < stack.size(); i++) {
            Action *stack_action = stack.at(i);
            if (stack_action->defined_objects().contains(prerequisite_object)) {
                break;
            }
        }
        if (i == stack.size()) {
            continue;
        }
        std::unordered_set<types::Object *> loop_members;
        for (; i < stack.size(); i++) {
            Action *stack_action = stack.at(i);
            loop_members.insert(stack_action->defined_objects().begin(),
                                stack_action->defined_objects().end());
        }
        return loop_members;
    }
    
    for (types::Object *prerequisite_object : current_action->prerequisites()) {
        for (Action *prerequisite_action : actions) {
            if (!prerequisite_action->defined_objects().contains(prerequisite_object)) {
                continue;
            }
            std::vector<Action *> new_stack = stack;
            new_stack.push_back(prerequisite_action);
            std::unordered_set<types::Object *> loop = FindLoop(actions, new_stack);
            if (!loop.empty()) {
                return loop;
            }
        }
    }
    
    return {};
}

bool PackageHandler::ExecuteActions(std::vector<Action *> ordered_actions) {
    for (Action *action : ordered_actions) {
        if (!action->execute()) {
            return false;
        }
    }
    return true;
}

}
}
