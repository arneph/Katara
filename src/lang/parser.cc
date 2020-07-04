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

std::unique_ptr<ast::File> Parser::ParseFile(std::string file_contents,
                                             std::vector<Error>& errors) {
    Parser parser(file_contents, errors);
    return parser.ParseFile();
}

Parser::Parser(std::string file_contents,
               std::vector<Error>& errors) : scanner_(file_contents), errors_(errors) {}

std::unique_ptr<ast::File> Parser::ParseFile() {
    auto file = std::make_unique<ast::File>();
    
    while (scanner_.token() != token::kEOF) {
        auto decl = ParseDecl();
        if (decl) {
            file->decls_.push_back(std::move(decl));
        }
        if (scanner_.token() != token::kSemicolon) {
            errors_.push_back(Error{
                scanner_.token_start(),
                "expected ';' or new line"
            });
            scanner_.SkipPastLine();
            continue;
        }
        scanner_.Next();
    }
    
    return file;
}

std::unique_ptr<ast::Decl> Parser::ParseDecl() {
    switch (scanner_.token()) {
        case token::kConst:
        case token::kVar:
        case token::kType:
            return ParseGenDecl();
        case token::kFunc:
            return ParseFuncDecl();
        default:
            errors_.push_back(Error{
                scanner_.token_start(),
                "expected 'const', 'var', 'type', or 'func'"
            });
            scanner_.SkipPastLine();
            return nullptr;
    }
}

std::unique_ptr<ast::GenDecl> Parser::ParseGenDecl() {
    auto gen_decl = std::make_unique<ast::GenDecl>();
    
    gen_decl->tok_start_ = scanner_.token_start();
    gen_decl->tok_ = scanner_.token();
    scanner_.Next();
    
    if (scanner_.token() == token::kLParen) {
        gen_decl->l_paren_ = scanner_.token_start();
        scanner_.Next();
        while (scanner_.token() != token::kRParen) {
            auto spec = ParseSpec(gen_decl->tok_);
            if (spec) {
                gen_decl->specs_.push_back(std::move(spec));
            }
            if (scanner_.token() != token::kSemicolon) {
                errors_.push_back(Error{
                    scanner_.token_start(),
                    "expected ';' or new line"
                });
                return nullptr;
            }
            scanner_.Next();
        }
        if (scanner_.token() != token::kRParen) {
            errors_.push_back(Error{
                scanner_.token_start(),
                "expected ')'"
            });
            return nullptr;
        }
        gen_decl->r_paren_ = scanner_.token_end();
        scanner_.Next();
        
    } else {
        auto spec = ParseSpec(gen_decl->tok_);
        if (!spec) {
            return nullptr;
        }
        gen_decl->specs_.push_back(std::move(spec));
    }
    
    return gen_decl;
}

std::unique_ptr<ast::Spec> Parser::ParseSpec(token::Token spec_type) {
    switch (spec_type) {
        case token::kConst:
        case token::kVar:
            return ParseValueSpec();
        case token::kType:
            return ParseTypeSpec();
        default:
            throw "unexpected spec type";
    }
}

std::unique_ptr<ast::ValueSpec> Parser::ParseValueSpec() {
    auto value_spec = std::make_unique<ast::ValueSpec>();
    
    auto names = ParseIdentList();
    if (names.empty()) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    value_spec->names_ = std::move(names);
    
    if (scanner_.token() != token::kAssign) {
        auto type = ParseExpr();
        if (!type) {
            scanner_.SkipPastLine();
            return nullptr;
        }
        value_spec->type_ = std::move(type);
    }
    
    if (scanner_.token() == token::kAssign) {
        scanner_.Next();
        auto values = ParseExprList();
        if (values.empty()) {
            scanner_.SkipPastLine();
            return nullptr;
        }
        value_spec->values_ = std::move(values);
    }
    
    return value_spec;
}

