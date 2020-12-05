//
//  stmt_handler.h
//  Katara
//
//  Created by Arne Philipeit on 11/22/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_stmt_handler_h
#define lang_type_checker_stmt_handler_h

#include <string>
#include <unordered_map>
#include <vector>

#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/info.h"
#include "lang/representation/types/info_builder.h"
#include "lang/processors/issues/issues.h"

namespace lang {
namespace type_checker {

class StmtHandler {
public:
    static void ProcessFuncBody(ast::BlockStmt *body,
                                types::Tuple *func_results,
                                types::InfoBuilder& info_builder,
                                std::vector<issues::Issue>& issues);
    
private:
    struct Context {
        types::Tuple *func_results;
        std::unordered_map<std::string, ast::Stmt *> labels;
        bool can_break;
        bool can_continue;
        bool can_fallthrough;
        bool is_last_stmt_in_block;
    };
    
    StmtHandler(types::InfoBuilder& info_builder,
                std::vector<issues::Issue>& issues)
    : info_(info_builder.info()), info_builder_(info_builder), issues_(issues) {}
    
    void CheckBlockStmt(ast::BlockStmt *block_stmt, Context ctx);
    void CheckStmt(ast::Stmt *stmt, Context ctx);
    void CheckDeclStmt(ast::DeclStmt *stmt);
    void CheckAssignStmt(ast::AssignStmt *assign_stmt);
    void CheckExprStmt(ast::ExprStmt *expr_stmt);
    void CheckIncDecStmt(ast::IncDecStmt *inc_dec_stmt);
    void CheckReturnStmt(ast::ReturnStmt *return_stmt, Context ctx);
    void CheckIfStmt(ast::IfStmt *if_stmt, Context ctx);
    void CheckExprSwitchStmt(ast::SwitchStmt *switch_stmt, Context ctx);
    void CheckExprCaseClause(ast::CaseClause *case_clause, types::Type *tag_type, Context ctx);
    void CheckTypeSwitchStmt(ast::SwitchStmt *switch_stmt, Context ctx);
    void CheckTypeCaseClause(ast::CaseClause *case_clause, types::Type *x_type, Context ctx);
    void CheckForStmt(ast::ForStmt *for_stmt, Context ctx);
    void CheckBranchStmt(ast::BranchStmt *branch_stmt, Context ctx);
    
    types::Info *info_;
    types::InfoBuilder& info_builder_;
    std::vector<issues::Issue>& issues_;
};

}
}

#endif /* lang_type_checker_stmt_handler_h */
