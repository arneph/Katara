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
                                    types::TypeInfo *type_info,
                                    std::vector<issues::Issue> &issues) {
    PackageHandler manager(package_files, package, type_info, issues);
    
    manager.FindActions();
    
    std::vector<Action *> ordered_actions = manager.FindActionOrder();
    
    return manager.ExecuteActions(ordered_actions);
}

PackageHandler::PackageHandler(std::vector<ast::File *> package_files,
                                           types::Package *package,
                                           types::TypeInfo *type_info,
                                           std::vector<issues::Issue> &issues)
    : package_files_(package_files), package_(package), type_info_(type_info), issues_(issues) {}

void PackageHandler::FindActions() {
    for (ast::File *file : package_files_) {
        for (auto& decl : file->decls_) {
            if (auto gen_decl = dynamic_cast<ast::GenDecl *>(decl.get())) {
                switch (gen_decl->tok_) {
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
            } else if (auto func_decl = dynamic_cast<ast::FuncDecl *>(decl.get())) {
                FindActionsForFuncDecl(func_decl);
            }
        }
    }
}

void PackageHandler::FindActionsForTypeDecl(ast::GenDecl *type_decl) {
    for (auto& spec : type_decl->specs_) {
        ast::TypeSpec *type_spec = static_cast<ast::TypeSpec *>(spec.get());
        types::TypeName *type_name =
            static_cast<types::TypeName *>(type_info_->definitions().at(type_spec->name_.get()));
        
        std::unordered_set<types::Object *> prerequisites = FindPrerequisites(type_spec);
        prerequisites.erase(type_name);
        for (types::Object *prerequisite : prerequisites) {
            if (dynamic_cast<types::TypeName *>(prerequisite) == nullptr &&
                dynamic_cast<types::Constant *>(prerequisite) == nullptr) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                {type_name->position(), prerequisite->position()},
                                                "type can only depend on types and constants"));
            }
        }
        
        std::unique_ptr<Action> action =
            std::make_unique<Action>(prerequisites,
                                     type_name,
                                     [=]() -> bool {
                return TypeHandler::ProcessTypeName(type_name,
                                                    type_spec,
                                                    type_info_,
                                                    issues_);
        });
        
        Action *action_ptr = action.get();
        actions_.push_back(std::move(action));
        const_and_type_actions_.push_back(action_ptr);
    }
}

void PackageHandler::FindActionsForConstDecl(ast::GenDecl *const_decl) {
    int64_t iota = 0;
    for (auto& spec : const_decl->specs_) {
        ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec.get());
        
        for (size_t i = 0; i < value_spec->names_.size(); i++) {
            ast::Ident *name = value_spec->names_.at(i).get();
            types::Constant *constant =
                static_cast<types::Constant *>(type_info_->definitions().at(name));
            
            ast::Expr *type_expr = nullptr;
            ast::Expr *value = nullptr;
            std::unordered_set<types::Object *> prerequisites;
            if (value_spec->type_) {
                type_expr = value_spec->type_.get();
                std::unordered_set<types::Object *> type_prerequisites =
                    FindPrerequisites(type_expr);
                prerequisites.insert(type_prerequisites.begin(),
                                     type_prerequisites.end());
            }
            if (value_spec->values_.size() > i) {
                value = value_spec->values_.at(i).get();
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
            
            std::unique_ptr<Action> action =
                std::make_unique<Action>(prerequisites,
                                         constant,
                                         [=]() -> bool {
                    types::Type *type = nullptr;
                    if (type_expr != nullptr) {
                        if (!TypeHandler::ProcessTypeExpr(type_expr, type_info_, issues_)) {
                            return false;
                        }
                        type = type_info_->TypeOf(type_expr);
                    }
                    
                    return ConstantHandler::ProcessConstant(constant,
                                                            type,
                                                            value,
                                                            iota,
                                                            type_info_,
                                                            issues_);
            });
            
            Action *action_ptr = action.get();
            actions_.push_back(std::move(action));
            const_and_type_actions_.push_back(action_ptr);
        }
        iota++;
    }
}