std::unique_ptr<ast::TypeSpec> Parser::ParseTypeSpec() {
    auto type_spec = std::make_unique<ast::TypeSpec>();
    
    auto name = ParseIdent();
    if (!name) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    type_spec->name_ = std::move(name);
    
    auto type = ParseExpr();
    if (!type) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    type_spec->type_ = std::move(type);
    
    return type_spec;
}

std::unique_ptr<ast::FuncDecl> Parser::ParseFuncDecl() {
    auto func_decl = std::make_unique<ast::FuncDecl>();
    func_decl->type_ = std::make_unique<ast::FuncType>();
    func_decl->type_->func_ = scanner_.token_start();
    scanner_.Next();
    
    auto name = ParseIdent();
    if (!name) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    func_decl->name_ = std::move(name);
    
    auto params = ParseFieldList(/* expect_paren= */ true);
    if (!params) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    func_decl->type_->params_ = std::move(params);
    
    if (scanner_.token() != token::kLBrace) {
        auto results = ParseFieldList(/* expect_paren = */ false);
        if (!results) {
            scanner_.SkipPastLine();
            return nullptr;
        }
        func_decl->type_->results_ = std::move(results);
    }
    
    auto body = ParseBlockStmt();
    if (!body) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    func_decl->body_ = std::move(body);
    
    return func_decl;
}

std::vector<std::unique_ptr<ast::Stmt>> Parser::ParseStmtList() {
    std::vector<std::unique_ptr<ast::Stmt>> list;
    while (scanner_.token() != token::kRBrace && scanner_.token() != token::kCase) {
        auto stmt = ParseStmt();
        if (!stmt) {
            continue;
        }
        list.push_back(std::move(stmt));
        if (scanner_.token() == token::kSemicolon) {
            scanner_.Next();
            continue;
        } else if (scanner_.token() == token::kRBrace ||
                   scanner_.token() == token::kCase) {
            scanner_.Next();
            break;
        } else {
            errors_.push_back(Error{
                scanner_.token_start(),
                "expected ';' or new line"
            });
            scanner_.SkipPastLine();
        }
    }
    return list;
}
           
std::unique_ptr<ast::Stmt> Parser::ParseStmt() {
    switch (scanner_.token()) {
        case token::kLBrace:
            return ParseBlockStmt();
        case token::kConst:
        case token::kVar:
        case token::kType:
            return ParseDeclStmt();
        case token::kReturn:
            return ParseReturnStmt();
        case token::kIf:
            return ParseIfStmt();
        case token::kSwitch:
            return ParseSwitchStmt();
        case token::kFor:
            return ParseForStmt();
        case token::kFallthrough:
        case token::kContinue:
        case token::kBreak:
            return ParseBranchStmt();
        default:
            break;
    }
    
    auto expr = ParseExpr();
    if (expr == nullptr) {
        return nullptr;
    }
    
    switch (scanner_.token()) {
        case token::kColon: {
            ast::Ident *ident_ptr = dynamic_cast<ast::Ident *>(expr.release());
            if (ident_ptr == nullptr) {
                errors_.push_back(Error{
                    expr->start(),
                    "expression can not be used as label"
                });
                scanner_.SkipPastLine();
                return nullptr;
            }
            std::unique_ptr<ast::Ident> ident(ident_ptr);
            return ParseLabeledStmt(std::move(ident));
        }
        default:
            return ParseSimpleStmt(std::move(expr));
    }
}

std::unique_ptr<ast::Stmt> Parser::ParseSimpleStmt() {
    auto expr = ParseExpr();
    if (!expr) {
        return nullptr;
    }
    
    return ParseSimpleStmt(std::move(expr));
}

