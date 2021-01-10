//
//  nodes.h
//  Katara
//
//  Created by Arne Philipeit on 5/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_ast_nodes_h
#define lang_ast_nodes_h

#include <map>
#include <string>
#include <vector>

#include "lang/representation/positions/positions.h"
#include "lang/representation/tokens/tokens.h"

namespace lang {
namespace ast {

enum class NodeKind {
  kFile,

  kGenDecl,
  kFuncDecl,
  kDeclStart = kGenDecl,
  kDeclEnd = kFuncDecl,

  kImportSpec,
  kValueSpec,
  kTypeSpec,
  kSpecStart = kImportSpec,
  kSpecEnd = kTypeSpec,

  kBlockStmt,
  kDeclStmt,
  kAssignStmt,
  kExprStmt,
  kIncDecStmt,
  kReturnStmt,
  kIfStmt,
  kExprSwitchStmt,
  kTypeSwitchStmt,
  kCaseClause,
  kForStmt,
  kLabeledStmt,
  kBranchStmt,
  kStmtStart = kBlockStmt,
  kStmtEnd = kBranchStmt,

  kUnaryExpr,
  kBinaryExpr,
  kCompareExpr,
  kParenExpr,
  kSelectionExpr,
  kTypeAssertExpr,
  kIndexExpr,
  kCallExpr,
  kFuncLit,
  kCompositeLit,
  kKeyValueExpr,
  kArrayType,
  kFuncType,
  kInterfaceType,
  kStructType,
  kTypeInstance,
  kBasicLit,
  kIdent,
  kExprStart = kUnaryExpr,
  kExprEnd = kIdent,

  kMethodSpec,
  kExprReceiver,
  kTypeReceiver,
  kFieldList,
  kField,
  kTypeParamList,
  kTypeParam,
};

class Node {
 public:
  virtual ~Node() {}

  bool is_decl() const;
  bool is_spec() const;
  bool is_stmt() const;
  bool is_expr() const;

  virtual NodeKind node_kind() const = 0;
  virtual pos::pos_t start() const = 0;
  virtual pos::pos_t end() const = 0;
};

// Decl ::= GenDecl | FuncDecl .
class Decl : public Node {
 public:
  virtual ~Decl() {}
};

// Stmt ::= BlockStmt
//        | DeclStmt
//        | AssignStmt
//        | ExprStmt
//        | IncDecStmt
//        | ReturnStmt
//        | IfStmt
//        | SwitchStmt
//        | CaseClause
//        | ForStmt
//        | LabeledStmt
//        | BranchStmt .
class Stmt : public Node {
 public:
  virtual ~Stmt() {}
};

// Expr ::= UnaryExpr
//        | BinaryExpr
//        | CompareExpr
//        | ParenExpr
//        | SelectionExpr
//        | TypeAssertExpr
//        | IndexExpr
//        | CallExpr
//        | FuncLit
//        | CompositeLit
//        | KeyValueExpr
//        | ArrayType
//        | FuncType
//        | InterfaceType
//        | StructType
//        | PointerType
//        | TypeInstance
//        | BasicLit
//        | Ident .
class Expr : public Node {
 public:
  virtual ~Expr() {}
};

class File;

class GenDecl;
class Spec;
class ValueSpec;
class TypeSpec;
class FuncDecl;

class BlockStmt;
class DeclStmt;
class AssignStmt;
class ExprStmt;
class IncDecStmt;
class ReturnStmt;
class IfStmt;
class ExprSwitchStmt;
class TypeSwitchStmt;
class CaseClause;
class ForStmt;
class LabeledStmt;
class BranchStmt;

class UnaryExpr;
class BinaryExpr;
class CompareExpr;
class ParenExpr;
class SelectionExpr;
class TypeAssertExpr;
class IndexExpr;
class CallExpr;
class FuncLit;
class CompositeLit;
class KeyValueExpr;

class ArrayType;
class FuncType;
class InterfaceType;
class MethodSpec;
class StructType;
class TypeInstance;

class ExprReceiver;
class TypeReceiver;
class FieldList;
class Field;
class TypeParamList;
class TypeParam;

class BasicLit;
class Ident;

// File ::= {Decl} .
class File final : public Node {
 public:
  Ident* package_name() const { return package_name_; }
  std::vector<Decl*> decls() const { return decls_; }

  NodeKind node_kind() const override { return NodeKind::kFile; }
  pos::pos_t start() const override { return start_; }
  pos::pos_t end() const override { return end_; }

 private:
  File(pos::pos_t start, pos::pos_t end, Ident* package_name, std::vector<Decl*> decls)
      : start_(start), end_(end), package_name_(package_name), decls_(decls) {}

  pos::pos_t start_;
  pos::pos_t end_;
  Ident* package_name_;
  std::vector<Decl*> decls_;

  friend class ASTBuilder;
};

// GenDecl ::= ("import" (ImportSpec | "(" {ImportSpec ";"} ")" )
//           | ("const" (ValueSpec | "(" {ValueSpec} ")" )
//           | ("var" (ValueSpec | "(" {ValueSpec} ")" )
//           | ("type" (TypeSpec | "( {TypeSpec} ")" ) .
class GenDecl final : public Decl {
 public:
  tokens::Token tok() { return tok_; }
  pos::pos_t l_paren() { return l_paren_; }
  std::vector<Spec*> specs() { return specs_; }
  pos::pos_t r_paren() { return r_paren_; }

