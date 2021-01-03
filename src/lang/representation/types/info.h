//
//  info.h
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_type_info_h
#define lang_types_type_info_h

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "lang/representation/constants/constants.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/scope.h"
#include "lang/representation/types/package.h"
#include "lang/representation/types/expr_info.h"
#include "lang/representation/types/selection.h"
#include "lang/representation/types/initializer.h"

namespace lang {
namespace types {

class InfoBuilder;

class Info {
public:
    const std::unordered_map<ast::Expr *, ExprInfo>& expr_infos() const;
    const std::unordered_map<ast::Expr *, constants::Value>& constant_values() const;
    
    const std::unordered_map<ast::Ident *, Object *>& definitions() const;
    const std::unordered_map<ast::Ident *, Object *>& uses() const;
    const std::unordered_map<ast::Node *, Object *>& implicits() const;
    
    const std::unordered_map<ast::SelectionExpr *, Selection>& selections() const;
    
    const std::unordered_map<ast::Node *, Scope *>& scopes() const;
    const std::unordered_set<Package *>& packages() const;
    
    const std::vector<Initializer>& init_order() const;
    
    Scope * universe() const;
    Basic * basic_type(Basic::Kind kind);
    
    Object * ObjectOf(ast::Ident *ident) const;
    Object * DefinitionOf(ast::Ident *ident) const;
    Object * UseOf(ast::Ident *ident) const;
    Object * ImplicitOf(ast::Node *node) const;
    
    Scope * ScopeOf(ast::Node *node) const;
    
    std::optional<ExprInfo> ExprInfoOf(ast::Expr *expr) const;
    std::optional<constants::Value> ConstantValueOf(ast::Expr *expr) const;
    
    Type * TypeOf(ast::Expr *expr) const;
    
    InfoBuilder builder();
    
private:
    std::vector<std::unique_ptr<Type>> type_unique_ptrs_;
    std::vector<std::unique_ptr<Object>> object_unique_ptrs_;
    std::vector<std::unique_ptr<Scope>> scope_unique_ptrs_;
    std::vector<std::unique_ptr<Package>> package_unique_ptrs_;
    
    std::unordered_map<ast::Expr *, ExprInfo> expr_infos_;
    std::unordered_map<ast::Expr *, constants::Value> constant_values_;
    
    std::unordered_map<ast::Ident *, Object *> definitions_;
    std::unordered_map<ast::Ident *, Object *> uses_;
    std::unordered_map<ast::Node *, Object *> implicits_;
    
    std::unordered_map<ast::SelectionExpr *, Selection> selections_;
    
    std::unordered_map<ast::Node *, Scope *> scopes_;
    std::unordered_set<Package *> packages_;
    
    std::vector<Initializer> init_order_;
    
    Scope *universe_;
    std::unordered_map<Basic::Kind, Basic*> basic_types_;
    
    friend InfoBuilder;
};

}
}

#endif /* lang_types_type_info_h */