std::unique_ptr<ast::Stmt> Parser::ParseSimpleStmt(std::unique_ptr<ast::Expr> expr) {
    switch (scanner_.token()) {
        case token::kInc:
        case token::kDec:
            return ParseIncDecStmt(std::move(expr));
        case token::kComma:
        case token::kAddAssign:
        case token::kSubAssign:
        case token::kMulAssign:
        case token::kQuoAssign:
        case token::kRemAssign:
        case token::kAndAssign:
        case token::kOrAssign:
        case token::kXorAssign:
        case token::kShlAssign:
        case token::kShrAssign:
        case token::kAndNotAssign:
        case token::kAssign:
        case token::kDefine:
            return ParseAssignStmt(std::move(expr));
        default:
            return ParseExprStmt(std::move(expr));
    }
}

std::unique_ptr<ast::BlockStmt> Parser::ParseBlockStmt() {
    auto block_stmt = std::make_unique<ast::BlockStmt>();
    
    if (scanner_.token() != token::kLBrace) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected '{'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    block_stmt->l_brace_ = scanner_.token_start();
    scanner_.Next();
    
    block_stmt->stmts_ = ParseStmtList();
    
    if (scanner_.token() != token::kRBrace) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected '}'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    block_stmt->r_brace_ = scanner_.token_start();
    scanner_.Next();
    
    return block_stmt;
}

std::unique_ptr<ast::DeclStmt> Parser::ParseDeclStmt() {
    auto decl_stmt = std::make_unique<ast::DeclStmt>();
    
    auto decl = ParseGenDecl();
    if (!decl) {
        return nullptr;
    }
    decl_stmt->decl_ = std::move(decl);
    
    return decl_stmt;
}

std::unique_ptr<ast::ReturnStmt> Parser::ParseReturnStmt() {
    auto return_stmt = std::make_unique<ast::ReturnStmt>();
    
    if (scanner_.token() != token::kReturn) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected 'return'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    return_stmt->return_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() == token::kSemicolon) {
        return return_stmt;
    }
    
    return_stmt->results_ = ParseExprList();
    
    return return_stmt;
}

std::unique_ptr<ast::IfStmt> Parser::ParseIfStmt() {
    auto if_stmt = std::make_unique<ast::IfStmt>();
    
    if (scanner_.token() != token::kIf) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected 'if'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    if_stmt->if_ = scanner_.token_start();
    scanner_.Next();
    
    auto expr = ParseExpr();
    if (!expr) {
        return nullptr;
    }
    
    if (scanner_.token() == token::kLBrace) {
        if_stmt->cond_ = std::move(expr);
    } else {
        auto init = ParseSimpleStmt(std::move(expr));
        if (!init) {
            return nullptr;
        }
        if_stmt->init_ = std::move(init);
        
        if (scanner_.token() != token::kSemicolon) {
            errors_.push_back(Error{
                scanner_.token_start(),
                "expected ';'"
            });
            scanner_.SkipPastLine();
            return nullptr;
        }
        scanner_.Next();
        
        auto cond = ParseExpr();
        if (!cond) {
            return nullptr;
        }
        if_stmt->cond_ = std::move(cond);
    }
    
    auto body = ParseBlockStmt();
    if (!body) {
        return nullptr;
    }
    if_stmt->body_ = std::move(body);
    
    if (scanner_.token() != token::kElse) {
        return if_stmt;
    }
    scanner_.Next();
    
    if (scanner_.token() != token::kIf &&
        scanner_.token() != token::kLBrace) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected 'if' or '{'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    
    auto else_stmt = ParseStmt();
    if (!else_stmt) {
        return nullptr;
    }
    if_stmt->else_ = std::move(else_stmt);
    
    return if_stmt;
}