  NodeKind node_kind() const override { return NodeKind::kGenDecl; }
  pos::pos_t start() const override { return tok_start_; }
  pos::pos_t end() const override;

 private:
  GenDecl(pos::pos_t tok_start, tokens::Token tok, pos::pos_t l_paren, std::vector<Spec*> specs,
          pos::pos_t r_paren)
      : tok_start_(tok_start), tok_(tok), l_paren_(l_paren), specs_(specs), r_paren_(r_paren) {}

  pos::pos_t tok_start_;
  tokens::Token tok_;
  pos::pos_t l_paren_;
  std::vector<Spec*> specs_;
  pos::pos_t r_paren_;

  friend class ASTBuilder;
};

// Spec ::= ImportSpec | ValueSpec | TypeSpec .
class Spec : public Node {
 public:
  virtual ~Spec() {}
};

// ImportSpec ::= [Ident] BasicLit .
class ImportSpec final : public Spec {
 public:
  Ident* name() { return name_; }
  BasicLit* path() { return path_; }

  NodeKind node_kind() const override { return NodeKind::kImportSpec; }
  pos::pos_t start() const override;
  pos::pos_t end() const override;

 private:
  ImportSpec(Ident* name, BasicLit* path) : name_(name), path_(path) {}

  Ident* name_;
  BasicLit* path_;

  friend class ASTBuilder;
};

// ValueSpec ::= Ident {"," Ident} [Type] ["=" Expr {"," Expr}] "\n" .
class ValueSpec final : public Spec {
 public:
  std::vector<Ident*> names() const { return names_; }
  Expr* type() const { return type_; }
  std::vector<Expr*> values() const { return values_; }

  NodeKind node_kind() const override { return NodeKind::kValueSpec; }
  pos::pos_t start() const override;
  pos::pos_t end() const override;

 private:
  ValueSpec(std::vector<Ident*> names, Expr* type, std::vector<Expr*> values)
      : names_(names), type_(type), values_(values) {}

  std::vector<Ident*> names_;
  Expr* type_;
  std::vector<Expr*> values_;

  friend class ASTBuilder;
};

// TypeSpec ::= Ident [TypeParamList] ["="] Type "\n" .
class TypeSpec final : public Spec {
 public:
  Ident* name() const { return name_; }
  TypeParamList* type_params() const { return type_params_; }
  pos::pos_t assign() const { return assign_; }
  Expr* type() const { return type_; }

  NodeKind node_kind() const override { return NodeKind::kTypeSpec; }
  pos::pos_t start() const override;
  pos::pos_t end() const override;

 private:
  TypeSpec(Ident* name, TypeParamList* type_params, pos::pos_t assign, Expr* type)
      : name_(name), type_params_(type_params), assign_(assign), type_(type) {}

  Ident* name_;
  TypeParamList* type_params_;
  pos::pos_t assign_;
  Expr* type_;

  friend class ASTBuilder;
};

// FuncDecl ::= "func" [ExprReceiver | TypeReceiver]
//              Ident [TypeParamList] FieldList [FieldList] BlockStmt .
class FuncDecl final : public Decl {
 public:
  enum class Kind {
    kFunc,
    kInstanceMethod,
    kTypeMethod,
  };

  Kind kind() const { return kind_; }
  ExprReceiver* expr_receiver() const;
  TypeReceiver* type_receiver() const;
  Ident* name() const { return name_; }
  TypeParamList* type_params() const { return type_params_; }
  FuncType* func_type() const { return func_type_; }
  BlockStmt* body() const { return body_; }

  NodeKind node_kind() const override { return NodeKind::kFuncDecl; }
  pos::pos_t start() const override;
  pos::pos_t end() const override;

 private:
  FuncDecl(Ident* name, TypeParamList* type_params, FuncType* func_type, BlockStmt* body)
      : kind_(Kind::kFunc),
        expr_receiver_(nullptr),
        type_receiver_(nullptr),
        name_(name),
        type_params_(type_params),
        func_type_(func_type),
        body_(body) {}

  FuncDecl(ExprReceiver* expr_receiver, Ident* name, TypeParamList* type_params,
           FuncType* func_type, BlockStmt* body)
      : kind_(Kind::kInstanceMethod),
        expr_receiver_(expr_receiver),
        type_receiver_(nullptr),
        name_(name),
        type_params_(type_params),
        func_type_(func_type),
        body_(body) {}

  FuncDecl(TypeReceiver* type_receiver, Ident* name, TypeParamList* type_params,
           FuncType* func_type, BlockStmt* body)
      : kind_(Kind::kTypeMethod),
        expr_receiver_(nullptr),
        type_receiver_(type_receiver),
        name_(name),
        type_params_(type_params),
        func_type_(func_type),
        body_(body) {}

  Kind kind_;
  ExprReceiver* expr_receiver_;
  TypeReceiver* type_receiver_;
  Ident* name_;
  TypeParamList* type_params_;
  FuncType* func_type_;
  BlockStmt* body_;

