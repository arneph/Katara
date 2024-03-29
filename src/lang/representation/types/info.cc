//
//  info.cc
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "info.h"

#include "src/lang/representation/types/info_builder.h"

namespace lang {
namespace types {

Object* Info::ObjectOf(ast::Ident* ident) const {
  auto defs_it = definitions_.find(ident);
  if (defs_it != definitions_.end()) {
    return defs_it->second;
  }
  auto uses_it = uses_.find(ident);
  if (uses_it != uses_.end()) {
    return uses_it->second;
  }
  return nullptr;
}

Object* Info::DefinitionOf(ast::Ident* ident) const {
  auto it = definitions_.find(ident);
  if (it != definitions_.end()) {
    return it->second;
  }
  return nullptr;
}

Object* Info::UseOf(ast::Ident* ident) const {
  auto it = uses_.find(ident);
  if (it != uses_.end()) {
    return it->second;
  }
  return nullptr;
}

Object* Info::ImplicitOf(ast::Node* node) const {
  auto it = implicits_.find(node);
  if (it != implicits_.end()) {
    return it->second;
  }
  return nullptr;
}

Scope* Info::ScopeOf(ast::Node* node) const {
  auto it = scopes_.find(node);
  if (it != scopes_.end()) {
    return it->second;
  }
  return nullptr;
}

std::optional<ExprInfo> Info::ExprInfoOf(ast::Expr* expr) const {
  auto it = expr_infos_.find(expr);
  if (it != expr_infos_.end()) {
    return it->second;
  }
  return std::nullopt;
}

Type* Info::TypeOf(ast::Expr* expr) const {
  auto it = expr_infos_.find(expr);
  if (it != expr_infos_.end()) {
    return it->second.type();
  }
  if (expr->node_kind() == ast::NodeKind::kIdent) {
    Object* obj = ObjectOf(static_cast<ast::Ident*>(expr));
    if (obj->is_typed()) {
      return static_cast<types::TypedObject*>(obj)->type();
    }
  }
  return nullptr;
}

InfoBuilder Info::builder() { return InfoBuilder(this); }

}  // namespace types
}  // namespace lang