std::unique_ptr<ast::SwitchStmt> Parser::ParseSwitchStmt() {
    auto switch_stmt = std::make_unique<ast::SwitchStmt>();
    
    if (scanner_.token() != token::kSwitch) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected 'switch'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    switch_stmt->switch_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() != token::kLBrace) {
        auto expr = ParseExpr();
        if (!expr) {
            return nullptr;
        }
        
        if (scanner_.token() == token::kLBrace) {
            switch_stmt->tag_ = std::move(expr);
        } else {
            auto init = ParseSimpleStmt(std::move(expr));
            if (!init) {
                return nullptr;
            }
            switch_stmt->init_ = std::move(init);
            
            if (scanner_.token() != token::kSemicolon) {
                errors_.push_back(Error{
                    scanner_.token_start(),
                    "expected ';'"
                });
                scanner_.SkipPastLine();
                return nullptr;
            }
            scanner_.Next();
            
            if (scanner_.token() != token::kLBrace) {
                auto tag = ParseExpr();
                if (!tag) {
                    return nullptr;
                }
                switch_stmt->tag_ = std::move(tag);
            }
        }
    }
    
    if (scanner_.token() != token::kLBrace) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected '{'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    scanner_.Next();
    
    switch_stmt->body_ = std::make_unique<ast::BlockStmt>();
    while (scanner_.token() != token::kRBrace) {
        auto clause = ParseCaseClause();
        if (!clause) {
            return nullptr;
        }
        switch_stmt->body_->stmts_.push_back(std::move(clause));
    }
    scanner_.Next();
    
    return switch_stmt;
}

std::unique_ptr<ast::CaseClause> Parser::ParseCaseClause() {
    auto case_clause = std::make_unique<ast::CaseClause>();
    
    if (scanner_.token() != token::kCase &&
        scanner_.token() != token::kDefault) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected 'case' or 'default'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    case_clause->tok_start_ = scanner_.token_start();
    case_clause->tok_ = scanner_.token();
    scanner_.Next();
    
    if (case_clause->tok_ == token::kCase) {
        auto cond_vals = ParseExprList();
        if (cond_vals.empty()) {
            scanner_.SkipPastLine();
            return nullptr;
        }
        case_clause->cond_vals_ = std::move(cond_vals);
    }
    
    if (scanner_.token() != token::kColon) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected ':'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    case_clause->colon_ = scanner_.token_start();
    scanner_.Next();
    
    case_clause->body_ = ParseStmtList();
    
    return case_clause;
}

std::unique_ptr<ast::ForStmt> Parser::ParseForStmt() {
    auto for_stmt = std::make_unique<ast::ForStmt>();
    
    if (scanner_.token() != token::kFor) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected 'for'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    for_stmt->for_ = scanner_.token();
    scanner_.Next();
    
    if (scanner_.token() != token::kLBrace) {
        auto expr = ParseExpr();
        if (!expr) {
            return nullptr;
        }
        
        if (scanner_.token() == token::kLBrace) {
            for_stmt->cond_ = std::move(expr);
        } else {
            auto init = ParseSimpleStmt(std::move(expr));
            if (!init) {
                return nullptr;
            }
            for_stmt->init_ = std::move(init);
            
            if (scanner_.token() != token::kSemicolon) {
                errors_.push_back(Error{
                    scanner_.token_start(),
                    "expected ';'"
                });
                scanner_.SkipPastLine();
                return nullptr;
            }
            scanner_.Next();
            
            auto cond = ParseExpr();
            if (!cond) {
                return nullptr;
            }
            for_stmt->cond_ = std::move(cond);
            
            if (scanner_.token() != token::kSemicolon) {
                errors_.push_back(Error{
                    scanner_.token_start(),
                    "expected ';'"
                });
                scanner_.SkipPastLine();
                return nullptr;
            }
            scanner_.Next();
            
            if (scanner_.token() != token::kLBrace) {
                auto post = ParseSimpleStmt();
                if (!post) {
                    return nullptr;
                }
                auto assign_stmt = dynamic_cast<ast::AssignStmt *>(post.get());
                if (assign_stmt != nullptr &&
                    assign_stmt->tok_ == token::kDefine) {
                    errors_.push_back(Error{
                        assign_stmt->start(),
                        "for loop post statement can not define variables"
                    });
                    return nullptr;
                }
                for_stmt->post_ = std::move(post);
            }
        }
    }
    
    auto body = ParseBlockStmt();
    if (!body) {
        return nullptr;
    }
    for_stmt->body_ = std::move(body);
    
    return for_stmt;
}

