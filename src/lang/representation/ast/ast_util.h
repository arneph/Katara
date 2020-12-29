//
//  ast_util.h
//  Katara
//
//  Created by Arne Philipeit on 7/18/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_ast_util_h
#define lang_ast_util_h

#include <functional>

#include "vcg/graph.h"

#include "lang/representation/positions/positions.h"
#include "lang/representation/ast/nodes.h"

namespace lang {
namespace ast {

vcg::Graph NodeToTree(pos::FileSet *file_set, Node *node);

class WalkFunction {
public:
    WalkFunction();
    WalkFunction(std::function<WalkFunction(Node *)> f);
    
    WalkFunction operator()(Node *node);
    explicit operator bool() const;
    
private:
    std::function<WalkFunction(Node *)> f_;
};

void Walk(Node *node, WalkFunction f);
void Walk(File *file, WalkFunction f);

void Walk(GenDecl *gen_decl, WalkFunction f);
void Walk(ImportSpec *import_spec, WalkFunction f);
void Walk(ValueSpec *value_spec, WalkFunction f);
void Walk(TypeSpec *type_spec, WalkFunction f);
void Walk(FuncDecl *func_decl, WalkFunction f);

void Walk(BlockStmt *block_stmt, WalkFunction f);
void Walk(DeclStmt *decl_stmt, WalkFunction f);
void Walk(AssignStmt *assign_stmt, WalkFunction f);
void Walk(ExprStmt *expr_stmt, WalkFunction f);
void Walk(IncDecStmt *inc_dec_stmt, WalkFunction f);
void Walk(ReturnStmt *return_stmt, WalkFunction f);
void Walk(IfStmt *if_stmt, WalkFunction f);
void Walk(ExprSwitchStmt *switch_stmt, WalkFunction f);
void Walk(TypeSwitchStmt *switch_stmt, WalkFunction f);
void Walk(CaseClause *case_clause, WalkFunction f);
void Walk(ForStmt *for_stmt, WalkFunction f);
void Walk(LabeledStmt *labeled_stmt, WalkFunction f);
void Walk(BranchStmt *branch_stmt, WalkFunction f);

void Walk(UnaryExpr *unary_expr, WalkFunction f);
void Walk(BinaryExpr *binary_expr, WalkFunction f);
void Walk(CompareExpr *compare_expr, WalkFunction f);
void Walk(ParenExpr *paren_expr, WalkFunction f);
void Walk(SelectionExpr *selection_expr, WalkFunction f);
void Walk(TypeAssertExpr *type_assert_expr, WalkFunction f);
void Walk(IndexExpr *index_expr, WalkFunction f);
void Walk(CallExpr *call_expr, WalkFunction f);
void Walk(FuncLit *func_lit, WalkFunction f);
void Walk(CompositeLit *composite_lit, WalkFunction f);
void Walk(KeyValueExpr *key_value_expr, WalkFunction f);

void Walk(ArrayType *array_type, WalkFunction f);
void Walk(FuncType *func_type, WalkFunction f);
void Walk(InterfaceType *interface_type, WalkFunction f);
void Walk(MethodSpec *method_spec, WalkFunction f);
void Walk(StructType *struct_type, WalkFunction f);
void Walk(TypeInstance *type_instance, WalkFunction f);

void Walk(ExprReceiver *receiver, WalkFunction f);
void Walk(TypeReceiver *receiver, WalkFunction f);
void Walk(FieldList *field_list, WalkFunction f);
void Walk(Field *field, WalkFunction f);
void Walk(TypeParamList *type_param_list, WalkFunction f);
void Walk(TypeParam *type_param, WalkFunction f);

void Walk(BasicLit *basic_lit, WalkFunction f);
void Walk(Ident *ident, WalkFunction f);

ast::Expr * Unparen(ast::Expr *expr);

}
}

#endif /* lang_ast_util_h */
