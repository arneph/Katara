//
//  ast.h
//  Katara
//
//  Created by Arne Philipeit on 5/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_ast_h
#define lang_ast_h

#include <string>
#include <vector>

#include "lang/positions.h"
#include "lang/token.h"

namespace lang {
namespace ast {

struct Node {
    virtual ~Node() {}
    
    virtual pos::pos_t start() const = 0;
    virtual pos::pos_t end() const = 0;
};

// Expr ::= UnaryExpr
//        | BinaryExpr
//        | ParenExpr
//        | SelectionExpr
//        | TypeAssertExpr
//        | IndexExpr
//        | CallExpr
//        | KeyValueExpr
//        | FuncLit
//        | CompositeLit
//        | ArrayType
//        | FuncType
//        | InterfaceType
//        | StructType
//        | PointerType
//        | TypeInstance
//        | BasicLit
//        | Ident .
struct Expr : public Node {
    virtual ~Expr() {}
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
struct Stmt : public Node {
    virtual ~Stmt() {}
};

// Decl ::= GenDecl | FuncDecl .
struct Decl : public Node {
    virtual ~Decl() {}
};

struct File;

struct GenDecl;
struct Spec;
struct ValueSpec;
struct TypeSpec;
struct FuncDecl;

struct BlockStmt;
struct DeclStmt;
struct AssignStmt;
struct ExprStmt;
struct IncDecStmt;
struct ReturnStmt;
struct IfStmt;
struct SwitchStmt;
struct CaseClause;
struct ForStmt;
struct LabeledStmt;
struct BranchStmt;

struct UnaryExpr;
struct BinaryExpr;
struct ParenExpr;
struct SelectionExpr;
struct TypeAssertExpr;
struct IndexExpr;
struct CallExpr;
struct KeyValueExpr;
struct FuncLit;
struct CompositeLit;

struct ArrayType;
struct FuncType;
struct InterfaceType;
struct MethodSpec;
struct StructType;
struct PointerType;
struct TypeInstance;

struct FieldList;
struct Field;
struct TypeArgList;
struct TypeParamList;
struct TypeParam;

struct BasicLit;
struct Ident;

// File ::= {Decl} .
struct File : public Node {
    std::vector<std::unique_ptr<Decl>> decls_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// GenDecl ::= ("const" (ValueSpec | "(" {ValueSpec} ")" )
//           | ("var" (ValueSpec | "(" {ValueSpec} ")" )
//           | ("type" (TypeSpec | "( {TypeSpec} ")" ) .
struct GenDecl : public Decl {
    pos::pos_t tok_start_;
    token::Token tok_;
    pos::pos_t l_paren_;
    std::vector<std::unique_ptr<Spec>> specs_;
    pos::pos_t r_paren_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// Spec ::= ValueSpec | TypeSpec .
struct Spec : public Node {
    virtual ~Spec() {}
};

// ValueSpec ::= Ident {"," Ident} [Type] ["=" Expr {"," Expr}] "\n" .
struct ValueSpec : public Spec {
    std::vector<std::unique_ptr<Ident>> names_;
    std::unique_ptr<Expr> type_;
    std::vector<std::unique_ptr<Expr>> values_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// TypeSpec ::= Ident [TypeParamList] Type "\n" .
struct TypeSpec : public Spec {
    std::unique_ptr<Ident> name_;
    std::unique_ptr<TypeParamList> type_params_;
    std::unique_ptr<Expr> type_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// FuncDecl ::= "func" [] Ident [TypeParamList] FieldList [FieldList] BlockStmt .
struct FuncDecl : public Decl {
    std::unique_ptr<FieldList> receiver_;
    std::unique_ptr<Ident> name_;
    std::unique_ptr<TypeParamList> type_params_;
    std::unique_ptr<FuncType> type_;
    std::unique_ptr<BlockStmt> body_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// BlockStmt ::= "{" {Stmt} "}" .
struct BlockStmt : public Stmt {
    pos::pos_t l_brace_;
    std::vector<std::unique_ptr<Stmt>> stmts_;
    pos::pos_t r_brace_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// DeclStmt ::= GenDecl .
struct DeclStmt : public Stmt {
    std::unique_ptr<GenDecl> decl_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// AssignStmt ::= Expr {"," Expr} AssignOp Expr {"," Expr} .
struct AssignStmt : public Stmt {
    std::vector<std::unique_ptr<Expr>> lhs_;
    pos::pos_t tok_start_;
    token::Token tok_;
    std::vector<std::unique_ptr<Expr>> rhs_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// ExprStmt ::= Expr .
struct ExprStmt : public Stmt {
    std::unique_ptr<Expr> x_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// IncDecStmt ::= Expr ("++" | "--") .
struct IncDecStmt : public Stmt {
    std::unique_ptr<Expr> x_;
    pos::pos_t tok_start_;
    token::Token tok_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// ReturnStmt ::= "return" [Expr {"," Expr}] .
struct ReturnStmt : public Stmt {
    pos::pos_t return_;
    std::vector<std::unique_ptr<Expr>> results_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// IfStmt ::= "if" [Stmt ";"] Expr BlockStmt
//            ["else" (BlockStmt | IfStmt)] .
struct IfStmt : public Stmt {
    pos::pos_t if_;
    std::unique_ptr<Stmt> init_;
    std::unique_ptr<Expr> cond_;
    std::unique_ptr<BlockStmt> body_;
    std::unique_ptr<Stmt> else_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// SwitchStmt ::= "switch" [Stmt ";"] [Expr] BlockStmt .
struct SwitchStmt : public Stmt {
    pos::pos_t switch_;
    std::unique_ptr<Stmt> init_;
    std::unique_ptr<Expr> tag_;
    std::unique_ptr<BlockStmt> body_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// CaseClause ::= (("case" Expr {"," Expr}) | "default") ":" {Stmt} .
struct CaseClause : public Stmt {
    pos::pos_t tok_start_;
    token::Token tok_;
    std::vector<std::unique_ptr<Expr>> cond_vals_;
    pos::pos_t colon_;
    std::vector<std::unique_ptr<Stmt>> body_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// ForStmt ::= "for" [([Stmt] ";" Expr ";" [Stmt]) | Expr] BlockStmt .
struct ForStmt : public Stmt {
    pos::pos_t for_;
    std::unique_ptr<Stmt> init_;
    std::unique_ptr<Expr> cond_;
    std::unique_ptr<Stmt> post_;
    std::unique_ptr<BlockStmt> body_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// LabeledStmt ::= Ident ":" Stmt .
struct LabeledStmt : public Stmt {
    std::unique_ptr<Ident> label_;
    pos::pos_t colon_start_;
    std::unique_ptr<Stmt> stmt_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// BrancStmt ::= "fallthrough"
//             | "continue" [Ident]
//             | "break" [Ident] .
struct BranchStmt : public Stmt {
    pos::pos_t tok_start_;
    token::Token tok_;
    std::unique_ptr<Ident> label_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// UnaryExpr ::= UnaryOp Expr .
struct UnaryExpr : public Expr {
    pos::pos_t op_start_;
    token::Token op_;
    std::unique_ptr<Expr> x_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// BinaryOp ::= Expr BinaryOp Expr .
struct BinaryExpr : public Expr {
    std::unique_ptr<Expr> x_;
    pos::pos_t op_start_;
    token::Token op_;
    std::unique_ptr<Expr> y_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// ParenExpr ::= "(" Expr ") .
struct ParenExpr : public Expr {
    pos::pos_t l_paren_;
    std::unique_ptr<Expr> x_;
    pos::pos_t r_paren_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// SelectionExpr ::= Expr "." Ident .
struct SelectionExpr : public Expr {
    std::unique_ptr<Expr> accessed_;
    std::unique_ptr<Ident> selection_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// TypeAssertExpr ::= Expr "." "<" Type ">" .
struct TypeAssertExpr : public Expr {
    std::unique_ptr<Expr> x_;
    pos::pos_t l_angle_;
    std::unique_ptr<Expr> type_; // nullptr for "type" keyword in type switch
    pos::pos_t r_angle_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// IndexExpr ::= Expr "[" Expr "]" .
struct IndexExpr : public Expr {
    std::unique_ptr<Expr> accessed_;
    pos::pos_t l_brack_;
    std::unique_ptr<Expr> index_;
    pos::pos_t r_brack_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// CallExpr ::= Expr [TypeArgList] "(" [Expr {"," Expr}] ")" .
struct CallExpr : public Expr {
    std::unique_ptr<Expr> func_;
    std::unique_ptr<TypeArgList> type_args_;
    pos::pos_t l_paren_;
    std::vector<std::unique_ptr<Expr>> args_;
    pos::pos_t r_paren_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// KeyValueExpr ::= Expr ":" Expr .
struct KeyValueExpr : public Expr {
    std::unique_ptr<Expr> key_;
    pos::pos_t colon_;
    std::unique_ptr<Expr> value_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// FuncLit ::= FuncType BlockStmt .
struct FuncLit : public Expr {
    std::unique_ptr<FuncType> type_;
    std::unique_ptr<BlockStmt> body_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// CompositeLit ::= Type "{" [Expr {"," Expr}] "}" .
struct CompositeLit : public Expr {
    std::unique_ptr<Expr> type_;
    pos::pos_t l_brace_;
    std::vector<std::unique_ptr<Expr>> values_;
    pos::pos_t r_brace_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// ArrayType ::= "[" Expr "]" Type .
struct ArrayType : public Expr {
    pos::pos_t l_brack_;
    std::unique_ptr<Expr> len_;
    pos::pos_t r_brack_;
    std::unique_ptr<Expr> element_type_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// FuncType ::= "func" FieldList [FieldList] .
struct FuncType : public Expr {
    pos::pos_t func_;
    std::unique_ptr<FieldList> params_;
    std::unique_ptr<FieldList> results_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// InterfaceType ::= "interface" "{" {MethodSpec ";"} "}" .
struct InterfaceType : public Expr {
    pos::pos_t interface_;
    pos::pos_t l_brace_;
    std::vector<std::unique_ptr<MethodSpec>> methods_;
    pos::pos_t r_brace_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// MethodSpec ::= Ident FieldList [FieldList] .
struct MethodSpec : public Node {
    std::unique_ptr<Ident> name_;
    std::unique_ptr<FieldList> params_;
    std::unique_ptr<FieldList> results_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// StructType ::= "struct" "{" FieldList "}" .
struct StructType : public Expr {
    pos::pos_t struct_;
    pos::pos_t l_brace_;
    std::unique_ptr<FieldList> fields_;
    pos::pos_t r_brace_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// TypeInstance ::= Type TypeArgList .
struct TypeInstance : public Expr {
    std::unique_ptr<Expr> type_;
    std::unique_ptr<TypeArgList> type_args_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// FieldList ::= "(" [Field {"," Field}] ")"
//             | Field
//             | {Field ";"} .
struct FieldList : public Node {
    pos::pos_t l_paren_;
    std::vector<std::unique_ptr<Field>> fields_;
    pos::pos_t r_paren_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// Field ::= {Ident} Type .
struct Field : public Node {
    std::vector<std::unique_ptr<Ident>> names_;
    std::unique_ptr<Expr> type_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// TypeArgList ::= "<" [Type {"," Type} ">" .
struct TypeArgList : public Node {
    pos::pos_t l_angle_;
    std::vector<std::unique_ptr<Expr>> args_;
    pos::pos_t r_angle_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// TypeParamList ::= "<" [TypeParam {"," TypeParam}] ">" .
struct TypeParamList : public Node {
    pos::pos_t l_angle_;
    std::vector<std::unique_ptr<TypeParam>> params_;
    pos::pos_t r_angle_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

// TypeParam ::= Ident [Type] .
struct TypeParam : public Node {
    std::unique_ptr<Ident> name_;
    std::unique_ptr<Expr> type_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

struct BasicLit : public Expr {
    pos::pos_t value_start_;
    token::Token kind_;
    std::string value_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

struct Ident : public Expr {
    pos::pos_t name_start_;
    std::string name_;
    
    pos::pos_t start() const;
    pos::pos_t end() const;
};

}
}

#endif /* lang_ast_h */