std::unique_ptr<ast::BranchStmt> Parser::ParseBranchStmt() {
    auto branch_stmt = std::make_unique<ast::BranchStmt>();
    
    if (scanner_.token() != token::kFallthrough &&
        scanner_.token() != token::kContinue &&
        scanner_.token() != token::kBreak) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected 'fallthrough', 'continue', or 'break'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    branch_stmt->tok_start_ = scanner_.token_start();
    branch_stmt->tok_ = scanner_.token();
    scanner_.Next();
    
    if (branch_stmt->tok_ == token::kContinue ||
        branch_stmt->tok_ == token::kBreak) {
        if (scanner_.token() == token::kIdent) {
            branch_stmt->label_ = ParseIdent();
        }
    }
    
    return branch_stmt;
}

std::unique_ptr<ast::ExprStmt> Parser::ParseExprStmt(std::unique_ptr<ast::Expr> x) {
    auto expr_stmt = std::make_unique<ast::ExprStmt>();
    
    if (dynamic_cast<ast::CallExpr *>(x.get()) == nullptr) {
        errors_.push_back(Error{
            x->start(),
            "expression can not be used as standalone statement"
        });
        return nullptr;
    }
    expr_stmt->x_ = std::move(x);
    
    return expr_stmt;
}

std::unique_ptr<ast::LabeledStmt> Parser::ParseLabeledStmt(std::unique_ptr<ast::Ident> label) {
    auto labeled_stmt = std::make_unique<ast::LabeledStmt>();
    labeled_stmt->label_ = std::move(label);
    
    if (scanner_.token() != token::kColon) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected ':'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    
    auto stmt = ParseStmt();
    if (!stmt) {
        return nullptr;
    }
    labeled_stmt->stmt_ = std::move(stmt);
    
    return labeled_stmt;
}

std::unique_ptr<ast::AssignStmt> Parser::ParseAssignStmt(std::unique_ptr<ast::Expr> first_expr) {
    auto assign_stmt = std::make_unique<ast::AssignStmt>();
    assign_stmt->lhs_ = ParseExprList(std::move(first_expr));
    
    switch (scanner_.token()) {
        case token::kAddAssign:
        case token::kSubAssign:
        case token::kMulAssign:
        case token::kQuoAssign:
        case token::kRemAssign:
        case token::kAndAssign:
        case token::kOrAssign:
        case token::kXorAssign:
        case token::kShlAssign:
        case token::kShrAssign:
        case token::kAndNotAssign:
        case token::kAssign:
        case token::kDefine:
            break;
        default:
            errors_.push_back(Error{
                scanner_.token_start(),
                "expected assignment operator"
            });
            scanner_.SkipPastLine();
            return nullptr;
    }
    assign_stmt->tok_start_ = scanner_.token_start();
    assign_stmt->tok_ = scanner_.token();
    scanner_.Next();
    
    auto rhs = ParseExprList();
    if (rhs.empty()) {
        return nullptr;
    }
    assign_stmt->rhs_ = std::move(rhs);
    
    return assign_stmt;
}

std::unique_ptr<ast::IncDecStmt> Parser::ParseIncDecStmt(std::unique_ptr<ast::Expr> x) {
    auto inc_dec_stmt = std::make_unique<ast::IncDecStmt>();
    inc_dec_stmt->x_ = std::move(x);
    
    if (scanner_.token() != token::kInc &&
        scanner_.token() != token::kDec) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected '++' or '--'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    inc_dec_stmt->tok_start_ = scanner_.token_start();
    inc_dec_stmt->tok_ = scanner_.token();
    scanner_.Next();
    
    return inc_dec_stmt;
}

std::vector<std::unique_ptr<ast::Expr>> Parser::ParseExprList() {
    auto expr = ParseExpr();
    if (!expr) {
        return {};
    }
    
    return ParseExprList(std::move(expr));
}

