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

std::unique_ptr<ast::File> Parser::ParseFile(pos::File *file,
                                             std::vector<issues::Issue>& issues) {
    scanner::Scanner scanner(file);
    Parser parser(scanner, issues);
    return parser.ParseFile();
}

Parser::Parser(scanner::Scanner& scanner,
               std::vector<issues::Issue>& issues) : scanner_(scanner), issues_(issues) {}

std::unique_ptr<ast::File> Parser::ParseFile() {
    auto file = std::make_unique<ast::File>();
    file->file_start_ = scanner_.token_start();
    
    while (scanner_.token() != tokens::kPackage) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected package declaration"));
        return file;
    }
    scanner_.Next();
    auto name = ParseIdent();
    if (name) {
        file->package_name_ = std::move(name);
    }
    if (scanner_.token() != tokens::kSemicolon) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ';' or new line"));
        return nullptr;
    }
    scanner_.Next();
    
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
        auto decl = ParseDecl();
        if (decl) {
            file->decls_.push_back(std::move(decl));
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
    file->file_end_ = scanner_.token_end();
    
    return file;
}

std::unique_ptr<ast::Decl> Parser::ParseDecl() {
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

std::unique_ptr<ast::GenDecl> Parser::ParseGenDecl() {
    auto gen_decl = std::make_unique<ast::GenDecl>();
    
    gen_decl->tok_start_ = scanner_.token_start();
    gen_decl->tok_ = scanner_.token();
    scanner_.Next();
    
    if (scanner_.token() == tokens::kLParen) {
        gen_decl->l_paren_ = scanner_.token_start();
        scanner_.Next();
        while (scanner_.token() != tokens::kRParen) {
            auto spec = ParseSpec(gen_decl->tok_);
            if (spec) {
                gen_decl->specs_.push_back(std::move(spec));
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

std::unique_ptr<ast::Spec> Parser::ParseSpec(tokens::Token spec_type) {
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

std::unique_ptr<ast::ImportSpec> Parser::ParseImportSpec() {
    auto import_spec = std::make_unique<ast::ImportSpec>();
    
    if (scanner_.token() == tokens::kIdent) {
        auto ident = ParseIdent();
        if (!ident) {
            return nullptr;
        }
        import_spec->name_ = std::move(ident);
    }
    
    if (scanner_.token() != tokens::kString) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected import package path"));
        return nullptr;
    }
    auto path = ParseBasicLit();
    if (!path) {
        return nullptr;
    }
    import_spec->path_ = std::move(path);
    
    return import_spec;
}

std::unique_ptr<ast::ValueSpec> Parser::ParseValueSpec() {
    auto value_spec = std::make_unique<ast::ValueSpec>();
    
    auto names = ParseIdentList();
    if (names.empty()) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    value_spec->names_ = std::move(names);
    
    if (scanner_.token() != tokens::kAssign) {
        auto type = ParseType();
        if (!type) {
            scanner_.SkipPastLine();
            return nullptr;
        }
        value_spec->type_ = std::move(type);
    }
    
    if (scanner_.token() == tokens::kAssign) {
        scanner_.Next();
        auto values = ParseExprList(/* disallow_composite_lit= */ false);
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
    
    if (scanner_.token() == tokens::kLss) {
        auto type_params = ParseTypeParamList();
        if (!type_params) {
            return nullptr;
        }
        type_spec->type_params_ = std::move(type_params);
    }
    
    auto type = ParseType();
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
    
    if (scanner_.token() == tokens::kLParen) {
        auto receiver = ParseFuncFieldList(/* expect_paren= */ true);
        if (!receiver) {
            return nullptr;
        }
        func_decl->receiver_ = std::move(receiver);
    }
    
    auto name = ParseIdent();
    if (!name) {
        return nullptr;
    }
    func_decl->name_ = std::move(name);
    
    if (scanner_.token() == tokens::kLss) {
        auto type_params = ParseTypeParamList();
        if (!type_params) {
            return nullptr;
        }
        func_decl->type_params_ = std::move(type_params);
    }
    
    auto params = ParseFuncFieldList(/* expect_paren= */ true);
    if (!params) {
        scanner_.SkipPastLine();
        return nullptr;
    }
    func_decl->type_->params_ = std::move(params);
    
    if (scanner_.token() != tokens::kLBrace) {
        auto results = ParseFuncFieldList(/* expect_paren = */ false);
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
    while (scanner_.token() != tokens::kRBrace && scanner_.token() != tokens::kCase) {
        auto stmt = ParseStmt();
        if (!stmt) {
            continue;
        }
        list.push_back(std::move(stmt));
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
           
std::unique_ptr<ast::Stmt> Parser::ParseStmt() {
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
    
    auto expr = ParseExpr(/* disallow_composite_lit= */ false);
    if (expr == nullptr) {
        return nullptr;
    }
    
    switch (scanner_.token()) {
        case tokens::kColon: {
            ast::Ident *ident_ptr = dynamic_cast<ast::Ident *>(expr.release());
            if (ident_ptr == nullptr) {
                issues_.push_back(issues::Issue(issues::Origin::Parser,
                                                issues::Severity::Fatal,
                                                expr->start(),
                                                "expression can not be used as label"));
                scanner_.SkipPastLine();
                return nullptr;
            }
            std::unique_ptr<ast::Ident> ident(ident_ptr);
            return ParseLabeledStmt(std::move(ident));
        }
        default:
            return ParseSimpleStmt(std::move(expr),
                                   /* disallow_composite_lit= */ false);
    }
}

std::unique_ptr<ast::Stmt> Parser::ParseSimpleStmt(bool disallow_composite_lit) {
    auto expr = ParseExpr(disallow_composite_lit);
    if (!expr) {
        return nullptr;
    }
    
    return ParseSimpleStmt(std::move(expr),
                           disallow_composite_lit);
}

std::unique_ptr<ast::Stmt> Parser::ParseSimpleStmt(std::unique_ptr<ast::Expr> expr,
                                                   bool disallow_composite_lit) {
    switch (scanner_.token()) {
        case tokens::kInc:
        case tokens::kDec:
            return ParseIncDecStmt(std::move(expr));
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
            return ParseAssignStmt(std::move(expr),
                                   disallow_composite_lit);
        default:
            return ParseExprStmt(std::move(expr));
    }
}

std::unique_ptr<ast::BlockStmt> Parser::ParseBlockStmt() {
    auto block_stmt = std::make_unique<ast::BlockStmt>();
    
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    block_stmt->l_brace_ = scanner_.token_start();
    scanner_.Next();
    
    block_stmt->stmts_ = ParseStmtList();
    
    if (scanner_.token() != tokens::kRBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '}'"));
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
    
    if (scanner_.token() != tokens::kReturn) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'return'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    return_stmt->return_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() == tokens::kSemicolon) {
        return return_stmt;
    }
    
    return_stmt->results_ = ParseExprList(/* disallow_composite_lit= */ false);
    
    return return_stmt;
}

std::unique_ptr<ast::IfStmt> Parser::ParseIfStmt() {
    auto if_stmt = std::make_unique<ast::IfStmt>();
    
    if (scanner_.token() != tokens::kIf) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'if'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    if_stmt->if_ = scanner_.token_start();
    scanner_.Next();
    
    auto expr = ParseExpr(/* disallow_composite_lit= */ true);
    if (!expr) {
        return nullptr;
    }
    
    if (scanner_.token() == tokens::kLBrace) {
        if_stmt->cond_ = std::move(expr);
    } else {
        auto init = ParseSimpleStmt(std::move(expr),
                                    /* disallow_composite_lit= */ true);
        if (!init) {
            return nullptr;
        }
        if_stmt->init_ = std::move(init);
        
        if (scanner_.token() != tokens::kSemicolon) {
            issues_.push_back(issues::Issue(issues::Origin::Parser,
                                            issues::Severity::Fatal,
                                            scanner_.token_start(),
                                            "expected ';'"));
            scanner_.SkipPastLine();
            return nullptr;
        }
        scanner_.Next();
        
        auto cond = ParseExpr(/* disallow_composite_lit= */ true);
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
    
    if (scanner_.token() != tokens::kElse) {
        return if_stmt;
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
    
    auto else_stmt = ParseStmt();
    if (!else_stmt) {
        return nullptr;
    }
    if_stmt->else_ = std::move(else_stmt);
    
    return if_stmt;
}

std::unique_ptr<ast::SwitchStmt> Parser::ParseSwitchStmt() {
    auto switch_stmt = std::make_unique<ast::SwitchStmt>();
    
    if (scanner_.token() != tokens::kSwitch) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'switch'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    switch_stmt->switch_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() != tokens::kLBrace) {
        auto expr = ParseExpr(/* disallow_composite_lit= */ true);
        if (!expr) {
            return nullptr;
        }
        
        if (scanner_.token() == tokens::kLBrace) {
            switch_stmt->tag_ = std::move(expr);
        } else {
            auto init = ParseSimpleStmt(std::move(expr),
                                        /* disallow_composite_lit= */ true);
            if (!init) {
                return nullptr;
            }
            switch_stmt->init_ = std::move(init);
            
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
                auto tag = ParseExpr(/* disallow_composite_lit= */ true);
                if (!tag) {
                    return nullptr;
                }
                switch_stmt->tag_ = std::move(tag);
            }
        }
    }
    
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    scanner_.Next();
    
    switch_stmt->body_ = std::make_unique<ast::BlockStmt>();
    while (scanner_.token() != tokens::kRBrace) {
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
    
    if (scanner_.token() != tokens::kCase &&
        scanner_.token() != tokens::kDefault) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'case' or 'default'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    case_clause->tok_start_ = scanner_.token_start();
    case_clause->tok_ = scanner_.token();
    scanner_.Next();
    
    if (case_clause->tok_ == tokens::kCase) {
        auto cond_vals = ParseExprList(/* disallow_composite_lit= */ false);
        if (cond_vals.empty()) {
            scanner_.SkipPastLine();
            return nullptr;
        }
        case_clause->cond_vals_ = std::move(cond_vals);
    }
    
    if (scanner_.token() != tokens::kColon) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ':'"));
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
    
    if (scanner_.token() != tokens::kFor) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'for'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    for_stmt->for_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() != tokens::kLBrace) {
        auto expr = ParseExpr(/* disallow_composite_lit= */ true);
        if (!expr) {
            return nullptr;
        }
        
        if (scanner_.token() == tokens::kLBrace) {
            for_stmt->cond_ = std::move(expr);
        } else {
            auto init = ParseSimpleStmt(std::move(expr),
                                        /* disallow_composite_lit= */ true);
            if (!init) {
                return nullptr;
            }
            for_stmt->init_ = std::move(init);
            
            if (scanner_.token() != tokens::kSemicolon) {
                issues_.push_back(issues::Issue(issues::Origin::Parser,
                                                issues::Severity::Fatal,
                                                scanner_.token_start(),
                    "expected ';'"));
                scanner_.SkipPastLine();
                return nullptr;
            }
            scanner_.Next();
            
            auto cond = ParseExpr(/* disallow_composite_lit= */ true);
            if (!cond) {
                return nullptr;
            }
            for_stmt->cond_ = std::move(cond);
            
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
                auto post = ParseSimpleStmt(/* disallow_composite_lit= */ true);
                if (!post) {
                    return nullptr;
                }
                auto assign_stmt = dynamic_cast<ast::AssignStmt *>(post.get());
                if (assign_stmt != nullptr &&
                    assign_stmt->tok_ == tokens::kDefine) {
                    issues_.push_back(
                        issues::Issue(issues::Origin::Parser,
                                      issues::Severity::Fatal,
                                      assign_stmt->start(),
                                      "for loop post statement can not define variables"));
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
    branch_stmt->tok_start_ = scanner_.token_start();
    branch_stmt->tok_ = scanner_.token();
    scanner_.Next();
    
    if (branch_stmt->tok_ == tokens::kContinue ||
        branch_stmt->tok_ == tokens::kBreak) {
        if (scanner_.token() == tokens::kIdent) {
            branch_stmt->label_ = ParseIdent();
        }
    }
    
    return branch_stmt;
}

std::unique_ptr<ast::ExprStmt> Parser::ParseExprStmt(std::unique_ptr<ast::Expr> x) {
    auto expr_stmt = std::make_unique<ast::ExprStmt>();
    
    if (dynamic_cast<ast::CallExpr *>(x.get()) == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        x->start(),
                                        "expression can not be used as standalone statement"));
        return nullptr;
    }
    expr_stmt->x_ = std::move(x);
    
    return expr_stmt;
}

std::unique_ptr<ast::LabeledStmt> Parser::ParseLabeledStmt(std::unique_ptr<ast::Ident> label) {
    auto labeled_stmt = std::make_unique<ast::LabeledStmt>();
    labeled_stmt->label_ = std::move(label);
    
    if (scanner_.token() != tokens::kColon) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ':'"));
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

std::unique_ptr<ast::AssignStmt> Parser::ParseAssignStmt(std::unique_ptr<ast::Expr> first_expr,
                                                         bool disallow_composite_lit) {
    auto assign_stmt = std::make_unique<ast::AssignStmt>();
    assign_stmt->lhs_ = ParseExprList(std::move(first_expr),
                                      disallow_composite_lit);
    
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
    assign_stmt->tok_start_ = scanner_.token_start();
    assign_stmt->tok_ = scanner_.token();
    scanner_.Next();
    
    auto rhs = ParseExprList(disallow_composite_lit);
    if (rhs.empty()) {
        return nullptr;
    }
    assign_stmt->rhs_ = std::move(rhs);
    
    return assign_stmt;
}

std::unique_ptr<ast::IncDecStmt> Parser::ParseIncDecStmt(std::unique_ptr<ast::Expr> x) {
    auto inc_dec_stmt = std::make_unique<ast::IncDecStmt>();
    inc_dec_stmt->x_ = std::move(x);
    
    if (scanner_.token() != tokens::kInc &&
        scanner_.token() != tokens::kDec) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '++' or '--'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    inc_dec_stmt->tok_start_ = scanner_.token_start();
    inc_dec_stmt->tok_ = scanner_.token();
    scanner_.Next();
    
    return inc_dec_stmt;
}

std::vector<std::unique_ptr<ast::Expr>> Parser::ParseExprList(bool disallow_composite_lit) {
    switch (scanner_.token()) {
        case tokens::kColon:
        case tokens::kRParen:
        case tokens::kSemicolon:
            return {};
        default:
            break;
    }
    auto expr = ParseExpr(disallow_composite_lit);
    if (!expr) {
        return {};
    }
    
    return ParseExprList(std::move(expr), disallow_composite_lit);
}

std::vector<std::unique_ptr<ast::Expr>> Parser::ParseExprList(std::unique_ptr<ast::Expr> first_expr,
                                                              bool disallow_composite_lit) {
    std::vector<std::unique_ptr<ast::Expr>> list;
    list.push_back(std::move(first_expr));
    while (scanner_.token() == tokens::kComma) {
        scanner_.Next();
        auto expr = ParseExpr(disallow_composite_lit);
        if (!expr) {
            return {};
        }
        list.push_back(std::move(expr));
    }
    return list;
}

std::unique_ptr<ast::Expr> Parser::ParseExpr(bool disallow_composite_lit) {
    return ParseExpr(0, disallow_composite_lit);
}

std::unique_ptr<ast::Expr> Parser::ParseExpr(tokens::precedence_t prec,
                                             bool disallow_composite_lit) {
    auto x = ParseUnaryExpr(disallow_composite_lit);
    if (!x) {
        return nullptr;
    }
    
    while (true) {
        pos::pos_t op_start = scanner_.token_start();
        tokens::Token op = scanner_.token();
        tokens::precedence_t op_prec = tokens::prececende(op);
        if (op_prec == 0 || op_prec < prec) {
            break;
        }
        scanner_.Next();
        
        auto y = ParseExpr(op_prec + 1, disallow_composite_lit);
        if (!y) {
            return nullptr;
        }
        
        auto binary_expr = std::make_unique<ast::BinaryExpr>();
        binary_expr->x_ = std::move(x);
        binary_expr->op_start_ = op_start;
        binary_expr->op_ = op;
        binary_expr->y_ = std::move(y);
        
        x = std::move(binary_expr);
    }
    return x;
}

std::unique_ptr<ast::Expr> Parser::ParseUnaryExpr(bool disallow_composite_lit) {
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
            return ParsePrimaryExpr(disallow_composite_lit);
    }
    
    auto unary_expr = std::make_unique<ast::UnaryExpr>();
    unary_expr->op_start_ = scanner_.token_start();
    unary_expr->op_ = scanner_.token();
    scanner_.Next();
    
    auto x = ParseUnaryExpr(disallow_composite_lit);
    if (!x) {
        return nullptr;
    }
    unary_expr->x_ = std::move(x);
    
    return unary_expr;
}

std::unique_ptr<ast::Expr> Parser::ParsePrimaryExpr(bool disallow_composite_lit) {
    std::unique_ptr<ast::Expr> primary_expr;
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
    
    return ParsePrimaryExpr(std::move(primary_expr),
                            disallow_composite_lit);
}

std::unique_ptr<ast::Expr> Parser::ParsePrimaryExpr(std::unique_ptr<ast::Expr> primary_expr,
                                                    bool disallow_composite_lit) {
    bool extended_primary_expr = true;
    while (extended_primary_expr) {
        switch (scanner_.token()) {
            case tokens::kPeriod:
                scanner_.Next();
                if (scanner_.token() == tokens::kIdent) {
                    primary_expr = ParseSelectionExpr(std::move(primary_expr));
                } else if (scanner_.token() == tokens::kLss) {
                    primary_expr = ParseTypeAssertExpr(std::move(primary_expr));
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
                primary_expr = ParseIndexExpr(std::move(primary_expr));
                break;
            case tokens::kLParen:
                primary_expr = ParseCallExpr(std::move(primary_expr), nullptr);
                break;
            case tokens::kLBrace:
                if (auto func_type_ptr = dynamic_cast<ast::FuncType *>(primary_expr.get());
                    func_type_ptr != nullptr) {
                    primary_expr.release();
                    auto func_type = std::unique_ptr<ast::FuncType>(func_type_ptr);
                    primary_expr = ParseFuncLit(std::move(func_type));
                } else if (disallow_composite_lit) {
                    return primary_expr;
                } else {
                    primary_expr = ParseCompositeLit(std::move(primary_expr));
                }
                break;
            case tokens::kLss:
                if (dynamic_cast<ast::Ident *>(primary_expr.get()) == nullptr &&
                    dynamic_cast<ast::SelectionExpr *>(primary_expr.get()) == nullptr) {
                    return primary_expr;
                } else if (primary_expr->end() + 1 != scanner_.token_start()) {
                    return primary_expr;
                } else {
                    auto type_args = ParseTypeArgList();
                    if (!type_args) {
                        return nullptr;
                    }
                    primary_expr = ParsePrimaryExpr(std::move(primary_expr),
                                                    std::move(type_args),
                                                    disallow_composite_lit);
                }
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

std::unique_ptr<ast::Expr> Parser::ParsePrimaryExpr(std::unique_ptr<ast::Expr> primary_expr,
                                                    std::unique_ptr<ast::TypeArgList> type_args,
                                                    bool disallow_composite_lit) {
    if (scanner_.token() == tokens::kLParen) {
        auto call_epxr = ParseCallExpr(std::move(primary_expr), std::move(type_args));
        if (!call_epxr) {
            return nullptr;
        }
        return ParsePrimaryExpr(std::move(call_epxr),
                                disallow_composite_lit);
        
    } else {
        auto type_instance = std::make_unique<ast::TypeInstance>();
        type_instance->type_ = std::move(primary_expr);
        type_instance->type_args_ = std::move(type_args);
        
        return ParsePrimaryExpr(std::move(type_instance),
                                disallow_composite_lit);
    }
}

std::unique_ptr<ast::ParenExpr> Parser::ParseParenExpr() {
    auto paren_expr = std::make_unique<ast::ParenExpr>();
    
    if (scanner_.token() != tokens::kLParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '('"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    paren_expr->l_paren_ = scanner_.token_start();
    scanner_.Next();
    
    auto x = ParseExpr(/* disallow_composite_lit= */ false);
    if (!x) {
        return nullptr;
    }
    paren_expr->x_ = std::move(x);
    
    if (scanner_.token() != tokens::kRParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ')'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    paren_expr->r_paren_ = scanner_.token_start();
    scanner_.Next();
    
    return paren_expr;
}

std::unique_ptr<ast::SelectionExpr> Parser::ParseSelectionExpr(std::unique_ptr<ast::Expr> accessed) {
    auto selection_expr = std::make_unique<ast::SelectionExpr>();
    selection_expr->accessed_ = std::move(accessed);
    
    auto selection = ParseIdent();
    if (!selection) {
        return nullptr;
    }
    selection_expr->selection_ = std::move(selection);
    
    return selection_expr;
}

std::unique_ptr<ast::TypeAssertExpr> Parser::ParseTypeAssertExpr(std::unique_ptr<ast::Expr> x) {
    auto type_assert_expr = std::make_unique<ast::TypeAssertExpr>();
    type_assert_expr->x_ = std::move(x);
    
    if (scanner_.token() != tokens::kLss) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '<'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    type_assert_expr->l_angle_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() == tokens::kType) {
        scanner_.Next();
    } else {
        auto type = ParseType();
        if (!type) {
            return nullptr;
        }
        type_assert_expr->type_ = std::move(type);
    }
    
    if (scanner_.token() != tokens::kGtr) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '>'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    type_assert_expr->r_angle_ = scanner_.token_start();
    scanner_.Next();
    
    return type_assert_expr;
}

std::unique_ptr<ast::IndexExpr> Parser::ParseIndexExpr(std::unique_ptr<ast::Expr> accessed) {
    auto index_expr = std::make_unique<ast::IndexExpr>();
    index_expr->accessed_ = std::move(accessed);
    
    if (scanner_.token() != tokens::kLBrack) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '['"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    index_expr->l_brack_ = scanner_.token_start();
    scanner_.Next();
    
    auto index = ParseExpr(/* disallow_composite_lit= */ false);
    if (!index) {
        return nullptr;
    }
    index_expr->index_ = std::move(index);
    
    if (scanner_.token() != tokens::kRBrack) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ']'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    index_expr->r_brack_ = scanner_.token_start();
    scanner_.Next();
    
    return index_expr;
}

std::unique_ptr<ast::CallExpr> Parser::ParseCallExpr(std::unique_ptr<ast::Expr> func,
                                                     std::unique_ptr<ast::TypeArgList> type_args) {
    auto call_expr = std::make_unique<ast::CallExpr>();
    call_expr->func_ = std::move(func);
    call_expr->type_args_ = std::move(type_args);
    
    if (scanner_.token() != tokens::kLParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '('"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    call_expr->l_paren_ = scanner_.token_start();
    scanner_.Next();
    
    call_expr->args_ = ParseExprList(/* disallow_composite_lit= */ false);
    
    if (scanner_.token() != tokens::kRParen) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ')'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    call_expr->r_paren_ = scanner_.token_start();
    scanner_.Next();
    
    return call_expr;
}

std::unique_ptr<ast::FuncLit> Parser::ParseFuncLit(std::unique_ptr<ast::FuncType> func_type) {
    auto func_lit = std::make_unique<ast::FuncLit>();
    func_lit->type_ = std::move(func_type);
    
    auto body = ParseBlockStmt();
    if (!body) {
        return nullptr;
    }
    func_lit->body_ = std::move(body);
    
    return func_lit;
}

std::unique_ptr<ast::CompositeLit> Parser::ParseCompositeLit(std::unique_ptr<ast::Expr> type) {
    auto composite_lit = std::make_unique<ast::CompositeLit>();
    composite_lit->type_ = std::move(type);
    
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    composite_lit->l_brace_ = scanner_.token_start();
    scanner_.Next();
    
    while (scanner_.token() != tokens::kRBrace) {
        auto element = ParseCompositeLitElement();
        if (!element) {
            return nullptr;
        }
        composite_lit->values_.push_back(std::move(element));
        
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
    composite_lit->r_brace_ = scanner_.token_start();
    scanner_.Next();
    
    return composite_lit;
}

std::unique_ptr<ast::Expr> Parser::ParseCompositeLitElement() {
    if (scanner_.token() == tokens::kLBrace) {
        return ParseCompositeLit(nullptr);
    }
    
    auto expr = ParseExpr(/* disallow_composite_lit= */ false);
    if (!expr) {
        return nullptr;
    }
    
    if (scanner_.token() != tokens::kColon) {
        return expr;
    }
    auto key_value_expr = std::make_unique<ast::KeyValueExpr>();
    key_value_expr->key_ = std::move(expr);
    key_value_expr->colon_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() == tokens::kLBrace) {
        expr = ParseCompositeLit(nullptr);
    } else {
        expr = ParseExpr(/* disallow_composite_lit= */ false);
    }
    if (!expr) {
        return nullptr;
    }
    key_value_expr->value_ = std::move(expr);
    
    return key_value_expr;
}

std::unique_ptr<ast::Expr> Parser::ParseType() {
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

std::unique_ptr<ast::Expr> Parser::ParseType(std::unique_ptr<ast::Ident> ident) {
    std::unique_ptr<ast::Expr> type = std::move(ident);
    
    if (scanner_.token() == tokens::kPeriod) {
        scanner_.Next();

        auto selection = ParseIdent(/* split_shift_ops= */ true);
        if (!selection) {
            return nullptr;
        }
        auto selection_expr = std::make_unique<ast::SelectionExpr>();
        selection_expr->accessed_ = std::move(type);
        selection_expr->selection_ = std::move(selection);
        
        type = std::move(selection_expr);
    }
    
    if (scanner_.token() == tokens::kLss) {
        type = ParseTypeInstance(std::move(type));
    }
    
    return type;
}

std::unique_ptr<ast::ArrayType> Parser::ParseArrayType() {
    auto array_type = std::make_unique<ast::ArrayType>();
    
    if (scanner_.token() != tokens::kLBrack) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '['"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    array_type->l_brack_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() != tokens::kRBrack) {
        auto len = ParseExpr(/* disallow_composite_lit= */ false);
        if (!len) {
            return nullptr;
        }
        array_type->len_ = std::move(len);
    }
    
    if (scanner_.token() != tokens::kRBrack) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected ']'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    array_type->r_brack_ = scanner_.token_start();
    scanner_.Next();
    
    auto element_type = ParseType();
    if (!element_type) {
        return nullptr;
    }
    array_type->element_type_ = std::move(element_type);
    
    return array_type;
}

std::unique_ptr<ast::FuncType> Parser::ParseFuncType() {
    auto func_type = std::make_unique<ast::FuncType>();
    
    if (scanner_.token() != tokens::kFunc) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'func'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    func_type->func_ = scanner_.token_start();
    scanner_.Next();
    
    auto params = ParseFuncFieldList(/* expect_paren= */ true);
    if (!params) {
        return nullptr;
    }
    func_type->params_ = std::move(params);
    
    switch (scanner_.token()) {
        case tokens::kLParen:
        case tokens::kLBrack:
        case tokens::kFunc:
        case tokens::kInterface:
        case tokens::kStruct:
        case tokens::kMul:
        case tokens::kRem:
        case tokens::kIdent:{
            auto results = ParseFuncFieldList(/* expect_paren= */ false);
            if (!results) {
                return nullptr;
            }
            func_type->results_ = std::move(results);
            break;
        }
        default:
            break;
    }
    
    return func_type;
}

std::unique_ptr<ast::InterfaceType> Parser::ParseInterfaceType() {
    auto interface_type = std::make_unique<ast::InterfaceType>();
    
    if (scanner_.token() != tokens::kInterface) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'interface'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    interface_type->interface_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    interface_type->l_brace_ = scanner_.token_start();
    scanner_.Next();
    
    while (scanner_.token() != tokens::kRBrace) {
        auto method_spec = ParseMethodSpec();
        if (!method_spec) {
            return nullptr;
        }
        interface_type->methods_.push_back(std::move(method_spec));
        
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
    interface_type->r_brace_ = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    return interface_type;
}

std::unique_ptr<ast::MethodSpec> Parser::ParseMethodSpec() {
    auto method_spec = std::make_unique<ast::MethodSpec>();
    
    auto name = ParseIdent();
    if (!name) {
        return nullptr;
    }
    method_spec->name_ = std::move(name);
    
    auto params = ParseFuncFieldList(/* expect_paren= */ true);
    if (!params) {
        return nullptr;
    }
    method_spec->params_ = std::move(params);
    
    switch (scanner_.token()) {
        case tokens::kLParen:
        case tokens::kLBrack:
        case tokens::kFunc:
        case tokens::kInterface:
        case tokens::kStruct:
        case tokens::kMul:
        case tokens::kRem:
        case tokens::kIdent:{
            auto results = ParseFuncFieldList(/* expect_paren= */ false);
            if (!results) {
                return nullptr;
            }
            method_spec->results_ = std::move(results);
            break;
        }
        default:
            break;
    }
    
    return method_spec;
}

std::unique_ptr<ast::StructType> Parser::ParseStructType() {
    auto struct_type = std::make_unique<ast::StructType>();
    
    if (scanner_.token() != tokens::kStruct) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected 'struct'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    struct_type->struct_ = scanner_.token_start();
    scanner_.Next();
    
    if (scanner_.token() != tokens::kLBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '{'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    struct_type->l_brace_ = scanner_.token_start();
    scanner_.Next();
    
    auto fields = ParseStructFieldList();
    if (!fields) {
        return nullptr;
    }
    struct_type->fields_ = std::move(fields);
    
    if (scanner_.token() != tokens::kRBrace) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '}'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    struct_type->r_brace_ = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    return struct_type;
}

std::unique_ptr<ast::UnaryExpr> Parser::ParsePointerType() {
    auto pointer_type = std::make_unique<ast::UnaryExpr>();
    
    if (scanner_.token() != tokens::kMul &&
        scanner_.token() != tokens::kRem) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '*' or '%'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    pointer_type->op_start_ = scanner_.token_start();
    pointer_type->op_ = scanner_.token();
    scanner_.Next();
    
    auto element_type = ParseType();
    if (!element_type) {
        return nullptr;
    }
    pointer_type->x_ = std::move(element_type);
    
    return pointer_type;
}

std::unique_ptr<ast::TypeInstance> Parser::ParseTypeInstance(std::unique_ptr<ast::Expr> type) {
    auto type_instance = std::make_unique<ast::TypeInstance>();
    type_instance->type_ = std::move(type);
    
    auto type_args = ParseTypeArgList();
    if (!type_args) {
        return nullptr;
    }
    type_instance->type_args_ = std::move(type_args);
    
    return type_instance;
}

std::unique_ptr<ast::FieldList> Parser::ParseFuncFieldList(bool expect_paren) {
    auto field_list = std::make_unique<ast::FieldList>();
    
    bool has_paren = (scanner_.token() == tokens::kLParen);
    if (expect_paren && !has_paren) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '('"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    if (has_paren) {
        field_list->l_paren_ = scanner_.token_start();
        scanner_.Next();
        
        if (scanner_.token() == tokens::kRParen) {
            field_list->r_paren_ = scanner_.token_start();
            scanner_.Next();
            
            return field_list;
        }
    }
        
    auto first_fields = ParseFuncFields();
    if (first_fields.empty()) {
        return nullptr;
    }
    for (auto& field : first_fields) {
        field_list->fields_.push_back(std::move(field));
    }
    if (!has_paren) {
        return field_list;
    }
    
    while (scanner_.token() == tokens::kComma) {
        scanner_.Next();
        
        auto fields = ParseFuncFields();
        if (fields.empty()) {
            return nullptr;
        }
        for (auto& field : fields) {
            field_list->fields_.push_back(std::move(field));
        }
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
        field_list->r_paren_ = scanner_.token_start();
        scanner_.Next(/* split_shift_ops= */ true);
    }
    
    return field_list;
}

std::vector<std::unique_ptr<ast::Field>> Parser::ParseFuncFields() {
    if (scanner_.token() != tokens::kIdent) {
        auto type = ParseType();
        if (!type) {
            return {};
        }
        std::vector<std::unique_ptr<ast::Field>> fields;
        fields.emplace_back(std::make_unique<ast::Field>());
        fields.back()->type_ = std::move(type);
        return fields;
    }
    
    auto ident = ParseIdent();
    if (scanner_.token() != tokens::kComma) {
        switch (scanner_.token()) {
            case tokens::kLBrack:
            case tokens::kFunc:
            case tokens::kInterface:
            case tokens::kStruct:
            case tokens::kMul:
            case tokens::kRem:
            case tokens::kIdent:{
                auto type = ParseType();
                if (!type) {
                    return {};
                }
                std::vector<std::unique_ptr<ast::Field>> fields;
                fields.emplace_back(std::make_unique<ast::Field>());
                fields.back()->names_.push_back(std::move(ident));
                fields.back()->type_ = std::move(type);
                return fields;
            }
            default:{
                auto named_type = ParseType(std::move(ident));
                if (!named_type) {
                    return {};
                }
                std::vector<std::unique_ptr<ast::Field>> fields;
                fields.emplace_back(std::make_unique<ast::Field>());
                fields.back()->type_ = std::move(named_type);
                return fields;
            }
        }
    }
    std::vector<std::unique_ptr<ast::Ident>> idents;
    idents.push_back(std::move(ident));
    
    while (scanner_.token() == tokens::kComma) {
        scanner_.Next();
        
        if (scanner_.token() != tokens::kIdent) {
            auto type = ParseType();
            if (!type) {
                return {};
            }
            
            std::vector<std::unique_ptr<ast::Field>> fields;
            for (auto& ident : idents) {
                fields.emplace_back(std::make_unique<ast::Field>());
                fields.back()->type_ = std::move(ident);
            }
            fields.emplace_back(std::make_unique<ast::Field>());
            fields.back()->type_ = std::move(type);
            return fields;
        }
        
        auto ident = ParseIdent();
        if (!ident) {
            return {};
        }
        idents.push_back(std::move(ident));
    }
    
    switch (scanner_.token()) {
        case tokens::kLBrack:
        case tokens::kFunc:
        case tokens::kInterface:
        case tokens::kStruct:
        case tokens::kMul:
        case tokens::kRem:
        case tokens::kIdent:{
            auto type = ParseType();
            if (!type) {
                return {};
            }
            std::vector<std::unique_ptr<ast::Field>> fields;
            fields.emplace_back(std::make_unique<ast::Field>());
            fields.back()->names_ = std::move(idents);
            fields.back()->type_ = std::move(type);
            return fields;
        }
        default:{
            std::vector<std::unique_ptr<ast::Field>> fields;
            for (auto& ident : idents) {
                fields.emplace_back(std::make_unique<ast::Field>());
                fields.back()->type_ = std::move(ident);
            }
            return fields;
        }
    }
}

std::unique_ptr<ast::FieldList> Parser::ParseStructFieldList() {
    auto field_list = std::make_unique<ast::FieldList>();
    
    while (scanner_.token() != tokens::kRBrace) {
        auto field = ParseStructField();
        if (!field) {
            return nullptr;
        }
        field_list->fields_.push_back(std::move(field));
        
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
    
    return field_list;
}

std::unique_ptr<ast::Field> Parser::ParseStructField() {
    auto field = std::make_unique<ast::Field>();
    
    if (scanner_.token() != tokens::kIdent) {
        auto type = ParseType();
        if (!type) {
            return nullptr;
        }
        field->type_ = std::move(type);
        return field;
    }
    
    auto ident = ParseIdent();
    if (scanner_.token() != tokens::kComma) {
        switch (scanner_.token()) {
            case tokens::kLBrack:
            case tokens::kFunc:
            case tokens::kInterface:
            case tokens::kStruct:
            case tokens::kMul:
            case tokens::kRem:
            case tokens::kIdent:{
                auto type = ParseType();
                if (!type) {
                    return nullptr;
                }
                field->names_.push_back(std::move(ident));
                field->type_ = std::move(type);
                return field;
            }
            default:{
                auto named_type = ParseType(std::move(ident));
                if (!named_type) {
                    return nullptr;
                }
                field->type_ = std::move(named_type);
                return field;
            }
        }
    }
    field->names_.push_back(std::move(ident));
    
    while (scanner_.token() == tokens::kComma) {
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

std::unique_ptr<ast::TypeArgList> Parser::ParseTypeArgList() {
    auto type_args = std::make_unique<ast::TypeArgList>();
        
    if (scanner_.token() != tokens::kLss) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '<'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    type_args->l_angle_ = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    if (scanner_.token() != tokens::kGtr) {
        auto type_arg = ParseType();
        if (!type_arg) {
            return nullptr;
        }
        type_args->args_.push_back(std::move(type_arg));
        
        while (scanner_.token() == tokens::kComma) {
            scanner_.Next();
            
            auto type_arg = ParseType();
            if (!type_arg) {
                return nullptr;
            }
            type_args->args_.push_back(std::move(type_arg));
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
    type_args->r_angle_ = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    return type_args;
}

std::unique_ptr<ast::TypeParamList> Parser::ParseTypeParamList() {
    auto type_params = std::make_unique<ast::TypeParamList>();
        
    if (scanner_.token() != tokens::kLss) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected '<'"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    type_params->l_angle_ = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    if (scanner_.token() != tokens::kGtr) {
        auto type_param = ParseTypeParam();
        if (!type_param) {
            return nullptr;
        }
        type_params->params_.push_back(std::move(type_param));
        
        while (scanner_.token() == tokens::kComma) {
            scanner_.Next();
            
            auto type_param = ParseTypeParam();
            if (!type_param) {
                return nullptr;
            }
            type_params->params_.push_back(std::move(type_param));
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
    type_params->r_angle_ = scanner_.token_start();
    scanner_.Next(/* split_shift_ops= */ true);
    
    return type_params;
}

std::unique_ptr<ast::TypeParam> Parser::ParseTypeParam() {
    auto type_param = std::make_unique<ast::TypeParam>();
    
    auto name = ParseIdent();
    if (!name) {
        return nullptr;
    }
    type_param->name_ = std::move(name);
    
    switch (scanner_.token()) {
        case tokens::kLBrack:
        case tokens::kFunc:
        case tokens::kInterface:
        case tokens::kStruct:
        case tokens::kMul:
        case tokens::kRem:
        case tokens::kIdent:{
            auto type = ParseType();
            if (!type) {
                return nullptr;
            }
            type_param->type_ = std::move(type);
            break;
        }
        default:
            break;
    }
    
    return type_param;
}

std::unique_ptr<ast::BasicLit> Parser::ParseBasicLit() {
    switch (scanner_.token()) {
        case tokens::kInt:
        case tokens::kChar:
        case tokens::kString:{
            auto basic_lit = std::make_unique<ast::BasicLit>();
            basic_lit->value_start_ = scanner_.token_start();
            basic_lit->kind_ = scanner_.token();
            basic_lit->value_ = scanner_.token_string();
            scanner_.Next();
            
            return basic_lit;
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

std::vector<std::unique_ptr<ast::Ident>> Parser::ParseIdentList() {
    std::vector<std::unique_ptr<ast::Ident>> list;
    auto ident = ParseIdent();
    if (!ident) {
        return {};
    }
    list.push_back(std::move(ident));
    while (scanner_.token() == tokens::kComma) {
        scanner_.Next();
        auto ident = ParseIdent();
        if (!ident) {
            return {};
        }
        list.push_back(std::move(ident));
    }
    return list;
}

std::unique_ptr<ast::Ident> Parser::ParseIdent(bool split_shift_ops) {
    if (scanner_.token() != tokens::kIdent) {
        issues_.push_back(issues::Issue(issues::Origin::Parser,
                                        issues::Severity::Fatal,
                                        scanner_.token_start(),
                                        "expected identifier"));
        scanner_.SkipPastLine();
        return nullptr;
    }
    auto ident = std::make_unique<ast::Ident>();
    ident->name_start_ = scanner_.token_start();
    ident->name_ = scanner_.token_string();
    scanner_.Next(split_shift_ops);
    return ident;
}

}
}