  friend class ASTBuilder;
};

// BlockStmt ::= "{" {Stmt} "}" .
class BlockStmt final : public Stmt {
 public:
  std::vector<Stmt*> stmts() const { return stmts_; }

  NodeKind node_kind() const override { return NodeKind::kBlockStmt; }
  pos::pos_t start() const override { return l_brace_; }
  pos::pos_t end() const override { return r_brace_; }

 private:
  BlockStmt(pos::pos_t l_brace, std::vector<Stmt*> stmts, pos::pos_t r_brace)
      : l_brace_(l_brace), stmts_(stmts), r_brace_(r_brace) {}

  pos::pos_t l_brace_;
  std::vector<Stmt*> stmts_;
  pos::pos_t r_brace_;

  friend class ASTBuilder;
};

// DeclStmt ::= GenDecl .
class DeclStmt final : public Stmt {
 public:
  GenDecl* decl() const { return decl_; }

  NodeKind node_kind() const override { return NodeKind::kDeclStmt; }
  pos::pos_t start() const override;
  pos::pos_t end() const override;

 private:
  DeclStmt(GenDecl* decl) : decl_(decl) {}

  GenDecl* decl_;

  friend class ASTBuilder;
};

// AssignStmt ::= Expr {"," Expr} AssignOp Expr {"," Expr} .
class AssignStmt final : public Stmt {
 public:
  std::vector<Expr*> lhs() const { return lhs_; }
  pos::pos_t tok_start() const { return tok_start_; }
  tokens::Token tok() const { return tok_; }
  std::vector<Expr*> rhs() const { return rhs_; }

  NodeKind node_kind() const override { return NodeKind::kAssignStmt; }
  pos::pos_t start() const override { return lhs_.front()->start(); }
  pos::pos_t end() const override { return rhs_.back()->end(); }

 private:
  AssignStmt(std::vector<Expr*> lhs, pos::pos_t tok_start, tokens::Token tok,
             std::vector<Expr*> rhs)
      : lhs_(lhs), tok_start_(tok_start), tok_(tok), rhs_(rhs) {}

  std::vector<Expr*> lhs_;
  pos::pos_t tok_start_;
  tokens::Token tok_;
  std::vector<Expr*> rhs_;

  friend class ASTBuilder;
};

// ExprStmt ::= Expr .
class ExprStmt final : public Stmt {
 public:
  Expr* x() const { return x_; }

  NodeKind node_kind() const override { return NodeKind::kExprStmt; }
  pos::pos_t start() const override { return x_->start(); }
  pos::pos_t end() const override { return x_->end(); }

 private:
  ExprStmt(Expr* x) : x_(x) {}

  Expr* x_;

  friend class ASTBuilder;
};

// IncDecStmt ::= Expr ("++" | "--") .
class IncDecStmt final : public Stmt {
 public:
  Expr* x() const { return x_; }
  pos::pos_t tok_start() const { return tok_start_; }
  tokens::Token tok() const { return tok_; }

  NodeKind node_kind() const override { return NodeKind::kIncDecStmt; }
  pos::pos_t start() const override { return x_->start(); }
  pos::pos_t end() const override { return tok_start_ + 1; }

 private:
  IncDecStmt(Expr* x, pos::pos_t tok_start, tokens::Token tok)
      : x_(x), tok_start_(tok_start), tok_(tok) {}

  Expr* x_;
  pos::pos_t tok_start_;
  tokens::Token tok_;

  friend class ASTBuilder;
};

// ReturnStmt ::= "return" [Expr {"," Expr}] .
class ReturnStmt final : public Stmt {
 public:
  std::vector<Expr*> results() const { return results_; }

  NodeKind node_kind() const override { return NodeKind::kReturnStmt; }
  pos::pos_t start() const override { return return_; }
  pos::pos_t end() const override {
    return (results_.empty()) ? return_ + 5 : results_.back()->end();
  }

 private:
  ReturnStmt(pos::pos_t return_start, std::vector<Expr*> results)
      : return_(return_start), results_(results) {}

  pos::pos_t return_;
  std::vector<Expr*> results_;

  friend class ASTBuilder;
};

// IfStmt ::= "if" [Stmt ";"] Expr BlockStmt
//            ["else" (BlockStmt | IfStmt)] .
class IfStmt final : public Stmt {
 public:
  Stmt* init_stmt() const { return init_; }
  Expr* cond_expr() const { return cond_; }
  BlockStmt* body() const { return body_; }
  Stmt* else_stmt() const { return else_; }

  NodeKind node_kind() const override { return NodeKind::kIfStmt; }
  pos::pos_t start() const override { return if_; }
  pos::pos_t end() const override;

 private:
  IfStmt(pos::pos_t if_start, Stmt* init, Expr* cond, BlockStmt* body, Stmt* else_stmt)
      : if_(if_start), init_(init), cond_(cond), body_(body), else_(else_stmt) {}

  pos::pos_t if_;
  Stmt* init_;
  Expr* cond_;
  BlockStmt* body_;
  Stmt* else_;

