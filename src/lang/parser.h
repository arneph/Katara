//
//  parser.h
//  Katara
//
//  Created by Arne Philipeit on 5/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_parser_h
#define lang_parser_h

#include <memory>
#include <string>
#include <vector>

#include "lang/positions.h"
#include "lang/token.h"
#include "lang/ast.h"
#include "lang/scanner.h"

namespace lang {
namespace parser {

class Parser {
public:
    struct Error {
        pos::pos_t pos_;
        std::string message_;
    };
    
    static std::unique_ptr<ast::File> ParseFile(std::string file_contents,
                                                std::vector<Error>& errors);
    
private:
    Parser(std::string file_contents,
           std::vector<Error>& errors);
    
    std::unique_ptr<ast::File> ParseFile();
    
    std::unique_ptr<ast::Decl> ParseDecl();
    std::unique_ptr<ast::GenDecl> ParseGenDecl();
    std::unique_ptr<ast::Spec> ParseSpec(token::Token spec_type);
    std::unique_ptr<ast::ValueSpec> ParseValueSpec();
    std::unique_ptr<ast::TypeSpec> ParseTypeSpec();
    std::unique_ptr<ast::FuncDecl> ParseFuncDecl();
    
    std::vector<std::unique_ptr<ast::Stmt>> ParseStmtList();
    std::unique_ptr<ast::Stmt>ParseStmt();
    std::unique_ptr<ast::Stmt>ParseSimpleStmt(bool disallow_composite_lit);
    std::unique_ptr<ast::Stmt>ParseSimpleStmt(std::unique_ptr<ast::Expr> expr,
                                              bool disallow_composite_lit);
    std::unique_ptr<ast::BlockStmt> ParseBlockStmt();
    std::unique_ptr<ast::DeclStmt> ParseDeclStmt();
    std::unique_ptr<ast::ReturnStmt> ParseReturnStmt();
    std::unique_ptr<ast::IfStmt> ParseIfStmt();
    std::unique_ptr<ast::SwitchStmt> ParseSwitchStmt();
    std::unique_ptr<ast::CaseClause> ParseCaseClause();
    std::unique_ptr<ast::ForStmt> ParseForStmt();
    std::unique_ptr<ast::BranchStmt> ParseBranchStmt();
    std::unique_ptr<ast::ExprStmt> ParseExprStmt(std::unique_ptr<ast::Expr> x);
    std::unique_ptr<ast::LabeledStmt> ParseLabeledStmt(std::unique_ptr<ast::Ident> label);
    std::unique_ptr<ast::AssignStmt> ParseAssignStmt(std::unique_ptr<ast::Expr> first_expr,
                                                     bool disallow_composite_lit);
    std::unique_ptr<ast::IncDecStmt> ParseIncDecStmt(std::unique_ptr<ast::Expr> x);
    
    std::vector<std::unique_ptr<ast::Expr>> ParseExprList(bool disallow_composite_lit);
    std::vector<std::unique_ptr<ast::Expr>> ParseExprList(std::unique_ptr<ast::Expr> first_expr,
                                                          bool disallow_composite_lit);
    std::unique_ptr<ast::Expr> ParseExpr(bool disallow_composite_lit);
    std::unique_ptr<ast::Expr> ParseExpr(token::precedence_t prec,
                                         bool disallow_composite_lit);
    std::unique_ptr<ast::Expr> ParseUnaryExpr(bool disallow_composite_lit);
    
    std::unique_ptr<ast::Expr> ParsePrimaryExpr(bool disallow_composite_lit);
    std::unique_ptr<ast::Expr> ParsePrimaryExpr(std::unique_ptr<ast::Expr> primary_expr,
                                                bool disallow_composite_lit);
    std::unique_ptr<ast::Expr> ParsePrimaryExpr(std::unique_ptr<ast::Expr> primary_expr,
                                                std::unique_ptr<ast::TypeArgList> type_args,
                                                bool disallow_composite_lit);
    std::unique_ptr<ast::ParenExpr> ParseParenExpr();
    std::unique_ptr<ast::SelectionExpr> ParseSelectionExpr(std::unique_ptr<ast::Expr> accessed);
    std::unique_ptr<ast::TypeAssertExpr> ParseTypeAssertExpr(std::unique_ptr<ast::Expr> x);
    std::unique_ptr<ast::IndexExpr> ParseIndexExpr(std::unique_ptr<ast::Expr> accessed);
    std::unique_ptr<ast::CallExpr> ParseCallExpr(std::unique_ptr<ast::Expr> func,
                                                 std::unique_ptr<ast::TypeArgList> type_args);
    std::unique_ptr<ast::FuncLit> ParseFuncLit(std::unique_ptr<ast::FuncType> type);
    std::unique_ptr<ast::CompositeLit> ParseCompositeLit(std::unique_ptr<ast::Expr> type);
    std::unique_ptr<ast::Expr> ParseCompositeLitElement();
    
    std::unique_ptr<ast::Expr> ParseType();
    std::unique_ptr<ast::Expr> ParseType(std::unique_ptr<ast::Ident> ident);
    std::unique_ptr<ast::ArrayType> ParseArrayType();
    std::unique_ptr<ast::FuncType> ParseFuncType();
    std::unique_ptr<ast::InterfaceType> ParseInterfaceType();
    std::unique_ptr<ast::MethodSpec> ParseMethodSpec();
    std::unique_ptr<ast::StructType> ParseStructType();
    std::unique_ptr<ast::UnaryExpr> ParsePointerType();
    std::unique_ptr<ast::TypeInstance> ParseTypeInstance(std::unique_ptr<ast::Expr> type);
    
    std::unique_ptr<ast::FieldList> ParseFuncFieldList(bool expect_paren);
    std::unique_ptr<ast::FieldList> ParseStructFieldList();
    std::unique_ptr<ast::Field> ParseField();
    std::unique_ptr<ast::TypeArgList> ParseTypeArgList();
    std::unique_ptr<ast::TypeParamList> ParseTypeParamList();
    std::unique_ptr<ast::TypeParam> ParseTypeParam();
    
    std::unique_ptr<ast::BasicLit> ParseBasicLit();
    std::vector<std::unique_ptr<ast::Ident>> ParseIdentList();
    std::unique_ptr<ast::Ident> ParseIdent(bool split_shift_ops = false);
    
    scanner::Scanner scanner_;
    std::vector<Error>& errors_;
};

}
}

#endif /* lang_parser_h */
