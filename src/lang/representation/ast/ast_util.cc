//
//  ast_util.cc
//  Katara
//
//  Created by Arne Philipeit on 7/18/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "ast_util.h"

#include "vcg/node.h"
#include "vcg/edge.h"

namespace lang {
namespace ast {

vcg::Graph NodeToTree(pos::FileSet *file_set, Node *node) {
    vcg::Graph graph;
    std::vector<int64_t> stack;
    int64_t count = 0;
    
    WalkFunction walker = WalkFunction([&](Node *ast_node) -> WalkFunction {
        if (ast_node == nullptr) {
            stack.pop_back();
            return WalkFunction();
        }
        int64_t number = count++;
        std::string title;
        vcg::Color color;
        if (dynamic_cast<Expr *>(ast_node)) {
            title = "expr";
            color = vcg::kTurquoise;
        } else if (dynamic_cast<Stmt *>(ast_node)) {
            title = "stmt";
            color = vcg::kGreen;
        } else if (dynamic_cast<Decl *>(ast_node)) {
            title = "decl";
            color = vcg::kYellow;
        } else {
            title = "node";
            color = vcg::kRed;
        }
        pos::File *file = file_set->FileAt(ast_node->start());
        std::string text = file->contents(ast_node->start(), ast_node->end());
        if (auto it = std::find(text.begin(), text.end(), '\n');
            it != text.end()) {
            text = std::string(text.begin(), it) + "...";
        }
        
        graph.nodes().push_back(vcg::Node(number, title, text, color));

        if (!stack.empty()) {
            graph.edges().push_back(vcg::Edge(stack.back(), number, true));
        }
        
        stack.push_back(number);
        
        return walker;
    });
    
    Walk(node, walker);
    
    return graph;
}

WalkFunction::WalkFunction() {}
WalkFunction::WalkFunction(std::function<WalkFunction(Node *)> f) : f_(f) {}

WalkFunction WalkFunction::operator()(Node *node) {
    return f_(node);
}

WalkFunction::operator bool() const {
    return (bool)f_;
}

void Walk(Node *node, WalkFunction f) {
    if (File *file = dynamic_cast<File *>(node)) {
        Walk(file, f);
    } else if (Expr *expr = dynamic_cast<Expr *>(node)) {
        Walk(expr, f);
    } else if (Stmt *stmt = dynamic_cast<Stmt *>(node)) {
        Walk(stmt, f);
    } else if (Decl *decl = dynamic_cast<Decl *>(node)) {
        Walk(decl, f);
    } else if (ImportSpec *import_spec = dynamic_cast<ImportSpec *>(node)) {
        Walk(import_spec, f);
    } else if (ValueSpec *value_spec = dynamic_cast<ValueSpec *>(node)) {
        Walk(value_spec, f);
    } else if (TypeSpec *type_spec = dynamic_cast<TypeSpec *>(node)) {
        Walk(type_spec, f);
    } else if (MethodSpec *method_spec = dynamic_cast<MethodSpec *>(expr)) {
        Walk(method_spec, f);
    } else if (FieldList *field_list = dynamic_cast<FieldList *>(expr)) {
        Walk(field_list, f);
    } else if (Field *field = dynamic_cast<Field *>(expr)) {
        Walk(field, f);
    } else if (TypeArgList *type_arg_list = dynamic_cast<TypeArgList *>(expr)) {
        Walk(type_arg_list, f);
    } else if (TypeParamList *type_param_list = dynamic_cast<TypeParamList *>(expr)) {
        Walk(type_param_list, f);
    } else if (TypeParam *type_param = dynamic_cast<TypeParam *>(expr)) {
        Walk(type_param, f);
    } else {
        throw "unexpected AST node";
    }
}

void Walk(Expr *expr, WalkFunction f) {
    if (UnaryExpr *unary_expr = dynamic_cast<UnaryExpr *>(expr)) {
        Walk(unary_expr, f);
    } else if (BinaryExpr *binary_expr = dynamic_cast<BinaryExpr *>(expr)) {
        Walk(binary_expr, f);
    } else if (ParenExpr *paren_expr = dynamic_cast<ParenExpr *>(expr)) {
        Walk(paren_expr, f);
    } else if (SelectionExpr *selection_expr = dynamic_cast<SelectionExpr *>(expr)) {
        Walk(selection_expr, f);
    } else if (TypeAssertExpr *type_assert_expr = dynamic_cast<TypeAssertExpr *>(expr)) {
        Walk(type_assert_expr, f);
    } else if (IndexExpr *index_expr = dynamic_cast<IndexExpr *>(expr)) {
        Walk(index_expr, f);
    } else if (CallExpr *call_expr = dynamic_cast<CallExpr *>(expr)) {
        Walk(call_expr, f);
    } else if (FuncLit *func_lit = dynamic_cast<FuncLit *>(expr)) {
        Walk(func_lit, f);
    } else if (CompositeLit *composite_lit = dynamic_cast<CompositeLit *>(expr)) {
        Walk(composite_lit, f);
    } else if (KeyValueExpr *key_value_expr = dynamic_cast<KeyValueExpr *>(expr)) {
        Walk(key_value_expr, f);
    } else if (ArrayType *array_type = dynamic_cast<ArrayType *>(expr)) {
        Walk(array_type, f);
    } else if (FuncType *func_type = dynamic_cast<FuncType *>(expr)) {
        Walk(func_type, f);
    } else if (InterfaceType *interface_type = dynamic_cast<InterfaceType *>(expr)) {
        Walk(interface_type, f);
    } else if (StructType *struct_type = dynamic_cast<StructType *>(expr)) {
        Walk(struct_type, f);
    } else if (TypeInstance *type_instance = dynamic_cast<TypeInstance *>(expr)) {
        Walk(type_instance, f);
    } else if (BasicLit *basic_lit = dynamic_cast<BasicLit *>(expr)) {
        Walk(basic_lit, f);
    } else if (Ident *ident = dynamic_cast<Ident *>(expr)) {
        Walk(ident, f);
    } else {
        throw "unexpected AST expr";
    }
}

void Walk(Stmt *stmt, WalkFunction f) {
    if (BlockStmt *block_stmt = dynamic_cast<BlockStmt *>(stmt)) {
        Walk(block_stmt, f);
    } else if (DeclStmt *decl_stmt = dynamic_cast<DeclStmt *>(stmt)) {
        Walk(decl_stmt, f);
    } else if (AssignStmt *assign_stmt = dynamic_cast<AssignStmt *>(stmt)) {
        Walk(assign_stmt, f);
    } else if (ExprStmt *expr_stmt = dynamic_cast<ExprStmt *>(stmt)) {
        Walk(expr_stmt, f);
    } else if (IncDecStmt *inc_dec_stmt = dynamic_cast<IncDecStmt *>(stmt)) {
        Walk(inc_dec_stmt, f);
    } else if (ReturnStmt *return_stmt = dynamic_cast<ReturnStmt *>(stmt)) {
        Walk(return_stmt, f);
    } else if (IfStmt *if_stmt = dynamic_cast<IfStmt *>(stmt)) {
        Walk(if_stmt, f);
    } else if (SwitchStmt *switch_stmt = dynamic_cast<SwitchStmt *>(stmt)) {
        Walk(switch_stmt, f);
    } else if (CaseClause *case_clause = dynamic_cast<CaseClause *>(stmt)) {
        Walk(case_clause, f);
    } else if (ForStmt *for_stmt = dynamic_cast<ForStmt *>(stmt)) {
        Walk(for_stmt, f);
    } else if (LabeledStmt *labeled_stmt = dynamic_cast<LabeledStmt *>(stmt)) {
        Walk(labeled_stmt, f);
    } else if (BranchStmt *branch_stmt = dynamic_cast<BranchStmt *>(stmt)) {
        Walk(branch_stmt, f);
    } else {
        throw "unexpected AST stmt";
    }
}

void Walk(Decl *decl, WalkFunction f) {
    if (GenDecl *gen_decl = dynamic_cast<GenDecl *>(decl)) {
        Walk(gen_decl, f);
    } else if (FuncDecl *func_decl = dynamic_cast<FuncDecl *>(decl)) {
        Walk(func_decl, f);
    } else {
        throw "unexpected AST decl";
    }
}

void Walk(File *file, WalkFunction f) {
    WalkFunction g = f(file);
    if (!g) return;
    for (auto& decl : file->decls_) {
        Walk(decl.get(), g);
    }
    g(nullptr);
}

void Walk(GenDecl *gen_decl, WalkFunction f) {
    WalkFunction g = f(gen_decl);
    if (!g) return;
    for (auto& spec : gen_decl->specs_) {
        Walk(spec.get(), g);
    }
    g(nullptr);
}

void Walk(ImportSpec *import_spec, WalkFunction f) {
    WalkFunction g = f(import_spec);
    if (!g) return;
    if (import_spec->name_) {
        Walk(import_spec->name_.get(), g);
    }
    Walk(import_spec->path_.get(), g);
    g(nullptr);
}

void Walk(ValueSpec *value_spec, WalkFunction f) {
    WalkFunction g = f(value_spec);
    if (!g) return;
    for (auto& name : value_spec->names_) {
        Walk(name.get(), g);
    }
    if (value_spec->type_) {
        Walk(value_spec->type_.get(), g);
    }
    for (auto& value : value_spec->values_) {
        Walk(value.get(), g);
    }
    g(nullptr);
}

void Walk(TypeSpec *type_spec, WalkFunction f) {
    WalkFunction g = f(type_spec);
    if (!g) return;
    Walk(type_spec->name_.get(), g);
    if (type_spec->type_params_) {
        Walk(type_spec->type_params_.get(), g);
    }
    Walk(type_spec->type_.get(), g);
    g(nullptr);
}

void Walk(FuncDecl *func_decl, WalkFunction f) {
    WalkFunction g = f(func_decl);
    if (!g) return;
    if (func_decl->receiver_) {
        Walk(func_decl->receiver_.get(), g);
    }
    Walk(func_decl->name_.get(), g);
    if (func_decl->type_params_) {
        Walk(func_decl->type_params_.get(), g);
    }
    Walk(func_decl->type_.get(), g);
    Walk(func_decl->body_.get(), g);
    g(nullptr);
}

void Walk(BlockStmt *block_stmt, WalkFunction f) {
    WalkFunction g = f(block_stmt);
    if (!g) return;
    for (auto& stmt : block_stmt->stmts_) {
        Walk(stmt.get(), g);
    }
    g(nullptr);
}

void Walk(DeclStmt *decl_stmt, WalkFunction f) {
    WalkFunction g = f(decl_stmt);
    if (!g) return;
    Walk(decl_stmt->decl_.get(), g);
    g(nullptr);
}

void Walk(AssignStmt *assign_stmt, WalkFunction f) {
    WalkFunction g = f(assign_stmt);
    if (!g) return;
    for (auto& l : assign_stmt->lhs_) {
        Walk(l.get(), g);
    }
    for (auto& r : assign_stmt->rhs_) {
        Walk(r.get(), g);
    }
    g(nullptr);
}

void Walk(ExprStmt *expr_stmt, WalkFunction f) {
    WalkFunction g = f(expr_stmt);
    if (!g) return;
    Walk(expr_stmt->x_.get(), g);
    g(nullptr);
}

void Walk(IncDecStmt *inc_dec_stmt, WalkFunction f) {
    WalkFunction g = f(inc_dec_stmt);
    if (!g) return;
    Walk(inc_dec_stmt->x_.get(), g);
    g(nullptr);
}

void Walk(ReturnStmt *return_stmt, WalkFunction f) {
    WalkFunction g = f(return_stmt);
    if (!g) return;
    for (auto& result : return_stmt->results_) {
        Walk(result.get(), g);
    }
    g(nullptr);
}

void Walk(IfStmt *if_stmt, WalkFunction f) {
    WalkFunction g = f(if_stmt);
    if (!g) return;
    if (if_stmt->init_) {
        Walk(if_stmt->init_.get(), g);
    }
    Walk(if_stmt->cond_.get(), g);
    Walk(if_stmt->body_.get(), g);
    if (if_stmt->else_) {
        Walk(if_stmt->else_.get(), g);
    }
    g(nullptr);
}

void Walk(SwitchStmt *switch_stmt, WalkFunction f) {
    WalkFunction g = f(switch_stmt);
    if (!g) return;
    if (switch_stmt->init_) {
        Walk(switch_stmt->init_.get(), g);
    }
    if (switch_stmt->tag_) {
        Walk(switch_stmt->tag_.get(), g);
    }
    Walk(switch_stmt->body_.get(), g);
    g(nullptr);
}

void Walk(CaseClause *case_clause, WalkFunction f) {
    WalkFunction g = f(case_clause);
    if (!g) return;
    for (auto& cond_val : case_clause->cond_vals_) {
        Walk(cond_val.get(), g);
    }
    for (auto& stmt : case_clause->body_) {
        Walk(stmt.get(), g);
    }
    g(nullptr);
}

void Walk(ForStmt *for_stmt, WalkFunction f) {
    WalkFunction g = f(for_stmt);
    if (!g) return;
    if (for_stmt->init_) {
        Walk(for_stmt->init_.get(), g);
    }
    if (for_stmt->cond_) {
        Walk(for_stmt->cond_.get(), g);
    }
    if (for_stmt->post_) {
        Walk(for_stmt->post_.get(), g);
    }
    Walk(for_stmt->body_.get(), g);
    g(nullptr);
}

void Walk(LabeledStmt *labeled_stmt, WalkFunction f) {
    WalkFunction g = f(labeled_stmt);
    if (!g) return;
    Walk(labeled_stmt->label_.get(), g);
    Walk(labeled_stmt->stmt_.get(), g);
    g(nullptr);
}

void Walk(BranchStmt *branch_stmt, WalkFunction f) {
    WalkFunction g = f(branch_stmt);
    if (!g) return;
    if (branch_stmt->label_) {
        Walk(branch_stmt->label_.get(), g);
    }
    g(nullptr);
}

void Walk(UnaryExpr *unary_expr, WalkFunction f) {
    WalkFunction g = f(unary_expr);
    if (!g) return;
    Walk(unary_expr->x_.get(), g);
    g(nullptr);
}

void Walk(BinaryExpr *binary_expr, WalkFunction f) {
    WalkFunction g = f(binary_expr);
    if (!g) return;
    Walk(binary_expr->x_.get(), g);
    Walk(binary_expr->y_.get(), g);
    g(nullptr);
}

void Walk(ParenExpr *paren_expr, WalkFunction f) {
    WalkFunction g = f(paren_expr);
    if (!g) return;
    Walk(paren_expr->x_.get(), g);
    g(nullptr);
}

void Walk(SelectionExpr *selection_expr, WalkFunction f) {
    WalkFunction g = f(selection_expr);
    if (!g) return;
    Walk(selection_expr->accessed_.get(), g);
    Walk(selection_expr->selection_.get(), g);
    g(nullptr);
}

void Walk(TypeAssertExpr *type_assert_expr, WalkFunction f) {
    WalkFunction g = f(type_assert_expr);
    if (!g) return;
    Walk(type_assert_expr->x_.get(), g);
    Walk(type_assert_expr->type_.get(), g);
    g(nullptr);
}

void Walk(IndexExpr *index_expr, WalkFunction f) {
    WalkFunction g = f(index_expr);
    if (!g) return;
    Walk(index_expr->accessed_.get(), g);
    Walk(index_expr->index_.get(), g);
    g(nullptr);
}

void Walk(CallExpr *call_expr, WalkFunction f) {
    WalkFunction g = f(call_expr);
    if (!g) return;
    Walk(call_expr->func_.get(), g);
    if (call_expr->type_args_) {
        Walk(call_expr->type_args_.get(), g);
    }
    for (auto& arg : call_expr->args_) {
        Walk(arg.get(), g);
    }
    g(nullptr);
}

void Walk(FuncLit *func_lit, WalkFunction f) {
    WalkFunction g = f(func_lit);
    if (!g) return;
    Walk(func_lit->type_.get(), g);
    Walk(func_lit->body_.get(), g);
    g(nullptr);
}

void Walk(CompositeLit *composite_lit, WalkFunction f) {
    WalkFunction g = f(composite_lit);
    if (!g) return;
    Walk(composite_lit->type_.get(), g);
    for (auto& value : composite_lit->values_) {
        Walk(value.get(), g);
    }
    g(nullptr);
}

void Walk(KeyValueExpr *key_value_expr, WalkFunction f) {
    WalkFunction g = f(key_value_expr);
    if (!g) return;
    Walk(key_value_expr->key_.get(), g);
    Walk(key_value_expr->value_.get(), g);
    g(nullptr);
}

void Walk(ArrayType *array_type, WalkFunction f) {
    WalkFunction g = f(array_type);
    if (!g) return;
    if (array_type->len_) {
        Walk(array_type->len_.get(), g);
    }
    Walk(array_type->element_type_.get(), g);
    g(nullptr);
}

void Walk(FuncType *func_type, WalkFunction f) {
    WalkFunction g = f(func_type);
    if (!g) return;
    Walk(func_type->params_.get(), g);
    if (func_type->results_) {
        Walk(func_type->results_.get(), g);
    }
    g(nullptr);
}

void Walk(InterfaceType *interface_type, WalkFunction f) {
    WalkFunction g = f(interface_type);
    if (!g) return;
    for (auto& method : interface_type->methods_) {
        Walk(method.get(), g);
    }
    g(nullptr);
}

void Walk(MethodSpec *method_spec, WalkFunction f) {
    WalkFunction g = f(method_spec);
    if (!g) return;
    Walk(method_spec->name_.get(), g);
    Walk(method_spec->params_.get(), g);
    if (method_spec->results_) {
        Walk(method_spec->results_.get(), g);
    }
    g(nullptr);
}

void Walk(StructType *struct_type, WalkFunction f) {
    WalkFunction g = f(struct_type);
    if (!g) return;
    Walk(struct_type->fields_.get(), g);
    g(nullptr);
}

void Walk(TypeInstance *type_instance, WalkFunction f) {
    WalkFunction g = f(type_instance);
    if (!g) return;
    Walk(type_instance->type_.get(), g);
    Walk(type_instance->type_args_.get(), g);
    g(nullptr);
}

void Walk(FieldList *field_list, WalkFunction f) {
    WalkFunction g = f(field_list);
    if (!g) return;
    for (auto& field : field_list->fields_) {
        Walk(field.get(), g);
    }
    g(nullptr);
}

void Walk(Field *field, WalkFunction f) {
    WalkFunction g = f(field);
    if (!g) return;
    for (auto& name : field->names_) {
        Walk(name.get(), g);
    }
    Walk(field->type_.get(), g);
    g(nullptr);
}

void Walk(TypeArgList *type_arg_list, WalkFunction f) {
    WalkFunction g = f(type_arg_list);
    if (!g) return;
    for (auto& arg : type_arg_list->args_) {
        Walk(arg.get(), g);
    }
    g(nullptr);
}

void Walk(TypeParamList *type_param_list, WalkFunction f) {
    WalkFunction g = f(type_param_list);
    if (!g) return;
    for (auto& param : type_param_list->params_) {
        Walk(param.get(), g);
    }
    g(nullptr);
}

void Walk(TypeParam *type_param, WalkFunction f) {
    WalkFunction g = f(type_param);
    if (!g) return;
    Walk(type_param->name_.get(), g);
    if (type_param->type_) {
        Walk(type_param->type_.get(), g);
    }
    g(nullptr);
}

void Walk(BasicLit *basic_lit, WalkFunction f) {
    WalkFunction g = f(basic_lit);
    if (g) g(nullptr);
}

void Walk(Ident *ident, WalkFunction f) {
    WalkFunction g = f(ident);
    if (g) g(nullptr);
}

bool IsTypeSwitchStmt(SwitchStmt *switch_stmt) {
    if (switch_stmt->init_ && switch_stmt->tag_) {
        return false;
    } else if (!switch_stmt->init_ && !switch_stmt->tag_) {
        return false;
    }
    Expr *expr = nullptr;
    if (switch_stmt->init_) {
        auto assign_stmt = dynamic_cast<AssignStmt *>(switch_stmt->init_.get());
        if (assign_stmt == nullptr ||
            assign_stmt->tok_ != tokens::kDefine ||
            assign_stmt->lhs_.size() != 1 ||
            assign_stmt->rhs_.size() != 1) {
            return false;
        }
        expr = assign_stmt->rhs_.at(0).get();
    } else if (switch_stmt->tag_) {
        expr = switch_stmt->tag_.get();
    }
    if (expr == nullptr) {
        return false;
    }
    TypeAssertExpr *type_assert_expr = dynamic_cast<TypeAssertExpr *>(expr);
    if (type_assert_expr == nullptr ||
        type_assert_expr->type_) {
        return false;
    }
    return true;
}

}
}