  friend class ASTBuilder;
};

// ExprSwitchStmt ::= "switch" [Stmt ";"] [Expr] BlockStmt .
class ExprSwitchStmt final : public Stmt {
 public:
  Stmt* init_stmt() { return init_; }
  Expr* tag_expr() { return tag_; }
  BlockStmt* body() { return body_; }

  NodeKind node_kind() const override { return NodeKind::kExprSwitchStmt; }
  pos::pos_t start() const override { return switch_; }
  pos::pos_t end() const override;

 private:
  ExprSwitchStmt(pos::pos_t switch_start, Stmt* init, Expr* tag, BlockStmt* body)
      : switch_(switch_start), init_(init), tag_(tag), body_(body) {}

  pos::pos_t switch_;
  Stmt* init_;
  Expr* tag_;
  BlockStmt* body_;

  friend class ASTBuilder;
};

// TypeSwitchStmt ::= "switch" [Ident ":="] Expr ".<type>" BlockStmt .
class TypeSwitchStmt final : public Stmt {
 public:
  Ident* var() const { return var_; }
  Expr* tag_expr() const { return tag_; }
  BlockStmt* body() const { return body_; }

  NodeKind node_kind() const override { return NodeKind::kTypeSwitchStmt; }
  pos::pos_t start() const override { return switch_; }
  pos::pos_t end() const override;

 private:
  TypeSwitchStmt(pos::pos_t switch_start, Ident* var, Expr* tag, BlockStmt* body)
      : switch_(switch_start), var_(var), tag_(tag), body_(body) {}

  pos::pos_t switch_;
  Ident* var_;
  Expr* tag_;
  BlockStmt* body_;

  friend class ASTBuilder;
};

// CaseClause ::= (("case" Expr {"," Expr}) | "default") ":" {Stmt} .
class CaseClause final : public Stmt {
 public:
  tokens::Token tok() const { return tok_; }
  std::vector<Expr*> cond_vals() const { return cond_vals_; }
  pos::pos_t colon() const { return colon_; }
  std::vector<Stmt*> body() const { return body_; }

  NodeKind node_kind() const override { return NodeKind::kCaseClause; }
  pos::pos_t start() const override { return tok_start_; }
  pos::pos_t end() const override { return (body_.empty()) ? colon_ : body_.back()->end(); }

 private:
  CaseClause(pos::pos_t tok_start, tokens::Token tok, std::vector<Expr*> cond_vals,
             pos::pos_t colon, std::vector<Stmt*> body)
      : tok_start_(tok_start), tok_(tok), cond_vals_(cond_vals), colon_(colon), body_(body) {}

  pos::pos_t tok_start_;
  tokens::Token tok_;
  std::vector<Expr*> cond_vals_;
  pos::pos_t colon_;
  std::vector<Stmt*> body_;

  friend class ASTBuilder;
};

// ForStmt ::= "for" [([Stmt] ";" Expr ";" [Stmt]) | Expr] BlockStmt .
class ForStmt final : public Stmt {
 public:
  Stmt* init_stmt() const { return init_; }
  Expr* cond_expr() const { return cond_; }
  Stmt* post_stmt() const { return post_; }
  BlockStmt* body() const { return body_; }

  NodeKind node_kind() const override { return NodeKind::kForStmt; }
  pos::pos_t start() const override { return for_; }
  pos::pos_t end() const override;

 private:
  ForStmt(pos::pos_t for_start, Stmt* init, Expr* cond, Stmt* post, BlockStmt* body)
      : for_(for_start), init_(init), cond_(cond), post_(post), body_(body) {}

  pos::pos_t for_;
  Stmt* init_;
  Expr* cond_;
  Stmt* post_;
  BlockStmt* body_;

  friend class ASTBuilder;
};

// LabeledStmt ::= Ident ":" Stmt .
class LabeledStmt final : public Stmt {
 public:
  Ident* label() const { return label_; }
  pos::pos_t colon() const { return colon_; }
  Stmt* stmt() const { return stmt_; }

  NodeKind node_kind() const override { return NodeKind::kLabeledStmt; }
  pos::pos_t start() const override;
  pos::pos_t end() const override { return stmt_->end(); }

 private:
  LabeledStmt(Ident* label, pos::pos_t colon, Stmt* stmt)
      : label_(label), colon_(colon), stmt_(stmt) {}

  Ident* label_;
  pos::pos_t colon_;
  Stmt* stmt_;

  friend class ASTBuilder;
};

// BrancStmt ::= "fallthrough"
//             | "continue" [Ident]
//             | "break" [Ident] .
class BranchStmt final : public Stmt {
 public:
  tokens::Token tok() const { return tok_; }
  Ident* label() const { return label_; }

  NodeKind node_kind() const override { return NodeKind::kBranchStmt; }
  pos::pos_t start() const override { return tok_start_; }
  pos::pos_t end() const override;

 private:
  BranchStmt(pos::pos_t tok_start, tokens::Token tok, Ident* label)
      : tok_start_(tok_start), tok_(tok), label_(label) {}

  pos::pos_t tok_start_;
  tokens::Token tok_;
  Ident* label_;

  friend class ASTBuilder;
};

// UnaryExpr ::= UnaryOp Expr .
class UnaryExpr final : public Expr {
 public:
  tokens::Token op() const { return op_; }
  Expr* x() const { return x_; }