std::vector<std::unique_ptr<ast::Expr>> Parser::ParseExprList(std::unique_ptr<ast::Expr> first_expr) {
    std::vector<std::unique_ptr<ast::Expr>> list;
    list.push_back(std::move(first_expr));
    while (scanner_.token() == token::kComma) {
        scanner_.Next();
        auto expr = ParseExpr();
        if (!expr) {
            return {};
        }
        list.push_back(std::move(expr));
    }
    return list;
}

std::unique_ptr<ast::Expr> Parser::ParseExpr() {
    return ParseExpr(0);
}

std::unique_ptr<ast::Expr> Parser::ParseExpr(token::precedence_t prec) {
    switch (scanner_.token()) {
        case token::kAdd:
        case token::kSub:
        case token::kNot:
        case token::kXor:
        case token::kMul:
        case token::kAnd:
            return ParseUnaryExpr();
        default:
            break;
    }
    if (prec > token::kMaxPrecedence) {
        return ParsePrimaryExpr();
    }
    
    auto x = ParseExpr(prec + 1);
    if (!x) {
        return nullptr;
    }
    
    pos::pos_t op_start = scanner_.token_start();
    token::Token op = scanner_.token();
    token::precedence_t op_prec = token::prececende(op);
    if (op_prec <= prec) {
        return x;
    }
    scanner_.Next();
    auto y = ParseExpr(op_prec + 1);
    if (!y) {
        return nullptr;
    }
    auto binary_expr = std::make_unique<ast::BinaryExpr>();
    binary_expr->x_ = std::move(x);
    binary_expr->op_start_ = op_start;
    binary_expr->op_ = op;
    binary_expr->y_ = std::move(y);
    return binary_expr;
}

std::unique_ptr<ast::Expr> Parser::ParseUnaryExpr() {
    switch (scanner_.token()) {
        case token::kAdd:
        case token::kSub:
        case token::kNot:
        case token::kXor:
        case token::kMul:
        case token::kAnd:
            break;
        default:
            return ParsePrimaryExpr();
    }
    
    auto unary_expr = std::make_unique<ast::UnaryExpr>();
    unary_expr->op_start_ = scanner_.token_start();
    unary_expr->op_ = scanner_.token();
    scanner_.Next();
    
    auto x = ParseUnaryExpr();
    if (!x) {
        return nullptr;
    }
    unary_expr->x_ = std::move(x);
    
    return unary_expr;
}

std::unique_ptr<ast::Expr> Parser::ParsePrimaryExpr() {
    std::unique_ptr<ast::Expr> primary_expr;
    switch (scanner_.token()) {
        case token::kInt:
            primary_expr = ParseBasicLit();
            break;
        case token::kLBrack:
            primary_expr = ParseArrayType();
            break;
        case token::kFunc:{
            auto type = ParseFuncType();
            if (scanner_.token() != token::kLBrace) {
                return type;
            }
            primary_expr = ParseFuncLit(std::move(type));
            break;
        }
        case token::kIdent:
            primary_expr = ParseIdent();
            break;
        case token::kLParen:
            primary_expr = ParseParenExpr();
            break;
        default:
            errors_.push_back(Error{
                scanner_.token_start(),
                "expected expression"
            });
            scanner_.SkipPastLine();
            return nullptr;
    }
    bool extended_primary_expr = true;
    while (extended_primary_expr) {
        switch (scanner_.token()) {
            case token::kLBrack:
                primary_expr = ParseIndexExpr(std::move(primary_expr));
                break;
            case token::kLParen:
                primary_expr = ParseCallExpr(std::move(primary_expr));
                break;
            default:
                extended_primary_expr = false;
                break;
        }
        if (!primary_expr) {
            return nullptr;
        }
    }
    return primary_expr;
}