void PackageHandler::FindActionsForVarDecl(ast::GenDecl *var_decl) {
    for (auto& spec : var_decl->specs_) {
        ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec.get());
        
        ast::Expr *type_expr = nullptr;
        std::unordered_set<types::Object *> type_prerequisites;
        if (value_spec->type_) {
            type_expr = value_spec->type_.get();
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
        
        if (value_spec->names_.size() > 1 && value_spec->values_.size() == 1) {
            std::vector<types::Variable *> variables;
            std::unordered_set<types::Object *> objects;
            for (auto& name : value_spec->names_) {
                types::Variable *variable =
                    static_cast<types::Variable *>(type_info_->definitions().at(name.get()));
                
                variables.push_back(variable);
                objects.insert(variable);
            }
            
            ast::Expr *value = value_spec->values_.at(0).get();
            std::unordered_set<types::Object *> prerequisites = FindPrerequisites(value);
            prerequisites.insert(type_prerequisites.begin(),
                                 type_prerequisites.end());
            
            std::unique_ptr<Action> action =
                std::make_unique<Action>(prerequisites,
                                         objects,
                                         [=]() -> bool {
                    types::Type *type = nullptr;
                    if (type_expr != nullptr) {
                        if (!TypeHandler::ProcessTypeExpr(type_expr, type_info_, issues_)) {
                            return false;
                        }
                        type = type_info_->TypeOf(type_expr);
                    }
                    
                    return VariableHandler::ProcessVariables(variables,
                                                             type,
                                                             value,
                                                             type_info_,
                                                             issues_);
            });
            
            Action *action_ptr = action.get();
            actions_.push_back(std::move(action));
            variable_and_func_decl_actions_.push_back(action_ptr);
            
        } else {
            for (size_t i = 0; i < value_spec->names_.size(); i++) {
                ast::Ident *name = value_spec->names_.at(i).get();
                types::Variable *variable =
                    static_cast<types::Variable *>(type_info_->definitions().at(name));
                
                ast::Expr *value = nullptr;
                std::unordered_set<types::Object *> prerequisites;
                prerequisites.insert(type_prerequisites.begin(),
                                     type_prerequisites.end());
                if (value_spec->values_.size() > i) {
                    value = value_spec->values_.at(i).get();
                    std::unordered_set<types::Object *> value_prerequisites
                        = FindPrerequisites(value);
                    prerequisites.insert(value_prerequisites.begin(),
                                         value_prerequisites.end());
                }
                
                std::unique_ptr<Action> action =
                    std::make_unique<Action>(prerequisites,
                                             variable,
                                             [=]() -> bool {
                        types::Type *type = nullptr;
                        if (type_expr != nullptr) {
                            if (!TypeHandler::ProcessTypeExpr(type_expr, type_info_, issues_)) {
                                return false;
                            }
                            type = type_info_->TypeOf(type_expr);
                        }
                        
                        return VariableHandler::ProcessVariable(variable,
                                                                type,
                                                                value,
                                                                type_info_,
                                                                issues_);
                });
                
                Action *action_ptr = action.get();
                actions_.push_back(std::move(action));
                variable_and_func_decl_actions_.push_back(action_ptr);
            }
        }
    }
}

void PackageHandler::FindActionsForFuncDecl(ast::FuncDecl *func_decl) {
    ast::Ident *name = func_decl->name_.get();
    ast::BlockStmt *body = func_decl->body_.get();
    types::Func *func = static_cast<types::Func *>(type_info_->definitions().at(name));
    
    std::unordered_set<types::Object *> prerequisites = FindPrerequisites(func_decl);
    
    std::unique_ptr<Action> decl_action =
    std::make_unique<Action>(prerequisites,
                             func,
                             [=]() -> bool {
        return TypeHandler::ProcessFuncDecl(func,
                                            func_decl,
                                            type_info_,
                                            issues_);
    });
    
    Action *decl_action_ptr = decl_action.get();
    actions_.push_back(std::move(decl_action));
    variable_and_func_decl_actions_.push_back(decl_action_ptr);
    
    std::unique_ptr<Action> body_action =
    std::make_unique<Action>(prerequisites,
                             func,
                             [=]() -> bool {
        types::Signature *signature = static_cast<types::Signature *>(func->type());
        StmtHandler::ProcessFuncBody(body,
                                     signature->results(),
                                     type_info_,
                                     issues_);
        return true;
    });
    
    Action *body_action_ptr = body_action.get();
    actions_.push_back(std::move(body_action));
    func_body_actions_.push_back(body_action_ptr);
}

std::unordered_set<types::Object *> PackageHandler::FindPrerequisites(ast::Node *node) {
    std::unordered_set<types::Object *> objects;
    ast::WalkFunction walker =
    ast::WalkFunction([&](ast::Node *node) -> ast::WalkFunction {
        if (node == nullptr) {
            return walker;
        }
        auto ident = dynamic_cast<ast::Ident *>(node);
        if (ident == nullptr) {
            return walker;
        }
        auto it = type_info_->uses().find(ident);
        if (it == type_info_->uses().end() ||
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
