//
//  parser.cc
//  Katara
//
//  Created by Arne Philipeit on 5/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "parser.h"

namespace lang {
namespace parser {

ast::File * Parser::ParseFile(pos::File *file,
                              ast::ASTBuilder& builder,
                              std::vector<issues::Issue>& issues) {
    scanner::Scanner scanner(file);
    Parser parser(scanner, builder, issues);
    return parser.ParseFile();
}

ast::File * Parser::ParseFile() {
    pos::pos_t file_start = scanner_.token_start();
    while (scanner_.token() != tokens::kPackage) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected package declaration"));
        return nullptr;
    }
    scanner_.Next();
    ast::Ident *package_name = ParseIdent();
    if (scanner_.token() != tokens::kSemicolon) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ';' or new line"));
        return nullptr;
    }
    scanner_.Next();
    
    std::vector<ast::Decl *> decls;
    bool finished_imports = false;
    while (scanner_.token() != tokens::kEOF) {
        if (scanner_.token() != tokens::kImport) {
            finished_imports = true;
        } else if (finished_imports) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "imports not allowed after non-import declarations"));
        }
        ast::Decl *decl = ParseDecl();
        if (decl != nullptr) {
            decls.push_back(decl);
        }
        if (scanner_.token() != tokens::kSemicolon) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected ';' or new line"));
            scanner_.SkipPastLine();
            continue;
        }
        scanner_.Next();
    }
    pos::pos_t file_end = scanner_.token_end();
    
    return ast_builder_.Create<ast::File>(file_start,
                                          file_end,
                                          package_name,
                                          decls);
}

ast::Decl * Parser::ParseDecl() {
    switch (scanner_.token()) {
        case tokens::kImport:
        case tokens::kConst:
        case tokens::kVar:
        case tokens::kType:
            return ParseGenDecl();
        case tokens::kFunc:
            return ParseFuncDecl();
        default:
            issues_.push_back(
                issues::Issue(issues::Origin::Parser,
                              issues::Severity::Fatal,
                              scanner_.token_start(),
                              "expected 'import', 'const', 'var', 'type', or 'func'"));
            scanner_.SkipPastLine();
            return nullptr;
    }
}

ast::GenDecl * Parser::ParseGenDecl() {
    pos::pos_t tok_start = scanner_.token_start();
    tokens::Token tok = scanner_.token();
    scanner_.Next();
    
    pos::pos_t l_paren = pos::kNoPos;
    std::vector<ast::Spec *> specs;
    pos::pos_t r_paren = pos::kNoPos;
    if (scanner_.token() == tokens::kLParen) {
        l_paren = scanner_.token_start();
        scanner_.Next();
        while (scanner_.token() != tokens::kRParen) {
            ast::Spec *spec = ParseSpec(tok);
            if (spec != nullptr) {
                specs.push_back(spec);
            }
            if (scanner_.token() != tokens::kSemicolon) {
                issues_.push_back(issues::Issue(issues::Origin::Parser,
                                                issues::Severity::Fatal,
                                                scanner_.token_start(),
                                                "expected ';' or new line"));
                return nullptr;
            }
            scanner_.Next();
        }
        if (scanner_.token() != tokens::kRParen) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected ')'"));
            return nullptr;
        }
        r_paren = scanner_.token_end();
        scanner_.Next();
        
    } else {
        ast::Spec *spec = ParseSpec(tok);
        if (spec == nullptr) {
            return nullptr;
        }
        specs.push_back(spec);
    }
    
    return ast_builder_.Create<ast::GenDecl>(tok_start,
                                             tok,
                                             l_paren,
                                             specs,
                                             r_paren);
}

ast::Spec * Parser::ParseSpec(tokens::Token spec_type) {
    switch (spec_type) {
        case tokens::kImport:
            return ParseImportSpec();
        case tokens::kConst:
        case tokens::kVar:
            return ParseValueSpec();
        case tokens::kType:
            return ParseTypeSpec();
        default:
            throw "unexpected spec type";
    }
}

ast::ImportSpec * Parser::ParseImportSpec() {
    ast::Ident *name = nullptr;
    if (scanner_.token() == tokens::kIdent) {
        name = ParseIdent();
        if (name == nullptr) {
            return nullptr;
        }
    }
    
    if (scanner_.token() != tokens::kString) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected import package path"));
        return nullptr;
    }
    ast::BasicLit *path = ParseBasicLit();
    if (path == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::ImportSpec>(name, path);
}

ast::ValueSpec * Parser::ParseValueSpec() {
    std::vector<ast::Ident *> names = ParseIdentList();
    if (names.empty()) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    
    ast::Expr *type = nullptr;
    if (scanner_.token() != tokens::kAssign &&
        scanner_.token() != tokens::kSemicolon) {
        type = ParseType();
        if (type == nullptr) {
            scanner_.SkipPastLine();
            return nullptr;
        }
    }
    
    std::vector<ast::Expr *> values;
    if (scanner_.token() == tokens::kAssign) {
        scanner_.Next();
        values = ParseExprList(kNoExprOptions);
        if (values.empty()) {
            scanner_.SkipPastLine();
            return nullptr;
        }
    }
    
    return ast_builder_.Create<ast::ValueSpec>(names, type, values);
}

ast::TypeSpec * Parser::ParseTypeSpec() {
    ast::Ident *name = ParseIdent();
    if (name == nullptr) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    
    ast::TypeParamList *type_params = nullptr;
    if (scanner_.token() == tokens::kLss) {
        type_params = ParseTypeParamList();
        if (type_params == nullptr) {
            return nullptr;
        }
    }
    
    pos::pos_t assign = pos::kNoPos;
    if (scanner_.token() == tokens::kAssign) {
        assign = scanner_.token_start();
        scanner_.Next();
    }
    
    ast::Expr *type = ParseType();
    if (type == nullptr) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    
    return ast_builder_.Create<ast::TypeSpec>(name, type_params, assign, type);
}