  NodeKind node_kind() const override { return NodeKind::kUnaryExpr; }
  pos::pos_t start() const override { return op_start_; }
  pos::pos_t end() const override { return x_->end(); }

 private:
  UnaryExpr(pos::pos_t op_start, tokens::Token op, Expr* x) : op_start_(op_start), op_(op), x_(x) {}

  pos::pos_t op_start_;
  tokens::Token op_;
  Expr* x_;

  friend class ASTBuilder;
};

// BinaryOp ::= Expr BinaryOp Expr .
class BinaryExpr final : public Expr {
 public:
  Expr* x() const { return x_; }
  pos::pos_t op_start() const { return op_start_; }
  tokens::Token op() const { return op_; }
  Expr* y() const { return y_; }

  NodeKind node_kind() const override { return NodeKind::kBinaryExpr; }
  pos::pos_t start() const override { return x_->start(); }
  pos::pos_t end() const override { return y_->end(); }

 private:
  BinaryExpr(Expr* x, pos::pos_t op_start, tokens::Token op, Expr* y)
      : x_(x), op_start_(op_start), op_(op), y_(y) {}

  Expr* x_;
  pos::pos_t op_start_;
  tokens::Token op_;
  Expr* y_;

  friend class ASTBuilder;
};

// CompareExpr ::= Expr CompareOp Expr {CompareOp Expr} .
class CompareExpr final : public Expr {
 public:
  std::vector<Expr*> operands() const { return operands_; }
  std::vector<pos::pos_t> compare_op_starts() const { return compare_op_starts_; }
  std::vector<tokens::Token> compare_ops() const { return compare_ops_; }

  NodeKind node_kind() const override { return NodeKind::kCompareExpr; }
  pos::pos_t start() const override { return operands_.front()->start(); }
  pos::pos_t end() const override { return operands_.back()->end(); }

 private:
  CompareExpr(std::vector<Expr*> operands, std::vector<pos::pos_t> compare_op_starts,
              std::vector<tokens::Token> compare_ops)
      : operands_(operands), compare_ops_(compare_ops) {}

  std::vector<Expr*> operands_;
  std::vector<pos::pos_t> compare_op_starts_;
  std::vector<tokens::Token> compare_ops_;

  friend class ASTBuilder;
};

// ParenExpr ::= "(" Expr ") .
class ParenExpr final : public Expr {
 public:
  Expr* x() const { return x_; }

  NodeKind node_kind() const override { return NodeKind::kParenExpr; }
  pos::pos_t start() const override { return l_paren_; }
  pos::pos_t end() const override { return r_paren_; }

 private:
  ParenExpr(pos::pos_t l_paren, Expr* x, pos::pos_t r_paren)
      : l_paren_(l_paren), x_(x), r_paren_(r_paren) {}

  pos::pos_t l_paren_;
  Expr* x_;
  pos::pos_t r_paren_;

  friend class ASTBuilder;
};

// SelectionExpr ::= Expr "." Ident .
class SelectionExpr final : public Expr {
 public:
  Expr* accessed() const { return accessed_; }
  Ident* selection() const { return selection_; }

  NodeKind node_kind() const override { return NodeKind::kSelectionExpr; }
  pos::pos_t start() const override { return accessed_->start(); }
  pos::pos_t end() const override;

 private:
  SelectionExpr(Expr* accessed, Ident* selection) : accessed_(accessed), selection_(selection) {}

  Expr* accessed_;
  Ident* selection_;

  friend class ASTBuilder;
};

// TypeAssertExpr ::= Expr "." "<" Type ">" .
class TypeAssertExpr final : public Expr {
 public:
  Expr* x() const { return x_; }
  pos::pos_t l_angle() const { return l_angle_; }
  Expr* type() const { return type_; }
  pos::pos_t r_angle() const { return r_angle_; }

  NodeKind node_kind() const override { return NodeKind::kTypeAssertExpr; }
  pos::pos_t start() const override { return x_->start(); }
  pos::pos_t end() const override { return r_angle_; }

 private:
  TypeAssertExpr(Expr* x, pos::pos_t l_angle, Expr* type, pos::pos_t r_angle)
      : x_(x), l_angle_(l_angle), type_(type), r_angle_(r_angle) {}

  Expr* x_;
  pos::pos_t l_angle_;
  Expr* type_;  // nullptr for "type" keyword in type switch
  pos::pos_t r_angle_;

  friend class ASTBuilder;
};

// IndexExpr ::= Expr "[" Expr "]" .
class IndexExpr final : public Expr {
 public:
  Expr* accessed() const { return accessed_; }
  pos::pos_t l_brack() const { return l_brack_; }
  Expr* index() const { return index_; }
  pos::pos_t r_brack() const { return r_brack_; }

  NodeKind node_kind() const override { return NodeKind::kIndexExpr; }
  pos::pos_t start() const override { return accessed_->start(); }
  pos::pos_t end() const override { return r_brack_; }

 private:
  IndexExpr(Expr* accessed, pos::pos_t l_brack, Expr* index, pos::pos_t r_brack)
      : accessed_(accessed), l_brack_(l_brack), index_(index), r_brack_(r_brack) {}