std::unique_ptr<ast::ParenExpr> Parser::ParseParenExpr() {
    auto paren_expr = std::make_unique<ast::ParenExpr>();
    
    if (scanner_.token() != token::kLParen) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected '('"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    paren_expr->l_paren_ = scanner_.token_start();
    scanner_.Next();
    
    auto x = ParseExpr(0);
    if (!x) {
        return nullptr;
    }
    paren_expr->x_ = std::move(x);
    
    if (scanner_.token() != token::kRParen) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected ')'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    paren_expr->r_paren_ = scanner_.token_start();
    scanner_.Next();
    
    return paren_expr;
}

std::unique_ptr<ast::IndexExpr> Parser::ParseIndexExpr(std::unique_ptr<ast::Expr> accessed) {
    auto index_expr = std::make_unique<ast::IndexExpr>();
    index_expr->accessed_ = std::move(accessed);
    
    if (scanner_.token() != token::kLBrack) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected '['"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    index_expr->l_brack_ = scanner_.token_start();
    scanner_.Next();
    
    auto index = ParseExpr(0);
    if (!index) {
        return nullptr;
    }
    index_expr->index_ = std::move(index);
    
    if (scanner_.token() != token::kRBrack) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected ']'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    index_expr->r_brack_ = scanner_.token_start();
    scanner_.Next();
    
    return index_expr;
}

std::unique_ptr<ast::CallExpr> Parser::ParseCallExpr(std::unique_ptr<ast::Expr> func) {
    auto call_expr = std::make_unique<ast::CallExpr>();
    call_expr->func_ = std::move(func);
    
    if (scanner_.token() != token::kLParen) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected '('"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    call_expr->l_paren_ = scanner_.token_start();
    scanner_.Next();
    
    call_expr->args_ = ParseExprList();
    
    if (scanner_.token() != token::kRParen) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected ')'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    call_expr->r_paren_ = scanner_.token_start();
    scanner_.Next();
    
    return call_expr;
}

std::unique_ptr<ast::FuncLit> Parser::ParseFuncLit(std::unique_ptr<ast::FuncType> type) {
    auto func_lit = std::make_unique<ast::FuncLit>();
    func_lit->type_ = std::move(type);
    
    auto body = ParseBlockStmt();
    if (!body) {
        return nullptr;
    }
    func_lit->body_ = std::move(body);
    
    return func_lit;
}

std::unique_ptr<ast::Expr> Parser::ParseType() {
    switch (scanner_.token()) {
        case token::kIdent:
            return ParseIdent();
        case token::kLBrack:
            return ParseArrayType();
        case token::kFunc:
            return ParseFuncType();
        default:
            errors_.push_back(Error{
                scanner_.token_start(),
                "expected type"
            });
            scanner_.SkipPastLine();
            return nullptr;
    }
}

std::unique_ptr<ast::ArrayType> Parser::ParseArrayType() {
    auto array_type = std::make_unique<ast::ArrayType>();
    
    if (scanner_.token() != token::kLBrack) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected '['"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    array_type->l_brack_ = scanner_.token_start();
    scanner_.Next();
    
    auto len = ParseExpr(0);
    if (!len) {
        return nullptr;
    }
    array_type->len_ = std::move(len);
    
    if (scanner_.token() != token::kRBrack) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected ']'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    array_type->r_brack_ = scanner_.token_start();
    scanner_.Next();
    
    auto element_type = ParseExpr(6);
    if (!element_type) {
        return nullptr;
    }
    array_type->element_type_ = std::move(element_type);
    
    return array_type;
}

std::unique_ptr<ast::FuncType> Parser::ParseFuncType() {
    auto func_type = std::make_unique<ast::FuncType>();
    
    if (scanner_.token() != token::kFunc) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected 'func'"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    func_type->func_ = scanner_.token_start();
    scanner_.Next();
    
    auto params = ParseFieldList(true);
    if (!params) {
        return nullptr;
    }
    func_type->params_ = std::move(params);
    
    if (scanner_.token() == token::kIdent ||
        scanner_.token() == token::kLBrack ||
        scanner_.token() == token::kFunc ||
        scanner_.token() == token::kLParen) {
        auto results = ParseFieldList(false);
        if (!results) {
            return nullptr;
        }
        func_type->results_ = std::move(results);
    }
    
    return func_type;
}