ast::FuncDecl * Parser::ParseFuncDecl() {
    pos::pos_t func = scanner_.token_start();
    scanner_.Next();
    
    ast::FuncDecl::Kind kind;
    ast::ExprReceiver *expr_receiver;
    ast::TypeReceiver *type_receiver;
    if (scanner_.token() == tokens::kLParen) {
        kind = ast::FuncDecl::Kind::kInstanceMethod;
        expr_receiver = ParseExprReceiver();
        if (expr_receiver == nullptr) {
            return nullptr;
        }
    } else if (scanner_.token() == tokens::kLss) {
        kind = ast::FuncDecl::Kind::kTypeMethod;
        type_receiver = ParseTypeReceiver();
        if (type_receiver == nullptr) {
            return nullptr;
        }
    } else {
        kind = ast::FuncDecl::Kind::kFunc;
    }
    
    ast::Ident *name = ParseIdent();
    if (name == nullptr) {
        return nullptr;
    }
    
    ast::TypeParamList *type_params = nullptr;
    if (scanner_.token() == tokens::kLss) {
        type_params = ParseTypeParamList();
        if (type_params == nullptr) {
            return nullptr;
        }
    }
    
    ast::FieldList *params = ParseFuncFieldList(kExpectParen);
    if (params == nullptr) {
        return nullptr;
    }
    
    ast::FieldList *results = nullptr;
    if (scanner_.token() != tokens::kLBrace) {
        results = ParseFuncFieldList(kNoFuncFieldListOptions);
        if (results == nullptr) {
            scanner_.SkipPastLine();
            return nullptr;
        }
    }
    
    ast::BlockStmt *body = ParseBlockStmt();
    if (body == nullptr) {
        return nullptr;
    }
    
    ast::FuncType *func_type = ast_builder_.Create<ast::FuncType>(func,
                                                                  params,
                                                                  results);
    switch (kind) {
        case ast::FuncDecl::Kind::kFunc:
            return ast_builder_.Create<ast::FuncDecl>(name, type_params, func_type, body);
        case ast::FuncDecl::Kind::kInstanceMethod:
            return ast_builder_.Create<ast::FuncDecl>(expr_receiver, name, type_params, func_type,
                                                      body);
        case ast::FuncDecl::Kind::kTypeMethod:
            return ast_builder_.Create<ast::FuncDecl>(type_receiver, name, type_params, func_type,
                                                      body);
        default:
            throw "internal error: unexpected func decl kind";
    }
}

std::vector<ast::Stmt *> Parser::ParseStmtList() {
    std::vector<ast::Stmt *> list;
    while (scanner_.token() != tokens::kRBrace && scanner_.token() != tokens::kCase) {
        ast::Stmt *stmt = ParseStmt();
        if (stmt == nullptr) {
            continue;
        }
        list.push_back(stmt);
        if (scanner_.token() == tokens::kSemicolon) {
            scanner_.Next();
            continue;
        } else if (scanner_.token() == tokens::kRBrace ||
                   scanner_.token() == tokens::kCase) {
            scanner_.Next();
            break;
        } else {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected ';' or new line"));
            scanner_.SkipPastLine();
        }
    }
    return list;
}

ast::Stmt * Parser::ParseStmt() {
    switch (scanner_.token()) {
        case tokens::kLBrace:
            return ParseBlockStmt();
        case tokens::kConst:
        case tokens::kVar:
        case tokens::kType:
            return ParseDeclStmt();
        case tokens::kReturn:
            return ParseReturnStmt();
        case tokens::kIf:
            return ParseIfStmt();
        case tokens::kSwitch:
            return ParseSwitchStmt();
        case tokens::kFor:
            return ParseForStmt();
        case tokens::kFallthrough:
        case tokens::kContinue:
        case tokens::kBreak:
            return ParseBranchStmt();
        default:
            break;
    }
    
    ast::Expr *expr = ParseExpr(kNoExprOptions);
    if (expr == nullptr) {
        return nullptr;
    }
    
    if (scanner_.token() == tokens::kColon) {
        if (expr->node_kind() != ast::NodeKind::kIdent) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            expr->start(),
                                            "expression can not be used as label"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        return ParseLabeledStmt(static_cast<ast::Ident *>(expr));
    } else {
        return ParseSimpleStmt(expr, kNoExprOptions);
    }
}

ast::Stmt * Parser::ParseSimpleStmt(ExprOptions expr_options) {
    ast::Expr *expr = ParseExpr(expr_options);
    if (expr == nullptr) {
        return nullptr;
    }
    
    return ParseSimpleStmt(expr, expr_options);
}

ast::Stmt * Parser::ParseSimpleStmt(ast::Expr *expr, ExprOptions expr_options) {
    switch (scanner_.token()) {
        case tokens::kInc:
        case tokens::kDec:
            return ParseIncDecStmt(expr);
        case tokens::kComma:
        case tokens::kAddAssign:
        case tokens::kSubAssign:
        case tokens::kMulAssign:
        case tokens::kQuoAssign:
        case tokens::kRemAssign:
        case tokens::kAndAssign:
        case tokens::kOrAssign:
        case tokens::kXorAssign:
        case tokens::kShlAssign:
        case tokens::kShrAssign:
        case tokens::kAndNotAssign:
        case tokens::kAssign:
        case tokens::kDefine:
            return ParseAssignStmt(expr, expr_options);
        default:
            return ParseExprStmt(expr);
    }
}