  Expr* accessed_;
  pos::pos_t l_brack_;
  Expr* index_;
  pos::pos_t r_brack_;

  friend class ASTBuilder;
};

// CallExpr ::= Expr ["<" Expr {"," Expr} ">"] "(" [Expr {"," Expr}] ")" .
class CallExpr final : public Expr {
 public:
  Expr* func() const { return func_; }
  pos::pos_t l_brack() const { return l_brack_; }
  std::vector<Expr*> type_args() const { return type_args_; }
  pos::pos_t r_brack() const { return r_brack_; }
  pos::pos_t l_paren() const { return l_paren_; }
  std::vector<Expr*> args() const { return args_; }
  pos::pos_t r_paren() const { return r_paren_; }

  NodeKind node_kind() const override { return NodeKind::kCallExpr; }
  pos::pos_t start() const override { return func_->start(); }
  pos::pos_t end() const override { return r_paren_; }

 private:
  CallExpr(Expr* func, pos::pos_t l_brack, std::vector<Expr*> type_args, pos::pos_t r_brack,
           pos::pos_t l_paren, std::vector<Expr*> args, pos::pos_t r_paren)
      : func_(func),
        l_brack_(l_brack),
        type_args_(type_args),
        r_brack_(r_brack),
        l_paren_(l_paren),
        args_(args),
        r_paren_(r_paren) {}

  Expr* func_;
  pos::pos_t l_brack_;
  std::vector<Expr*> type_args_;
  pos::pos_t r_brack_;
  pos::pos_t l_paren_;
  std::vector<Expr*> args_;
  pos::pos_t r_paren_;

  friend class ASTBuilder;
};

// FuncLit ::= FuncType BlockStmt .
class FuncLit final : public Expr {
 public:
  FuncType* type() const { return type_; }
  BlockStmt* body() const { return body_; }

  NodeKind node_kind() const override { return NodeKind::kFuncLit; }
  pos::pos_t start() const override;
  pos::pos_t end() const override;

 private:
  FuncLit(FuncType* type, BlockStmt* body) : type_(type), body_(body) {}

  FuncType* type_;
  BlockStmt* body_;

  friend class ASTBuilder;
};

// CompositeLit ::= Type "{" [Expr {"," Expr}] "}" .
class CompositeLit final : public Expr {
 public:
  Expr* type() const { return type_; }
  pos::pos_t l_brace() const { return l_brace_; }
  std::vector<Expr*> values() const { return values_; }
  pos::pos_t r_brace() const { return r_brace_; }

  NodeKind node_kind() const override { return NodeKind::kCompositeLit; }
  pos::pos_t start() const override { return type_->start(); }
  pos::pos_t end() const override { return r_brace_; }

 private:
  CompositeLit(Expr* type, pos::pos_t l_brace, std::vector<Expr*> values, pos::pos_t r_brace)
      : type_(type), l_brace_(l_brace), values_(values), r_brace_(r_brace) {}

  Expr* type_;
  pos::pos_t l_brace_;
  std::vector<Expr*> values_;
  pos::pos_t r_brace_;

  friend class ASTBuilder;
};

// KeyValueExpr ::= Expr ":" Expr .
class KeyValueExpr final : public Expr {
 public:
  Expr* key() const { return key_; }
  pos::pos_t colon() const { return colon_; }
  Expr* value() const { return value_; }

  NodeKind node_kind() const override { return NodeKind::kKeyValueExpr; }
  pos::pos_t start() const override { return key_->start(); }
  pos::pos_t end() const override { return value_->end(); }

 private:
  KeyValueExpr(Expr* key, pos::pos_t colon, Expr* value)
      : key_(key), colon_(colon), value_(value) {}

  Expr* key_;
  pos::pos_t colon_;
  Expr* value_;

  friend class ASTBuilder;
};

// ArrayType ::= "[" [Expr] "]" Type .
class ArrayType final : public Expr {
 public:
  pos::pos_t l_brack() const { return l_brack_; }
  Expr* len() const { return len_; }
  pos::pos_t r_brack() const { return r_brack_; }
  Expr* element_type() const { return element_type_; }

  NodeKind node_kind() const override { return NodeKind::kArrayType; }
  pos::pos_t start() const override { return l_brack_; }
  pos::pos_t end() const override { return element_type_->end(); }

 private:
  ArrayType(pos::pos_t l_brack, Expr* len, pos::pos_t r_brack, Expr* element_type)
      : l_brack_(l_brack), len_(len), r_brack_(r_brack), element_type_(element_type) {}

  pos::pos_t l_brack_;
  Expr* len_;
  pos::pos_t r_brack_;
  Expr* element_type_;

  friend class ASTBuilder;
};

// FuncType ::= "func" FieldList [FieldList] .
class FuncType final : public Expr {
 public:
  FieldList* params() const { return params_; }
  FieldList* results() const { return results_; }

  NodeKind node_kind() const override { return NodeKind::kFuncType; }
  pos::pos_t start() const override { return func_; }
  pos::pos_t end() const override;

