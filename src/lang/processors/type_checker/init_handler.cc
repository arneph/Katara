//
//  init_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "init_handler.h"

#include "lang/representation/ast/ast_util.h"

namespace lang {
namespace type_checker {

void InitHandler::HandleInits(std::vector<ast::File *> package_files,
                              types::Package *package,
                              types::TypeInfo *info,
                              std::vector<issues::Issue> &issues) {
    InitHandler handler(package_files,
                        package,
                        info,
                        issues);
    
    handler.FindInitOrder();
}

void InitHandler::FindInitOrder() {
    std::map<types::Variable *, types::Initializer *> initializers;
    std::map<types::Object *, std::unordered_set<types::Object *>> dependencies;
    
    FindInitializersAndDependencies(initializers, dependencies);
    
    std::unordered_set<types::Variable *> done_vars;
    std::unordered_set<types::Object *> done_objs;
    while (initializers.size() > done_vars.size()) {
        size_t done_objs_size_before = done_objs.size();
        
        for (auto& [l, r] : dependencies) {
            if (done_objs.find(l) != done_objs.end()) {
                continue;
            }
            bool all_dependencies_done = true;
            for (auto dependency : r) {
                if (done_objs.find(dependency) == done_objs.end()) {
                    all_dependencies_done = false;
                    break;
                }
            }
            if (!all_dependencies_done) {
                continue;
            }
            
            auto var = dynamic_cast<types::Variable *>(l);
            if (var == nullptr) {
                done_objs.insert(l);
                continue;
            }
            auto it = initializers.find(var);
            if (it == initializers.end()) {
                done_objs.insert(var);
                continue;
            }
            types::Initializer *initializer = it->second;
            
            info_->init_order_.push_back(initializer);
            
            for (auto var : initializer->lhs_) {
                done_vars.insert(var);
                done_objs.insert(var);
            }
        }
        
        size_t done_objs_size_after = done_objs.size();
        if (done_objs_size_before == done_objs_size_after) {
            std::vector<pos::pos_t> positions;
            std::string names = "";
            for (auto [var, _] : initializers) {
                if (done_vars.find(var) != done_vars.end()) {
                    continue;
                }
                positions.push_back(var->position_);
                if (names.empty()) {
                    names = var->name_;
                } else {
                    names += ", " + var->name_;
                }
            }
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            positions,
                                            "initialization loop(s) for variables: " + names));
            break;
        }
    }
}

void InitHandler::FindInitializersAndDependencies(
                                                  std::map<types::Variable *, types::Initializer *>& initializers,
                                                  std::map<types::Object *, std::unordered_set<types::Object *>>& dependencies) {
    for (ast::File *file : package_files_) {
        for (auto& decl : file->decls_) {
            if (auto gen_decl = dynamic_cast<ast::GenDecl *>(decl.get())) {
                for (auto& spec : gen_decl->specs_) {
                    auto value_spec = dynamic_cast<ast::ValueSpec *>(spec.get());
                    if (value_spec == nullptr) {
                        continue;
                    }
                    
                    std::vector<types::Object *> lhs_objects;
                    std::vector<std::unordered_set<types::Object *>> rhs_objects;
                    
                    for (size_t i = 0; i < value_spec->names_.size(); i++) {
                        lhs_objects.push_back(info_->definitions_.at(value_spec->names_.at(i).get()));
                    }
                    for (auto& expr : value_spec->values_) {
                        rhs_objects.push_back(FindInitDependenciesOfNode(expr.get()));
                    }
                    
                    for (size_t i = 0; i < lhs_objects.size(); i++) {
                        types::Object *l = lhs_objects.at(i);
                        std::unordered_set<types::Object *> r;
                        if (rhs_objects.size() == 1) {
                            r = rhs_objects.at(0);
                        } else if (rhs_objects.size() == lhs_objects.size()) {
                            r = rhs_objects.at(i);
                        }
                        dependencies.insert({l, r});
                    }
                    if (gen_decl->tok_ != tokens::kVar ||
                        value_spec->values_.empty()) {
                        continue;
                    }
                    
                    for (size_t i = 0; i < value_spec->values_.size(); i++) {
                        auto initializer =
                        std::unique_ptr<types::Initializer>(new types::Initializer());
                        
                        auto initializer_ptr = initializer.get();
                        info_->initializer_unique_ptrs_.push_back(std::move(initializer));
                        
                        if (value_spec->values_.size() == 1) {
                            for (auto obj : lhs_objects) {
                                auto var = static_cast<types::Variable *>(obj);
                                initializer_ptr->lhs_.push_back(var);
                                initializers.insert({var, initializer_ptr});
                            }
                        } else {
                            auto var = static_cast<types::Variable *>(lhs_objects.at(i));
                            initializer_ptr->lhs_.push_back(var);
                            initializers.insert({var, initializer_ptr});
                        }
                        initializer_ptr->rhs_ = value_spec->values_.at(i).get();
                    }
                }
            } else if (auto func_decl = dynamic_cast<ast::FuncDecl *>(decl.get())) {
                types::Object *f = info_->definitions_.at(func_decl->name_.get());
                std::unordered_set<types::Object *> r =
                FindInitDependenciesOfNode(func_decl->body_.get());
                dependencies.insert({f, r});
            } else {
                throw "unexpected declaration";
            }
        }
    }
}

std::unordered_set<types::Object *> InitHandler::FindInitDependenciesOfNode(ast::Node *node) {
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
        auto it = info_->uses_.find(ident);
        if (it == info_->uses_.end() ||
            it->second->parent() != package_->scope() ||
            (dynamic_cast<types::Constant *>(it->second) == nullptr &&
             dynamic_cast<types::Variable *>(it->second) == nullptr &&
             dynamic_cast<types::Func *>(it->second) == nullptr)) {
            return walker;
        }
        objects.insert(it->second);
        return walker;
    });
    ast::Walk(node, walker);
    return objects;
}

}
}