ast::BlockStmt * Parser::ParseBlockStmt() {
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_brace = scanner_.token_start();
    scanner_.Next();
    
    std::vector<ast::Stmt *> stmts = ParseStmtList();
    
    if (scanner_.token() != tokens::kRBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '}'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_brace = scanner_.token_start();
    scanner_.Next();
    
    return ast_builder_.Create<ast::BlockStmt>(l_brace, stmts, r_brace);
}

ast::DeclStmt * Parser::ParseDeclStmt() {
    ast::GenDecl *decl = ParseGenDecl();
    if (decl == nullptr) {
        return nullptr;
    }
    return ast_builder_.Create<ast::DeclStmt>(decl);
}

ast::ReturnStmt * Parser::ParseReturnStmt() {
    if (scanner_.token() != tokens::kReturn) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'return'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t return_ = scanner_.token_start();
    scanner_.Next();
    
    std::vector<ast::Expr *> results;
    if (scanner_.token() == tokens::kSemicolon) {
        return ast_builder_.Create<ast::ReturnStmt>(return_, results);
    }
    results = ParseExprList(kNoExprOptions);
    if (results.empty()) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::ReturnStmt>(return_, results);
}

ast::IfStmt * Parser::ParseIfStmt() {
    if (scanner_.token() != tokens::kIf) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'if'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t if_ = scanner_.token_start();
    scanner_.Next();
    
    ast::Expr *expr = ParseExpr(kDisallowCompositeLit);
    if (expr == nullptr) {
        return nullptr;
    }
    
    ast::Stmt *init = nullptr;
    ast::Expr *cond;
    if (scanner_.token() == tokens::kLBrace) {
        cond = expr;
    } else {
        init = ParseSimpleStmt(expr, kDisallowCompositeLit);
        if (init == nullptr) {
            return nullptr;
        }
        
        if (scanner_.token() != tokens::kSemicolon) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected ';'"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        scanner_.Next();
        
        cond = ParseExpr(kDisallowCompositeLit);
        if (cond == nullptr) {
            return nullptr;
        }
    }
    
    ast::BlockStmt *body = ParseBlockStmt();
    if (body == nullptr) {
        return nullptr;
    }
    
    ast::Stmt *else_stmt = nullptr;
    if (scanner_.token() != tokens::kElse) {
        return ast_builder_.Create<ast::IfStmt>(if_, init, cond, body, else_stmt);
    }
    scanner_.Next();
    
    if (scanner_.token() != tokens::kIf &&
        scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'if' or '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    else_stmt = ParseStmt();
    if (else_stmt == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::IfStmt>(if_, init, cond, body, else_stmt);
}

ast::Stmt * Parser::ParseSwitchStmt() {
    if (scanner_.token() != tokens::kSwitch) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'switch'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t switch_start = scanner_.token_start();
    scanner_.Next();
    
    ast::Stmt *init = nullptr;
    ast::Expr *tag = nullptr;
    if (scanner_.token() != tokens::kLBrace) {
        ast::Expr *expr = ParseExpr(kDisallowCompositeLit);
        if (expr == nullptr) {
            return nullptr;
        }
        
        if (scanner_.token() == tokens::kLBrace) {
            tag = expr;
        } else {
            init = ParseSimpleStmt(expr, kDisallowCompositeLit);
            if (init == nullptr) {
                return nullptr;
            }
            if (scanner_.token() != tokens::kSemicolon) {
                issues_.push_back(issues::Issue(issues::Origin::Parser,
                                                issues::Severity::Fatal,
                                                scanner_.token_start(),
                                                "expected ';'"));
                scanner_.SkipPastLine();
                return nullptr;
            }
            scanner_.Next();
            
            if (scanner_.token() != tokens::kLBrace) {
                tag = ParseExpr(kDisallowCompositeLit);
                if (tag == nullptr) {
                    return nullptr;
                }
            }
        }
    }
    
    auto process_expr_switch_stmt = [&]() -> ast::ExprSwitchStmt* {
        ast::BlockStmt *body = ParseSwitchStmtBody();
        if (body == nullptr) {
            return nullptr;
        }
        return ast_builder_.Create<ast::ExprSwitchStmt>(switch_start, init, tag, body);
    };
    auto process_type_switch_stmt = [&](ast::Ident *var, ast::Expr *tag) -> ast::TypeSwitchStmt* {
        ast::BlockStmt *body = ParseSwitchStmtBody();
        if (body == nullptr) {
            return nullptr;
        }
        return ast_builder_.Create<ast::TypeSwitchStmt>(switch_start, var, tag, body);
    };
    
    if (init != nullptr && tag == nullptr) {
        if (init->node_kind() != ast::NodeKind::kAssignStmt) {
            return process_expr_switch_stmt();
        }
        ast::AssignStmt *assign_stmt = static_cast<ast::AssignStmt *>(init);
        if (assign_stmt->tok() != tokens::kDefine ||
            assign_stmt->lhs().size() != 1 ||
            assign_stmt->rhs().size() != 1 ||
            assign_stmt->lhs().at(0)->node_kind() != ast::NodeKind::kIdent ||
            assign_stmt->rhs().at(0)->node_kind() != ast::NodeKind::kTypeAssertExpr) {
            return process_expr_switch_stmt();
        }
        auto var = static_cast<ast::Ident *>(assign_stmt->lhs().at(0));
        auto type_assert_expr = static_cast<ast::TypeAssertExpr *>(assign_stmt->rhs().at(0));
        if (type_assert_expr->type() != nullptr) {
            return process_expr_switch_stmt();
        }
        return process_type_switch_stmt(var, type_assert_expr->x());
    }
    if (init == nullptr && tag != nullptr) {
        if (tag->node_kind() != ast::NodeKind::kTypeAssertExpr) {
            return process_expr_switch_stmt();
        }
        ast::TypeAssertExpr *type_assert_expr = static_cast<ast::TypeAssertExpr *>(tag);
        if (type_assert_expr->type() != nullptr) {
            return process_expr_switch_stmt();
        }
        return process_type_switch_stmt(nullptr, type_assert_expr->x());
    }
    
    return process_expr_switch_stmt();
}

ast::BlockStmt * Parser::ParseSwitchStmtBody() {
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_brace = scanner_.token();
    scanner_.Next();
    
    std::vector<ast::Stmt *> stmts;
    while (scanner_.token() != tokens::kRBrace) {
        ast::CaseClause *clause = ParseCaseClause();
        if (clause == nullptr) {
            return nullptr;
        }
        stmts.push_back(clause);
    }
    
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_brace = scanner_.token();
    scanner_.Next();
    return ast_builder_.Create<ast::BlockStmt>(l_brace, stmts, r_brace);
}

ast::CaseClause * Parser::ParseCaseClause() {
    if (scanner_.token() != tokens::kCase &&
        scanner_.token() != tokens::kDefault) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'case' or 'default'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t tok_start = scanner_.token_start();
    tokens::Token tok = scanner_.token();
    scanner_.Next();
    
    std::vector<ast::Expr *> cond_vals;
    if (tok == tokens::kCase) {
        cond_vals = ParseExprList(kNoExprOptions);
        if (cond_vals.empty()) {
            scanner_.SkipPastLine();
            return nullptr;
        }
    }
    
    if (scanner_.token() != tokens::kColon) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ':'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t colon = scanner_.token_start();
    scanner_.Next();
    
    std::vector<ast::Stmt *> body = ParseStmtList();
    
    return ast_builder_.Create<ast::CaseClause>(tok_start,
                                                tok,
                                                cond_vals,
                                                colon,
                                                body);
}

ast::ForStmt * Parser::ParseForStmt() {
    if (scanner_.token() != tokens::kFor) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'for'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t for_ = scanner_.token_start();
    scanner_.Next();
    
    ast::Stmt *init = nullptr;
    ast::Expr *cond = nullptr;
    ast::Stmt *post = nullptr;
    if (scanner_.token() != tokens::kLBrace) {
        ast::Expr *expr = ParseExpr(kDisallowCompositeLit);
        if (expr == nullptr) {
            return nullptr;
        }
        
        if (scanner_.token() == tokens::kLBrace) {
            cond = std::move(expr);
        } else {
            init = ParseSimpleStmt(expr, kDisallowCompositeLit);
            if (init == nullptr) {
                return nullptr;
            }
            
            if (scanner_.token() != tokens::kSemicolon) {
                issues_.push_back(issues::Issue(issues::Origin::Parser,
                                                issues::Severity::Fatal,
                                                scanner_.token_start(),
                    "expected ';'"));
                scanner_.SkipPastLine();
                return nullptr;
            }
            scanner_.Next();
            
            cond = ParseExpr(kDisallowCompositeLit);
            if (cond == nullptr) {
                return nullptr;
            }
            
            if (scanner_.token() != tokens::kSemicolon) {
                issues_.push_back(issues::Issue(issues::Origin::Parser,
                                                issues::Severity::Fatal,
                                                scanner_.token_start(),
                                                "expected ';'"));
                scanner_.SkipPastLine();
                return nullptr;
            }
            scanner_.Next();
            
            if (scanner_.token() != tokens::kLBrace) {
                post = ParseSimpleStmt(kDisallowCompositeLit);
                if (post == nullptr) {
                    return nullptr;
                }
                if (post->node_kind() == ast::NodeKind::kAssignStmt &&
                    static_cast<ast::AssignStmt *>(post)->tok() == tokens::kDefine) {
                    issues_.push_back(
                        issues::Issue(issues::Origin::Parser,
                                      issues::Severity::Fatal,
                                      post->start(),
                                      "for loop post statement can not define variables"));
                    return nullptr;
                }
            }
        }
    }
    
    ast::BlockStmt *body = ParseBlockStmt();
    if (body == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::ForStmt>(for_, init, cond, post, body);
}

ast::BranchStmt * Parser::ParseBranchStmt() {
    if (scanner_.token() != tokens::kFallthrough &&
        scanner_.token() != tokens::kContinue &&
        scanner_.token() != tokens::kBreak) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'fallthrough', 'continue', or 'break'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t tok_start = scanner_.token_start();
    tokens::Token tok = scanner_.token();
    scanner_.Next();
    
    ast::Ident *label = nullptr;
    if (tok == tokens::kContinue ||
        tok == tokens::kBreak) {
        if (scanner_.token() == tokens::kIdent) {
            label = ParseIdent();
        }
    }
    
    return ast_builder_.Create<ast::BranchStmt>(tok_start, tok, label);
}

ast::ExprStmt * Parser::ParseExprStmt(ast::Expr *x) {
    if (x->node_kind() != ast::NodeKind::kCallExpr) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        x->start(),
                                        "expression can not be used as standalone statement"));
        return nullptr;
    }
    
    return ast_builder_.Create<ast::ExprStmt>(x);
}

ast::LabeledStmt * Parser::ParseLabeledStmt(ast::Ident *label) {
    if (scanner_.token() != tokens::kColon) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ':'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t colon = scanner_.token_start();
    scanner_.Next();
    
    ast::Stmt *stmt = ParseStmt();
    if (stmt == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::LabeledStmt>(label, colon, stmt);
}

ast::AssignStmt * Parser::ParseAssignStmt(ast::Expr *first_expr, ExprOptions expr_options) {
    std::vector<ast::Expr *> lhs = ParseExprList(first_expr, expr_options);
    
    switch (scanner_.token()) {
        case tokens::kAddAssign:
        case tokens::kSubAssign:
        case tokens::kMulAssign:
        case tokens::kQuoAssign:
        case tokens::kRemAssign:
        case tokens::kAndAssign:
        case tokens::kOrAssign:
        case tokens::kXorAssign:
        case tokens::kShlAssign:
        case tokens::kShrAssign:
        case tokens::kAndNotAssign:
        case tokens::kAssign:
        case tokens::kDefine:
            break;
        default:
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected assignment operator"));
            scanner_.SkipPastLine();
            return nullptr;
    }
    pos::pos_t tok_start = scanner_.token_start();
    tokens::Token tok = scanner_.token();
    scanner_.Next();
    
    std::vector<ast::Expr *> rhs = ParseExprList(expr_options);
    if (rhs.empty()) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::AssignStmt>(lhs, tok_start, tok, rhs);
}

ast::IncDecStmt * Parser::ParseIncDecStmt(ast::Expr *x) {
    if (scanner_.token() != tokens::kInc &&
        scanner_.token() != tokens::kDec) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '++' or '--'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t tok_start = scanner_.token_start();
    tokens::Token tok = scanner_.token();
    scanner_.Next();
    
    return ast_builder_.Create<ast::IncDecStmt>(x, tok_start, tok);
}

std::vector<ast::Expr *> Parser::ParseExprList(ExprOptions expr_options) {
    switch (scanner_.token()) {
        case tokens::kColon:
        case tokens::kRParen:
        case tokens::kSemicolon:
            return {};
        default:
            break;
    }
    ast::Expr *expr = ParseExpr(expr_options);
    if (expr == nullptr) {
        return {};
    }
    
    return ParseExprList(expr, expr_options);
}

std::vector<ast::Expr *> Parser::ParseExprList(ast::Expr *first_expr, ExprOptions expr_options) {
    std::vector<ast::Expr *> list;
    list.push_back(first_expr);
    while (scanner_.token() == tokens::kComma) {
        scanner_.Next();
        ast::Expr *expr = ParseExpr(expr_options);
        if (expr == nullptr) {
            return {};
        }
        list.push_back(expr);
    }
    return list;
}

ast::Expr * Parser::ParseExpr(ExprOptions expr_options) {
    return ParseExpr(0, expr_options);
}

ast::Expr * Parser::ParseExpr(tokens::precedence_t prec, ExprOptions expr_options) {
    ast::Expr *x = ParseUnaryExpr(expr_options);
    if (x == nullptr) {
        return nullptr;
    }
    
    bool is_comparison = false;
    std::vector<ast::Expr *> compare_expr_operands{x};
    std::vector<pos::pos_t> compare_expr_op_starts;
    std::vector<tokens::Token> compare_expr_ops;
    while (true) {
        pos::pos_t op_start = scanner_.token_start();
        tokens::Token op = scanner_.token();
        tokens::precedence_t op_prec = tokens::prececende(op);
        if (op_prec == 0 || op_prec < prec) {
            break;
        }
        switch (op) {
            case tokens::kEql:
            case tokens::kNeq:
            case tokens::kLss:
            case tokens::kLeq:
            case tokens::kGtr:
            case tokens::kGeq:
                is_comparison = true;
                break;
            default:
                break;
        }
        scanner_.Next();
        
        ast::Expr *y = ParseExpr(op_prec + 1, expr_options);
        if (y == nullptr) {
            return nullptr;
        }
        
        if (!is_comparison) {
            x = ast_builder_.Create<ast::BinaryExpr>(x, op_start, op, y);
        } else {
            compare_expr_op_starts.push_back(op_start);
            compare_expr_ops.push_back(op);
            compare_expr_operands.push_back(y);
        }
    }
    if (!is_comparison) {
        return x;
    } else {
        return ast_builder_.Create<ast::CompareExpr>(compare_expr_operands,
                                                     compare_expr_op_starts,
                                                     compare_expr_ops);
    }
}

ast::Expr * Parser::ParseUnaryExpr(ExprOptions expr_options) {
    switch (scanner_.token()) {
        case tokens::kAdd:
        case tokens::kSub:
        case tokens::kNot:
        case tokens::kXor:
        case tokens::kMul:
        case tokens::kRem:
        case tokens::kAnd:
            break;
        default:
            return ParsePrimaryExpr(expr_options);
    }
    
    pos::pos_t op_start = scanner_.token_start();
    tokens::Token op = scanner_.token();
    scanner_.Next();
    
    ast::Expr *x = ParseUnaryExpr(expr_options);
    if (x == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::UnaryExpr>(op_start, op, x);
}

ast::Expr * Parser::ParsePrimaryExpr(ExprOptions expr_options) {
    ast::Expr *primary_expr;
    switch (scanner_.token()) {
        case tokens::kInt:
        case tokens::kChar:
        case tokens::kString:
            primary_expr = ParseBasicLit();
            break;
        case tokens::kLBrack:
        case tokens::kFunc:
        case tokens::kInterface:
        case tokens::kStruct:
            primary_expr = ParseType();
            break;
        case tokens::kIdent:
            primary_expr = ParseIdent();
            break;
        case tokens::kLParen:
            primary_expr = ParseParenExpr();
            break;
        default:
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected expression"));
            scanner_.SkipPastLine();
            return nullptr;
    }
    
    return ParsePrimaryExpr(primary_expr, expr_options);
}

ast::Expr * Parser::ParsePrimaryExpr(ast::Expr *primary_expr, ExprOptions expr_options) {
    bool extended_primary_expr = true;
    while (extended_primary_expr) {
        switch (scanner_.token()) {
            case tokens::kPeriod:
                scanner_.Next();
                if (scanner_.token() == tokens::kIdent) {
                    primary_expr = ParseSelectionExpr(primary_expr);
                } else if (scanner_.token() == tokens::kLss) {
                    primary_expr = ParseTypeAssertExpr(primary_expr);
                } else {
                    issues_.push_back(issues::Issue(issues::Origin::Parser,
                                                    issues::Severity::Fatal,
                                                    scanner_.token_start(),
                                                    "expected identifier or '<'"));
                    scanner_.SkipPastLine();
                    return nullptr;
                }
                break;
            case tokens::kLBrack:
                primary_expr = ParseIndexExpr(primary_expr);
                break;
            case tokens::kLParen:
                // call expr without type args
                primary_expr = ParseCallExpr(primary_expr,
                                             pos::kNoPos,
                                             std::vector<ast::Expr *>{},
                                             pos::kNoPos);
                break;
            case tokens::kLBrace:
                if (primary_expr->node_kind() == ast::NodeKind::kFuncType) {
                    primary_expr = ParseFuncLit(static_cast<ast::FuncType *>(primary_expr));
                } else if (expr_options & kDisallowCompositeLit) {
                    return primary_expr;
                } else {
                    primary_expr = ParseCompositeLit(primary_expr);
                }
                break;
            case tokens::kLss:
                if (primary_expr->node_kind() != ast::NodeKind::kIdent &&
                    primary_expr->node_kind() != ast::NodeKind::kSelectionExpr) {
                    return primary_expr;
                } else if (primary_expr->end() + 1 != scanner_.token_start()) {
                    return primary_expr;
                } else {
                    pos::pos_t l_brack = scanner_.token_start();
                    scanner_.Next(/* split_shift_ops= */ true);
                    
                    std::vector<ast::Expr *> type_args;
                    if (scanner_.token() != tokens::kGtr) {
                        ast::Expr *type_arg = ParseType();
                        if (type_arg == nullptr) {
                            return nullptr;
                        }
                        type_args.push_back(type_arg);
                        
                        while (scanner_.token() == tokens::kComma) {
                            scanner_.Next();
                            
                            ast::Expr *type_arg = ParseType();
                            if (type_arg == nullptr) {
                                return nullptr;
                            }
                            type_args.push_back(type_arg);
                        }
                    }
                    
                    if (scanner_.token() != tokens::kGtr) {
                        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                                        issues::Severity::Fatal,
                                                        scanner_.token_start(),
                                                        "expected '>'"));
                        scanner_.SkipPastLine();
                        return nullptr;
                    }
                    pos::pos_t r_brack = scanner_.token_start();
                    scanner_.Next(/* split_shift_ops= */ true);
                    
                    primary_expr = ParsePrimaryExpr(primary_expr,
                                                    l_brack,
                                                    type_args,
                                                    r_brack,
                                                    expr_options);
                }
                break;
            default:
                extended_primary_expr = false;
                break;
        }
        if (primary_expr == nullptr) {
            return nullptr;
        }
    }
    return primary_expr;
}

ast::Expr * Parser::ParsePrimaryExpr(ast::Expr *primary_expr,
                                     pos::pos_t l_brack,
                                     std::vector<ast::Expr *> type_args,
                                     pos::pos_t r_brack,
                                     ExprOptions expr_options) {
    if (scanner_.token() == tokens::kLParen) {
        ast::CallExpr *call_epxr = ParseCallExpr(primary_expr, l_brack, type_args, r_brack);
        if (call_epxr == nullptr) {
            return nullptr;
        }
        return ParsePrimaryExpr(call_epxr, expr_options);
        
    } else {
        ast::TypeInstance *type_instance = ast_builder_.Create<ast::TypeInstance>(primary_expr,
                                                                                  l_brack,
                                                                                  type_args,
                                                                                  r_brack);
        return ParsePrimaryExpr(type_instance, expr_options);
    }
}

ast::ParenExpr * Parser::ParseParenExpr() {
    if (scanner_.token() != tokens::kLParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '('"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_paren = scanner_.token_start();
    scanner_.Next();
    
    ast::Expr *x = ParseExpr(kNoExprOptions);
    if (x == nullptr) {
        return nullptr;
    }
    
    if (scanner_.token() != tokens::kRParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ')'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_paren = scanner_.token_start();
    scanner_.Next();
    
    return ast_builder_.Create<ast::ParenExpr>(l_paren, x, r_paren);
}

ast::SelectionExpr * Parser::ParseSelectionExpr(ast::Expr *accessed) {
    ast::Ident *selection = ParseIdent();
    if (selection == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::SelectionExpr>(accessed, selection);
}

ast::TypeAssertExpr * Parser::ParseTypeAssertExpr(ast::Expr *x) {
    if (scanner_.token() != tokens::kLss) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '<'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_angle = scanner_.token_start();
    scanner_.Next();
    
    ast::Expr *type = nullptr;
    if (scanner_.token() == tokens::kType) {
        scanner_.Next();
    } else {
        type = ParseType();
        if (type == nullptr) {
            return nullptr;
        }
    }
    
    if (scanner_.token() != tokens::kGtr) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '>'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_angle = scanner_.token_start();
    scanner_.Next();
    
    return ast_builder_.Create<ast::TypeAssertExpr>(x, l_angle, type, r_angle);
}

ast::IndexExpr * Parser::ParseIndexExpr(ast::Expr *accessed) {
    if (scanner_.token() != tokens::kLBrack) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '['"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_brack = scanner_.token_start();
    scanner_.Next();
    
    ast::Expr *index = ParseExpr(kNoExprOptions);
    if (index == nullptr) {
        return nullptr;
    }
    
    if (scanner_.token() != tokens::kRBrack) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ']'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_brack = scanner_.token_start();
    scanner_.Next();
    
    return ast_builder_.Create<ast::IndexExpr>(accessed, l_brack, index, r_brack);
}

ast::CallExpr * Parser::ParseCallExpr(ast::Expr *func,
                                      pos::pos_t l_brack,
                                      std::vector<ast::Expr *> type_args,
                                      pos::pos_t r_brack) {
    if (scanner_.token() != tokens::kLParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '('"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_paren = scanner_.token_start();
    scanner_.Next();
    
    std::vector<ast::Expr *> args = ParseExprList(kNoExprOptions);
    
    if (scanner_.token() != tokens::kRParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ')'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_paren = scanner_.token_start();
    scanner_.Next();
    
    return ast_builder_.Create<ast::CallExpr>(func,
                                              l_brack, type_args, r_brack,
                                              l_paren, args, r_paren);
}

ast::FuncLit * Parser::ParseFuncLit(ast::FuncType *func_type) {
    ast::BlockStmt *body = ParseBlockStmt();
    if (body == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::FuncLit>(func_type, body);
}

ast::CompositeLit * Parser::ParseCompositeLit(ast::Expr *type) {
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_brace = scanner_.token_start();
    scanner_.Next();
    
    std::vector<ast::Expr *> values;
    while (scanner_.token() != tokens::kRBrace) {
        ast::Expr *element = ParseCompositeLitElement();
        if (element == nullptr) {
            return nullptr;
        }
        values.push_back(element);
        
        if (scanner_.token() == tokens::kRBrace) {
            break;
        }
        if (scanner_.token() != tokens::kComma) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected ',' or '}'"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        scanner_.Next();
    }
    pos::pos_t r_brace = scanner_.token_start();
    scanner_.Next();
    
    return ast_builder_.Create<ast::CompositeLit>(type, l_brace, values, r_brace);
}

ast::Expr * Parser::ParseCompositeLitElement() {
    if (scanner_.token() == tokens::kLBrace) {
        return ParseCompositeLit(nullptr);
    }
    
    ast::Expr *key = nullptr;
    if (scanner_.token() == tokens::kLBrace) {
        key = ParseCompositeLit(nullptr);
    } else {
        key = ParseExpr(kNoExprOptions);
    }
    if (key == nullptr) {
        return nullptr;
    }
    
    if (scanner_.token() != tokens::kColon) {
        return key;
    }
    pos::pos_t colon = scanner_.token_start();
    scanner_.Next();
    
    ast::Expr *value = nullptr;
    if (scanner_.token() == tokens::kLBrace) {
        value = ParseCompositeLit(nullptr);
    } else {
        value = ParseExpr(kNoExprOptions);
    }
    if (value == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::KeyValueExpr>(key, colon, value);
}

bool Parser::CanStartType(tokens::Token token) {
    switch (token) {
        case tokens::kLBrack:
        case tokens::kFunc:
        case tokens::kInterface:
        case tokens::kStruct:
        case tokens::kMul:
        case tokens::kRem:
        case tokens::kIdent:
            return true;
        default:
            return false;
    }
}

ast::Expr * Parser::ParseType() {
    switch (scanner_.token()) {
        case tokens::kLBrack:
            return ParseArrayType();
        case tokens::kFunc:
            return ParseFuncType();
        case tokens::kInterface:
            return ParseInterfaceType();
        case tokens::kStruct:
            return ParseStructType();
        case tokens::kMul:
        case tokens::kRem:
            return ParsePointerType();
        case tokens::kIdent:
            return ParseType(ParseIdent(/* split_shift_ops= */ true));
        default:
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected type"));
            scanner_.SkipPastLine();
            return nullptr;
    }
}

ast::Expr * Parser::ParseType(ast::Ident *ident) {
    ast::Expr *type = ident;
    
    if (scanner_.token() == tokens::kPeriod) {
        scanner_.Next();

        ast::Ident *selection = ParseIdent(/* split_shift_ops= */ true);
        if (selection == nullptr) {
            return nullptr;
        }
        type = ast_builder_.Create<ast::SelectionExpr>(type, selection);
    }
    
    if (scanner_.token() == tokens::kLss) {
        type = ParseTypeInstance(type);
    }
    
    return type;
}

ast::ArrayType * Parser::ParseArrayType() {
    if (scanner_.token() != tokens::kLBrack) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '['"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_brack = scanner_.token_start();
    scanner_.Next();
    
    ast::Expr *len = nullptr;
    if (scanner_.token() != tokens::kRBrack) {
        len = ParseExpr(kNoExprOptions);
        if (len == nullptr) {
            return nullptr;
        }
    }
    
    if (scanner_.token() != tokens::kRBrack) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ']'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_brack = scanner_.token_start();
    scanner_.Next();
    
    ast::Expr *element_type = ParseType();
    if (element_type == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::ArrayType>(l_brack, len, r_brack, element_type);
}

ast::FuncType * Parser::ParseFuncType() {
    if (scanner_.token() != tokens::kFunc) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'func'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t func = scanner_.token_start();
    scanner_.Next();
    
    ast::FieldList *params = ParseFuncFieldList(kExpectParen);
    if (params == nullptr) {
        return nullptr;
    }
    
    ast::FieldList *results = nullptr;
    if (scanner_.token() == tokens::kLParen ||
        scanner_.token() == tokens::kIdent ||
        CanStartType(scanner_.token())) {
        results = ParseFuncFieldList(kNoFuncFieldListOptions);
        if (results == nullptr) {
            return nullptr;
        }
    }
    
    return ast_builder_.Create<ast::FuncType>(func, params, results);
}

ast::InterfaceType * Parser::ParseInterfaceType() {
    if (scanner_.token() != tokens::kInterface) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'interface'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t interface = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_brace = scanner_.token_start();
    scanner_.Next();
    
    std::vector<ast::Expr *> embedded_interfaces;
    std::vector<ast::MethodSpec *> methods;
    while (scanner_.token() != tokens::kRBrace) {
        if (scanner_.token() == tokens::kIdent) {
            ast::Expr *embedded_interface = ParseEmbdeddedInterface();
            if (embedded_interface == nullptr) {
                return nullptr;
            }
            embedded_interfaces.push_back(embedded_interface);
        } else if (scanner_.token() == tokens::kLParen ||
                   scanner_.token() == tokens::kLss) {
            ast::MethodSpec *method = ParseMethodSpec();
            if (method == nullptr) {
                return nullptr;
            }
            methods.push_back(method);
        } else {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected type name, '(' or '<'"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        if (scanner_.token() != tokens::kSemicolon) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected ';' or new line"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        scanner_.Next();
    }
    pos::pos_t r_brace = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    return ast_builder_.Create<ast::InterfaceType>(interface,
                                                   l_brace,
                                                   embedded_interfaces,
                                                   methods,
                                                   r_brace);
}

ast::Expr * Parser::ParseEmbdeddedInterface() {
    ast::Ident *ident = ParseIdent();
    if (ident == nullptr) {
        return nullptr;
    }
    return ParseType(ident);
}

ast::MethodSpec * Parser::ParseMethodSpec() {
    if (scanner_.token() != tokens::kLParen &&
        scanner_.token() != tokens::kLss) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '()' or '<>'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t kind_start = scanner_.token_start();
    tokens::Token kind = scanner_.token();
    scanner_.Next();
    
    ast::Ident *instance_type_param = nullptr;
    if (scanner_.token() == tokens::kIdent) {
        instance_type_param = ParseIdent();
    }
    
    if (kind == tokens::kLParen && scanner_.token() != tokens::kRParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ')'"));
        scanner_.SkipPastLine();
        return nullptr;
    } else if (kind == tokens::kLss && scanner_.token() != tokens::kGtr) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '>'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    scanner_.Next();
    
    ast::Ident *name = ParseIdent();
    if (name == nullptr) {
        return nullptr;
    }
    
    ast::FieldList *params = ParseFuncFieldList(kExpectParen);
    if (params == nullptr) {
        return nullptr;
    }
    
    ast::FieldList *results = nullptr;
    if (scanner_.token() == tokens::kLParen ||
        scanner_.token() == tokens::kIdent ||
        CanStartType(scanner_.token())) {
        results = ParseFuncFieldList(kNoFuncFieldListOptions);
        if (results == nullptr) {
            return nullptr;
        }
    }
    
    return ast_builder_.Create<ast::MethodSpec>(kind_start, kind, instance_type_param,
                                                name, params, results);
}

ast::StructType * Parser::ParseStructType() {
    if (scanner_.token() != tokens::kStruct) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'struct'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t struct_start = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_brace = scanner_.token_start();
    scanner_.Next();
    
    ast::FieldList *fields = ParseStructFieldList();
    if (fields == nullptr) {
        return nullptr;
    }
    
    if (scanner_.token() != tokens::kRBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '}'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_brace = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    return ast_builder_.Create<ast::StructType>(struct_start, l_brace, fields, r_brace);
}

ast::UnaryExpr * Parser::ParsePointerType() {
    if (scanner_.token() != tokens::kMul &&
        scanner_.token() != tokens::kRem) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '*' or '%'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t op_start = scanner_.token_start();
    tokens::Token op = scanner_.token();
    scanner_.Next();
    
    ast::Expr *element_type = ParseType();
    if (element_type == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::UnaryExpr>(op_start, op, element_type);
}

ast::TypeInstance * Parser::ParseTypeInstance(ast::Expr *type) {
    if (scanner_.token() != tokens::kLss) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '<'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_brack = scanner_.token_start();
    scanner_.Next();
    
    std::vector<ast::Expr *> type_args;
    ast::Expr *first_type_arg = ParseType();
    if (first_type_arg == nullptr) {
        return nullptr;
    }
    type_args.push_back(first_type_arg);
    
    while (scanner_.token() == tokens::kComma) {
        scanner_.Next();
        
        ast::Expr *type_arg = ParseType();
        if (type_arg == nullptr) {
            return nullptr;
        }
        type_args.push_back(type_arg);
    }
    
    if (scanner_.token() != tokens::kGtr) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '<'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_brack = scanner_.token_start();
    scanner_.Next();
    
    return ast_builder_.Create<ast::TypeInstance>(type, l_brack, type_args, r_brack);
}

ast::ExprReceiver * Parser::ParseExprReceiver() {
    if (scanner_.token() != tokens::kLParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '('"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_paren = scanner_.token_start();
    scanner_.Next();
    
    ast::Ident *name = nullptr;
    tokens::Token pointer = tokens::kIllegal;
    ast::Ident *type_name;
    if (scanner_.token() == tokens::kIdent) {
        ast::Ident *ident = ParseIdent();
        if (scanner_.token() == tokens::kMul || scanner_.token() == tokens::kRem) {
            name = ident;
            pointer = scanner_.token();
            scanner_.Next();
            type_name = ParseIdent();
        } else if (scanner_.token() == tokens::kIdent) {
            name = ident;
            type_name = ParseIdent();
        } else {
            type_name = ident;
        }
    } else if (scanner_.token() == tokens::kMul || scanner_.token() == tokens::kRem) {
        pointer = scanner_.token();
        scanner_.Next();
        type_name = ParseIdent();
    } else {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected identifier, '*' or '%'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    if (type_name == nullptr) {
        return nullptr;
    }
    
    std::vector<ast::Ident *> type_parameter_names;
    if (scanner_.token() == tokens::kLss) {
        scanner_.Next();
        
        type_parameter_names = ParseIdentList();
        if (type_parameter_names.empty()) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected at least one type parameter name"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        
        if (scanner_.token() != tokens::kGtr) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected '>'"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        scanner_.Next();
    }
    
    if (scanner_.token() != tokens::kRParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ')'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_paren = scanner_.token_start();
    scanner_.Next();
    
    return ast_builder_.Create<ast::ExprReceiver>(l_paren,
                                                  name,
                                                  pointer, type_name, type_parameter_names,
                                                  r_paren);
}

ast::TypeReceiver * Parser::ParseTypeReceiver() {
    if (scanner_.token() != tokens::kLss) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '<'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_brack = scanner_.token_start();
    scanner_.Next();
    
    ast::Ident *type_name = ParseIdent();
    if (type_name == nullptr) {
        return nullptr;
    }
    
    std::vector<ast::Ident *> type_parameter_names;
    if (scanner_.token() == tokens::kLss) {
        scanner_.Next();
        
        type_parameter_names = ParseIdentList(/* split_shift_ops= */ true);
        if (type_parameter_names.empty()) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected at least one type parameter name"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        
        if (scanner_.token() != tokens::kGtr) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected '>'"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        scanner_.Next();
    }
    
    if (scanner_.token() != tokens::kGtr) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '>'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_brack = scanner_.token_start();
    scanner_.Next();
    
    return ast_builder_.Create<ast::TypeReceiver>(l_brack,
                                                  type_name, type_parameter_names,
                                                  r_brack);
}

ast::FieldList * Parser::ParseFuncFieldList(FuncFieldListOptions options) {
    bool has_paren = (scanner_.token() == tokens::kLParen);
    if ((options & kExpectParen) != 0 && !has_paren) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '('"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_paren = pos::kNoPos;
    std::vector<ast::Field *> field_list;
    pos::pos_t r_paren = pos::kNoPos;
    if (has_paren) {
        l_paren = scanner_.token_start();
        scanner_.Next();
        
        if (scanner_.token() == tokens::kRParen) {
            r_paren = scanner_.token_start();
            scanner_.Next();
            
            return ast_builder_.Create<ast::FieldList>(l_paren, field_list, r_paren);
        }
    }
        
    std::vector<ast::Field *> fields = ParseFuncFields();
    if (fields.empty()) {
        return nullptr;
    }
    for (ast::Field *field : fields) {
        field_list.push_back(field);
    }
    if (!has_paren) {
        return ast_builder_.Create<ast::FieldList>(l_paren, field_list, r_paren);
    }
    
    if (has_paren) {
        if (scanner_.token() != tokens::kRParen) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected ')'"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        r_paren = scanner_.token_start();
        scanner_.Next(/* split_shift_ops= */ true);
    }
    
    return ast_builder_.Create<ast::FieldList>(l_paren, field_list, r_paren);
}

std::vector<ast::Field *> Parser::ParseFuncFields() {
    bool has_named_fields = false;
    std::vector<ast::Field *> fields;
    std::vector<ast::Ident *> idents;
    pos::pos_t first_field = scanner_.token_start();
    
    auto parse_unnamed_func_fields =
        [&](bool continue_type_after_last_ident) -> std::vector<ast::Field *> {
        if (has_named_fields) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            first_field,
                                            "can not mix named and unnamed arguments"));
            scanner_.SkipPastLine();
            return std::vector<ast::Field *>{};
        }
        for (int i = 0; i < idents.size(); i++) {
            if (i == idents.size() - 1 && continue_type_after_last_ident) {
                break;
            }
            ast::Ident *ident = idents.at(i);
            fields.push_back(ast_builder_.Create<ast::Field>(std::vector<ast::Ident *>{}, ident));
        }
        if (continue_type_after_last_ident) {
            ast::Expr *type = ParseType(idents.back());
            if (!type) {
                return std::vector<ast::Field *>{};
            }
            fields.push_back(ast_builder_.Create<ast::Field>(std::vector<ast::Ident *>{}, type));
            if (scanner_.token() == tokens::kComma) {
                scanner_.Next();
            } else {
                return fields;
            }
        }
        while (true) {
            ast::Expr *type = ParseType();
            if (type == nullptr) {
                return std::vector<ast::Field *>{};
            }
            fields.push_back(ast_builder_.Create<ast::Field>(std::vector<ast::Ident *>{}, type));
            
            if (scanner_.token() == tokens::kComma) {
                scanner_.Next();
            } else {
                return fields;
            }
        }
    };
    
    while (true) {
        if (scanner_.token() != tokens::kIdent) {
            return parse_unnamed_func_fields(false);
        }
        ast::Ident *ident = ParseIdent();
        
        if (scanner_.token() == tokens::kComma) {
            scanner_.Next();
            idents.push_back(ident);
            continue;
        }
        
        if (CanStartType(scanner_.token())) {
            has_named_fields = true;
            idents.push_back(ident);
            
            ast::Expr *type = ParseType();
            if (type == nullptr) {
                return std::vector<ast::Field *>{};
            }
            fields.push_back(ast_builder_.Create<ast::Field>(idents, type));
            idents.clear();
            if (scanner_.token() == tokens::kComma) {
                scanner_.Next();
            } else {
                return fields;
            }
        } else {
            idents.push_back(ident);
            return parse_unnamed_func_fields(true);
        }
    }
}

ast::FieldList * Parser::ParseStructFieldList() {
    std::vector<ast::Field *> fields;
    while (scanner_.token() != tokens::kRBrace) {
        ast::Field *field = ParseStructField();
        if (field == nullptr) {
            return nullptr;
        }
        fields.push_back(field);
        
        if (scanner_.token() != tokens::kSemicolon) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected ';' or new line"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        scanner_.Next();
    }
    
    return ast_builder_.Create<ast::FieldList>(pos::kNoPos, fields, pos::kNoPos);
}

ast::Field * Parser::ParseStructField() {
    if (scanner_.token() != tokens::kIdent) {
        ast::Expr *type = ParseType();
        if (type == nullptr) {
            return nullptr;
        }
        return ast_builder_.Create<ast::Field>(std::vector<ast::Ident *>{}, type);
    }
    
    ast::Ident *ident = ParseIdent();
    if (CanStartType(scanner_.token())) {
        ast::Expr *type = ParseType();
        if (type == nullptr) {
            return nullptr;
        }
        return ast_builder_.Create<ast::Field>(std::vector<ast::Ident *>{ident}, type);
    } else if (scanner_.token() != tokens::kComma) {
        ast::Expr *named_type = ParseType(ident);
        if (named_type == nullptr) {
            return nullptr;
        }
        return ast_builder_.Create<ast::Field>(std::vector<ast::Ident *>{}, named_type);
    }
    std::vector<ast::Ident *> names{ident};
    while (scanner_.token() == tokens::kComma) {
        scanner_.Next();
        
        ast::Ident *name = ParseIdent();
        if (name == nullptr) {
            return nullptr;
        }
        names.push_back(name);
    }
    
    ast::Expr *type = ParseType();
    if (type == nullptr) {
        return nullptr;
    }
    
    return ast_builder_.Create<ast::Field>(names, type);
}

ast::TypeParamList * Parser::ParseTypeParamList() {
    if (scanner_.token() != tokens::kLss) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '<'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t l_angle = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    std::vector<ast::TypeParam *> type_params;
    if (scanner_.token() != tokens::kGtr) {
        ast::TypeParam *type_param = ParseTypeParam();
        if (type_param == nullptr) {
            return nullptr;
        }
        type_params.push_back(type_param);
        
        while (scanner_.token() == tokens::kComma) {
            scanner_.Next();
            
            ast::TypeParam *type_param = ParseTypeParam();
            if (type_param == nullptr) {
                return nullptr;
            }
            type_params.push_back(type_param);
        }
    }
    
    if (scanner_.token() != tokens::kGtr) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '>'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t r_angle = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    return ast_builder_.Create<ast::TypeParamList>(l_angle, type_params, r_angle);
}

ast::TypeParam * Parser::ParseTypeParam() {
    ast::Ident *name = ParseIdent();
    if (name == nullptr) {
        return nullptr;
    }
    
    ast::Expr *type = nullptr;
    if (CanStartType(scanner_.token())) {
        type = ParseType();
        if (type == nullptr) {
            return nullptr;
        }
    }
    return ast_builder_.Create<ast::TypeParam>(name, type);
}

ast::BasicLit * Parser::ParseBasicLit() {
    switch (scanner_.token()) {
        case tokens::kInt:
        case tokens::kChar:
        case tokens::kString:{
            pos::pos_t value_start = scanner_.token_start();
            std::string value = scanner_.token_string();
            tokens::Token kind = scanner_.token();
            scanner_.Next();
            
            return ast_builder_.Create<ast::BasicLit>(value_start, value, kind);
        }
        default:
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected literal"));
            scanner_.SkipPastLine();
            return nullptr;
    }
}

std::vector<ast::Ident *> Parser::ParseIdentList(bool split_shift_ops) {
    std::vector<ast::Ident *> list;
    ast::Ident *ident = ParseIdent();
    if (ident == nullptr) {
        return {};
    }
    list.push_back(ident);
    while (scanner_.token() == tokens::kComma) {
        scanner_.Next();
        ast::Ident *ident = ParseIdent(split_shift_ops);
        if (ident == nullptr) {
            return {};
        }
        list.push_back(ident);
    }
    return list;
}

ast::Ident * Parser::ParseIdent(bool split_shift_ops) {
    if (scanner_.token() != tokens::kIdent) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected identifier"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pos::pos_t name_start = scanner_.token_start();
    std::string name = scanner_.token_string();
    scanner_.Next(split_shift_ops);
    return ast_builder_.Create<ast::Ident>(name_start, name);
}

}
}