 private:
  FuncType(pos::pos_t func_type_start, FieldList* params, FieldList* results)
      : func_(func_type_start), params_(params), results_(results) {}

  pos::pos_t func_;
  FieldList* params_;
  FieldList* results_;

  friend class ASTBuilder;
};

// InterfaceType ::= "interface" "{" {(Expr | MethodSpec) ";"} "}" .
class InterfaceType final : public Expr {
 public:
  pos::pos_t l_brace() const { return l_brace_; }
  std::vector<Expr*> embedded_interfaces() const { return embedded_interfaces_; }
  std::vector<MethodSpec*> methods() const { return methods_; }
  pos::pos_t r_brace() const { return r_brace_; }

  NodeKind node_kind() const override { return NodeKind::kInterfaceType; }
  pos::pos_t start() const override { return interface_; }
  pos::pos_t end() const override { return r_brace_; }

 private:
  InterfaceType(pos::pos_t interface_start, pos::pos_t l_brace,
                std::vector<Expr*> embedded_interfaces, std::vector<MethodSpec*> methods,
                pos::pos_t r_brace)
      : interface_(interface_start),
        l_brace_(l_brace),
        embedded_interfaces_(embedded_interfaces),
        methods_(methods),
        r_brace_(r_brace) {}

  pos::pos_t interface_;
  pos::pos_t l_brace_;
  std::vector<Expr*> embedded_interfaces_;
  std::vector<MethodSpec*> methods_;
  pos::pos_t r_brace_;

  friend class ASTBuilder;
};

// MethodSpec ::= ("(" [Ident] ")" | "<" [Ident] ">") Ident FieldList
// [FieldList] .
class MethodSpec final : public Node {
 public:
  tokens::Token kind() const { return kind_; }
  Ident* instance_type_param() const { return instance_type_param_; }
  Ident* name() const { return name_; }
  FieldList* params() const { return params_; }
  FieldList* results() const { return results_; }

  NodeKind node_kind() const override { return NodeKind::kMethodSpec; }
  pos::pos_t start() const override { return spec_start_; }
  pos::pos_t end() const override;

 private:
  MethodSpec(pos::pos_t spec_start, tokens::Token kind, Ident* instance_type_param, Ident* name,
             FieldList* params, FieldList* results)
      : spec_start_(spec_start),
        kind_(kind),
        instance_type_param_(instance_type_param),
        name_(name),
        params_(params),
        results_(results) {}

  pos::pos_t spec_start_;
  tokens::Token kind_;
  Ident* instance_type_param_;
  Ident* name_;
  FieldList* params_;
  FieldList* results_;

  friend class ASTBuilder;
};

// StructType ::= "class" "{" FieldList "}" .
class StructType final : public Expr {
 public:
  pos::pos_t l_brace() const { return l_brace_; }
  FieldList* fields() const { return fields_; }
  pos::pos_t r_brace() const { return r_brace_; }

  NodeKind node_kind() const override { return NodeKind::kStructType; }
  pos::pos_t start() const override { return struct_; }
  pos::pos_t end() const override { return r_brace_; }

 private:
  StructType(pos::pos_t struct_start, pos::pos_t l_brace, FieldList* fields, pos::pos_t r_brace)
      : struct_(struct_start), l_brace_(l_brace), fields_(fields), r_brace_(r_brace) {}

  pos::pos_t struct_;
  pos::pos_t l_brace_;
  FieldList* fields_;
  pos::pos_t r_brace_;

  friend class ASTBuilder;
};

// TypeInstance ::= Type "<" Expr {"," Expr} ">" .
class TypeInstance final : public Expr {
 public:
  Expr* type() const { return type_; }
  pos::pos_t l_brack() const { return l_brack_; }
  std::vector<Expr*> type_args() const { return type_args_; }
  pos::pos_t r_brack() const { return r_brack_; }

  NodeKind node_kind() const override { return NodeKind::kTypeInstance; }
  pos::pos_t start() const override { return type_->start(); }
  pos::pos_t end() const override { return r_brack_; }

 private:
  TypeInstance(Expr* type, pos::pos_t l_brack, std::vector<Expr*> type_args, pos::pos_t r_brack)
      : type_(type), l_brack_(l_brack), type_args_(type_args), r_brack_(r_brack) {}

  Expr* type_;
  pos::pos_t l_brack_;
  std::vector<Expr*> type_args_;
  pos::pos_t r_brack_;

  friend class ASTBuilder;
};

// ExprReceiver ::= '(' [Ident] ['*' | '%'] Ident ['<' [Ident {',' Ident}] '>']
// ')'
class ExprReceiver final : public Node {
 public:
  Ident* name() const { return name_; }
  tokens::Token pointer() const { return pointer_; }
  Ident* type_name() const { return type_name_; }
  std::vector<Ident*> type_parameter_names() const { return type_parameter_names_; }

  NodeKind node_kind() const override { return NodeKind::kExprReceiver; }
  pos::pos_t start() const override { return l_paren_; }
  pos::pos_t end() const override { return r_paren_; }

 private:
  ExprReceiver(pos::pos_t l_paren, Ident* name, tokens::Token pointer, Ident* type_name,
               std::vector<Ident*> type_parameter_names, pos::pos_t r_paren)
      : l_paren_(l_paren),
        name_(name),
        pointer_(pointer),
        type_name_(type_name),
        type_parameter_names_(type_parameter_names),
        r_paren_(r_paren) {}

