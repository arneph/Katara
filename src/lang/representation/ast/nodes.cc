//
//  nodes.cc
//  Katara
//
//  Created by Arne Philipeit on 5/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "nodes.h"

#include "src/common/logging/logging.h"

namespace lang {
namespace ast {

using ::common::logging::fail;
using ::common::positions::kNoPos;
using ::common::positions::pos_t;

bool Node::is_decl() const {
  NodeKind kind = node_kind();
  return NodeKind::kDeclStart <= kind && kind <= NodeKind::kDeclEnd;
}

bool Node::is_spec() const {
  NodeKind kind = node_kind();
  return NodeKind::kSpecStart <= kind && kind <= NodeKind::kSpecEnd;
}

bool Node::is_stmt() const {
  NodeKind kind = node_kind();
  return NodeKind::kStmtStart <= kind && kind <= NodeKind::kStmtEnd;
}

bool Node::is_expr() const {
  NodeKind kind = node_kind();
  return NodeKind::kExprStart <= kind && kind <= NodeKind::kExprEnd;
}

pos_t GenDecl::end() const {
  if (r_paren_ != kNoPos) {
    return r_paren_;
  }
  return specs_.back()->end();
}

pos_t ImportSpec::start() const {
  if (name_) {
    return name_->start();
  }
  return path_->start();
}

pos_t ImportSpec::end() const { return path_->end(); }

pos_t ValueSpec::start() const { return names_.front()->start(); }

pos_t ValueSpec::end() const {
  if (!values_.empty()) {
    return values_.back()->end();
  }
  if (type_) {
    return type_->end();
  }
  return names_.back()->end();
}

pos_t TypeSpec::start() const { return name_->start(); }

pos_t TypeSpec::end() const { return type_->end(); }

ExprReceiver* FuncDecl::expr_receiver() const {
  if (kind_ != Kind::kInstanceMethod) {
    fail("attempted to access expr receiver of non-instance-method func");
  }
  return expr_receiver_;
}

TypeReceiver* FuncDecl::type_receiver() const {
  if (kind_ != Kind::kTypeMethod) {
    fail("attempted to access type receiver of non-type-method func");
  }
  return type_receiver_;
}

pos_t FuncDecl::start() const { return func_type_->start(); }

pos_t FuncDecl::end() const { return body_->end(); }

pos_t DeclStmt::start() const { return decl_->start(); }

pos_t DeclStmt::end() const { return decl_->end(); }

pos_t IfStmt::end() const {
  if (else_) {
    return else_->end();
  }
  return body_->end();
}

pos_t ForStmt::end() const { return body_->end(); }

pos_t ExprSwitchStmt::end() const { return body_->end(); }

pos_t TypeSwitchStmt::end() const { return body_->end(); }

pos_t LabeledStmt::start() const { return label_->start(); }

pos_t BranchStmt::end() const {
  if (label_) {
    return label_->end();
  }
  switch (tok_) {
    case tokens::kFallthrough:
      return tok_start_ + 10;
    case tokens::kContinue:
      return tok_start_ + 7;
    case tokens::kBreak:
      return tok_start_ + 4;
    default:
      fail("unexpected ast::BranchStmt token");
  }
}

pos_t SelectionExpr::end() const { return selection_->end(); }

pos_t FuncLit::start() const { return type_->start(); }

pos_t FuncLit::end() const { return body_->end(); }

pos_t FuncType::end() const {
  if (results_ != nullptr) {
    return results_->end();
  }
  return params_->end();
}

pos_t MethodSpec::end() const {
  if (!results_) {
    return params_->end();
  }
  return results_->end();
}

pos_t FieldList::start() const {
  if (l_paren_ != kNoPos) {
    return l_paren_;
  }
  return fields_.front()->start();
}

pos_t FieldList::end() const {
  if (r_paren_ != kNoPos) {
    return r_paren_;
  }
  return fields_.back()->end();
}

pos_t Field::start() const {
  if (!names_.empty()) {
    return names_.front()->start();
  }
  return type_->start();
}

pos_t TypeParam::start() const { return name_->start(); }

pos_t TypeParam::end() const {
  if (!type_) {
    return name_->end();
  }
  return type_->start();
}

}  // namespace ast
}  // namespace lang