std::unique_ptr<ast::FieldList> Parser::ParseFieldList(bool expect_paren) {
    auto field_list = std::make_unique<ast::FieldList>();
    
    bool has_paren = (scanner_.token() == token::kLParen);
    if (expect_paren && !has_paren) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected '('"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    if (has_paren) {
        field_list->l_paren_ = scanner_.token_start();
        scanner_.Next();
    }
    
    auto first_field = ParseField();
    if (!first_field) {
        return nullptr;
    }
    field_list->fields_.push_back(std::move(first_field));
    if (!has_paren) {
        return field_list;
    }
    
    while (scanner_.token() == token::kComma) {
        scanner_.Next();
        
        auto field = ParseField();
        if (!field) {
            return nullptr;
        }
        field_list->fields_.push_back(std::move(field));
    }
    
    if (has_paren) {
        if (scanner_.token() != token::kRParen) {
            errors_.push_back(Error{
                scanner_.token_start(),
                "expected ')'"
            });
            scanner_.SkipPastLine();
            return nullptr;
        }
        field_list->r_paren_ = scanner_.token_start();
        scanner_.Next();
    }
    
    return field_list;
}

std::unique_ptr<ast::Field> Parser::ParseField() {
    auto field = std::make_unique<ast::Field>();
    
    if (scanner_.token() != token::kIdent) {
        auto type = ParseType();
        if (!type) {
            return nullptr;
        }
        field->type_ = std::move(type);
        return field;
    }
    
    auto ident = ParseIdent();
    if (scanner_.token() != token::kComma) {
        switch (scanner_.token()) {
            case token::kIdent:
            case token::kLBrack:
            case token::kFunc:{
                auto type = ParseType();
                if (!type) {
                    return nullptr;
                }
                field->names_.push_back(std::move(ident));
                field->type_ = std::move(type);
                return field;
            }
            default:
                field->type_ = std::move(ident);
                return field;
        }
    }
    field->names_.push_back(std::move(ident));
    
    while (scanner_.token() == token::kComma) {
        scanner_.Next();
        
        auto name = ParseIdent();
        if (!name) {
            return nullptr;
        }
        field->names_.push_back(std::move(name));
    }
    
    auto type = ParseType();
    if (!type) {
        return nullptr;
    }
    field->type_ = std::move(type);
    
    return field;
}

std::unique_ptr<ast::BasicLit> Parser::ParseBasicLit() {
    auto basic_lit = std::make_unique<ast::BasicLit>();
    
    if (scanner_.token() != token::kInt) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected literal"
        });
        scanner_.SkipPastLine();
        return nullptr;
    }
    basic_lit->value_start_ = scanner_.token_start();
    basic_lit->kind_ = scanner_.token();
    basic_lit->value_ = scanner_.token_string();
    scanner_.Next();
    
    return basic_lit;
}

std::vector<std::unique_ptr<ast::Ident>> Parser::ParseIdentList() {
    std::vector<std::unique_ptr<ast::Ident>> list;
    auto ident = ParseIdent();
    if (!ident) {
        return {};
    }
    list.push_back(std::move(ident));
    while (scanner_.token() == token::kComma) {
        scanner_.Next();
        auto ident = ParseIdent();
        if (!ident) {
            return {};
        }
        list.push_back(std::move(ident));
    }
    return list;
}

std::unique_ptr<ast::Ident> Parser::ParseIdent() {
    if (scanner_.token() != token::kIdent) {
        errors_.push_back(Error{
            scanner_.token_start(),
            "expected identifier"
        });
        return nullptr;
    }
    auto ident = std::make_unique<ast::Ident>();
    ident->name_start_ = scanner_.token_start();
    ident->name_ = scanner_.token_string();
    scanner_.Next();
    return ident;
}

}
}