  pos::pos_t l_paren_;
  Ident* name_;
  tokens::Token pointer_;
  Ident* type_name_;
  std::vector<Ident*> type_parameter_names_;
  pos::pos_t r_paren_;

  friend class ASTBuilder;
};

// TypeReceiver ::= '<' Ident ['<' [Ident {',' Ident}] '>'] '>'
class TypeReceiver final : public Node {
 public:
  Ident* type_name() const { return type_name_; }
  std::vector<Ident*> type_parameter_names() const { return type_parameter_names_; }

  NodeKind node_kind() const override { return NodeKind::kTypeReceiver; }
  pos::pos_t start() const override { return l_brack_; }
  pos::pos_t end() const override { return r_brack_; }

 private:
  TypeReceiver(pos::pos_t l_brack, Ident* type_name, std::vector<Ident*> type_parameter_names,
               pos::pos_t r_brack)
      : l_brack_(l_brack),
        type_name_(type_name),
        type_parameter_names_(type_parameter_names),
        r_brack_(r_brack) {}

  pos::pos_t l_brack_;
  Ident* type_name_;
  std::vector<Ident*> type_parameter_names_;
  pos::pos_t r_brack_;

  friend class ASTBuilder;
};

// FieldList ::= "(" [Field {"," Field}] ")"
//             | Field
//             | {Field ";"} .
class FieldList final : public Node {
 public:
  pos::pos_t l_paren() const { return l_paren_; }
  std::vector<Field*> fields() const { return fields_; }
  pos::pos_t r_paren() const { return r_paren_; }

  NodeKind node_kind() const override { return NodeKind::kFieldList; }
  pos::pos_t start() const override;
  pos::pos_t end() const override;

 private:
  FieldList(pos::pos_t l_paren, std::vector<Field*> fields, pos::pos_t r_paren)
      : l_paren_(l_paren), fields_(fields), r_paren_(r_paren) {}

  pos::pos_t l_paren_;
  std::vector<Field*> fields_;
  pos::pos_t r_paren_;

  friend class ASTBuilder;
};

// Field ::= {Ident} Type .
class Field final : public Node {
 public:
  std::vector<Ident*> names() const { return names_; }
  Expr* type() const { return type_; }

  NodeKind node_kind() const override { return NodeKind::kField; }
  pos::pos_t start() const override;
  pos::pos_t end() const override { return type_->end(); }

 private:
  Field(std::vector<Ident*> names, Expr* type) : names_(names), type_(type) {}

  std::vector<Ident*> names_;
  Expr* type_;

  friend class ASTBuilder;
};

// TypeParamList ::= "<" [TypeParam {"," TypeParam}] ">" .
class TypeParamList final : public Node {
 public:
  std::vector<TypeParam*> params() const { return params_; }

  NodeKind node_kind() const override { return NodeKind::kTypeParamList; }
  pos::pos_t start() const override { return l_angle_; }
  pos::pos_t end() const override { return r_angle_; }

 private:
  TypeParamList(pos::pos_t l_angle, std::vector<TypeParam*> params, pos::pos_t r_angle)
      : l_angle_(l_angle), params_(params), r_angle_(r_angle) {}

  pos::pos_t l_angle_;
  std::vector<TypeParam*> params_;
  pos::pos_t r_angle_;

  friend class ASTBuilder;
};

// TypeParam ::= Ident [Type] .
class TypeParam final : public Node {
 public:
  Ident* name() const { return name_; }
  Expr* type() const { return type_; }

  NodeKind node_kind() const override { return NodeKind::kTypeParam; }
  pos::pos_t start() const override;
  pos::pos_t end() const override;

 private:
  TypeParam(Ident* name, Expr* type) : name_(name), type_(type) {}

  Ident* name_;
  Expr* type_;

  friend class ASTBuilder;
};

class BasicLit final : public Expr {
 public:
  std::string value() const { return value_; }
  tokens::Token kind() const { return kind_; }

  NodeKind node_kind() const override { return NodeKind::kBasicLit; }
  pos::pos_t start() const override { return value_start_; }
  pos::pos_t end() const override { return value_start_ + value_.length() - 1; }

 private:
  BasicLit(pos::pos_t value_start, std::string value, tokens::Token kind)
      : value_start_(value_start), value_(value), kind_(kind) {}

  pos::pos_t value_start_;
  std::string value_;
  tokens::Token kind_;

  friend class ASTBuilder;
};

class Ident final : public Expr {
 public:
  std::string name() const { return name_; }

  NodeKind node_kind() const override { return NodeKind::kIdent; }
  pos::pos_t start() const override { return name_start_; }
  pos::pos_t end() const override { return name_start_ + name_.length() - 1; }

 private:
  Ident(pos::pos_t name_start, std::string name) : name_start_(name_start), name_(name) {}

  pos::pos_t name_start_;
  std::string name_;

  friend class ASTBuilder;
};

}  // namespace ast
}  // namespace lang

#endif /* lang_ast_nodes_h */
