//
//  ast_util.cc
//  Katara
//
//  Created by Arne Philipeit on 7/18/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "ast_util.h"

#include <memory>
#include <vector>

#include "vcg/edge.h"
#include "vcg/node.h"

namespace lang {
namespace ast {

vcg::Graph NodeToTree(pos::FileSet* file_set, Node* node) {
  vcg::Graph graph;
  std::vector<int64_t> stack;
  int64_t count = 0;

  WalkFunction walker = WalkFunction([&](Node* ast_node) -> WalkFunction {
    if (ast_node == nullptr) {
      stack.pop_back();
      return WalkFunction();
    }
    int64_t number = count++;
    std::string title;
    vcg::Color color;
    if (ast_node->is_expr()) {
      title = "expr";
      color = vcg::kTurquoise;
    } else if (ast_node->is_stmt()) {
      title = "stmt";
      color = vcg::kGreen;
    } else if (ast_node->is_decl()) {
      title = "decl";
      color = vcg::kYellow;
    } else {
      title = "node";
      color = vcg::kRed;
    }
    pos::File* file = file_set->FileAt(ast_node->start());
    std::string text = file->contents(ast_node->start(), ast_node->end());
    if (auto it = std::find(text.begin(), text.end(), '\n'); it != text.end()) {
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
WalkFunction::WalkFunction(std::function<WalkFunction(Node*)> f) : f_(f) {}

WalkFunction WalkFunction::operator()(Node* node) { return f_(node); }

WalkFunction::operator bool() const { return (bool)f_; }

void Walk(Node* node, WalkFunction f) {
  switch (node->node_kind()) {
    case NodeKind::kFile:
      Walk(static_cast<File*>(node), f);
      break;
    case NodeKind::kGenDecl:
      Walk(static_cast<GenDecl*>(node), f);
      break;
    case NodeKind::kFuncDecl:
      Walk(static_cast<FuncDecl*>(node), f);
      break;
    case NodeKind::kImportSpec:
      Walk(static_cast<ImportSpec*>(node), f);
      break;
    case NodeKind::kValueSpec:
      Walk(static_cast<ValueSpec*>(node), f);
      break;
    case NodeKind::kTypeSpec:
      Walk(static_cast<TypeSpec*>(node), f);
      break;
    case NodeKind::kBlockStmt:
      Walk(static_cast<BlockStmt*>(node), f);
      break;
    case NodeKind::kDeclStmt:
      Walk(static_cast<DeclStmt*>(node), f);
      break;
    case NodeKind::kAssignStmt:
      Walk(static_cast<AssignStmt*>(node), f);
      break;
    case NodeKind::kExprStmt:
      Walk(static_cast<ExprStmt*>(node), f);
      break;
    case NodeKind::kIncDecStmt:
      Walk(static_cast<IncDecStmt*>(node), f);
      break;
    case NodeKind::kReturnStmt:
      Walk(static_cast<ReturnStmt*>(node), f);
      break;
    case NodeKind::kIfStmt:
      Walk(static_cast<IfStmt*>(node), f);
      break;
    case NodeKind::kExprSwitchStmt:
      Walk(static_cast<ExprSwitchStmt*>(node), f);
      break;
    case NodeKind::kTypeSwitchStmt:
      Walk(static_cast<TypeSwitchStmt*>(node), f);
      break;
    case NodeKind::kCaseClause:
      Walk(static_cast<CaseClause*>(node), f);
      break;
    case NodeKind::kForStmt:
      Walk(static_cast<ForStmt*>(node), f);
      break;
    case NodeKind::kLabeledStmt:
      Walk(static_cast<LabeledStmt*>(node), f);
      break;
    case NodeKind::kBranchStmt:
      Walk(static_cast<BranchStmt*>(node), f);
      break;
    case NodeKind::kUnaryExpr:
      Walk(static_cast<UnaryExpr*>(node), f);
      break;
    case NodeKind::kBinaryExpr:
      Walk(static_cast<BinaryExpr*>(node), f);
      break;
    case NodeKind::kCompareExpr:
      Walk(static_cast<CompareExpr*>(node), f);
      break;
    case NodeKind::kParenExpr:
      Walk(static_cast<ParenExpr*>(node), f);
      break;
    case NodeKind::kSelectionExpr:
      Walk(static_cast<SelectionExpr*>(node), f);
      break;
    case NodeKind::kTypeAssertExpr:
      Walk(static_cast<TypeAssertExpr*>(node), f);
      break;
    case NodeKind::kIndexExpr:
      Walk(static_cast<IndexExpr*>(node), f);
      break;
    case NodeKind::kCallExpr:
      Walk(static_cast<CallExpr*>(node), f);
      break;
    case NodeKind::kFuncLit:
      Walk(static_cast<FuncLit*>(node), f);
      break;
    case NodeKind::kCompositeLit:
      Walk(static_cast<CompositeLit*>(node), f);
      break;
    case NodeKind::kKeyValueExpr:
      Walk(static_cast<KeyValueExpr*>(node), f);
      break;
    case NodeKind::kArrayType:
      Walk(static_cast<ArrayType*>(node), f);
      break;
    case NodeKind::kFuncType:
      Walk(static_cast<FuncType*>(node), f);
      break;
    case NodeKind::kInterfaceType:
      Walk(static_cast<InterfaceType*>(node), f);
      break;
    case NodeKind::kStructType:
      Walk(static_cast<StructType*>(node), f);
      break;
    case NodeKind::kTypeInstance:
      Walk(static_cast<TypeInstance*>(node), f);
      break;
    case NodeKind::kBasicLit:
      Walk(static_cast<BasicLit*>(node), f);
      break;
    case NodeKind::kIdent:
      Walk(static_cast<Ident*>(node), f);
      break;
    case NodeKind::kMethodSpec:
      Walk(static_cast<MethodSpec*>(node), f);
      break;
    case NodeKind::kExprReceiver:
      Walk(static_cast<ExprReceiver*>(node), f);
      break;
    case NodeKind::kTypeReceiver:
      Walk(static_cast<TypeReceiver*>(node), f);
      break;
    case NodeKind::kFieldList:
      Walk(static_cast<FieldList*>(node), f);
      break;
    case NodeKind::kField:
      Walk(static_cast<Field*>(node), f);
      break;
    case NodeKind::kTypeParamList:
      Walk(static_cast<TypeParamList*>(node), f);
      break;
    case NodeKind::kTypeParam:
      Walk(static_cast<TypeParam*>(node), f);
      break;
  }
}

void Walk(File* file, WalkFunction f) {
  WalkFunction g = f(file);
  if (!g) return;
  for (Decl* decl : file->decls()) {
    Walk(decl, g);
  }
  g(nullptr);
}

void Walk(GenDecl* gen_decl, WalkFunction f) {
  WalkFunction g = f(gen_decl);
  if (!g) return;
  for (Spec* spec : gen_decl->specs()) {
    Walk(spec, g);
  }
  g(nullptr);
}

void Walk(ImportSpec* import_spec, WalkFunction f) {
  WalkFunction g = f(import_spec);
  if (!g) return;
  if (import_spec->name() != nullptr) {
    Walk(import_spec->name(), g);
  }
  Walk(import_spec->path(), g);
  g(nullptr);
}

void Walk(ValueSpec* value_spec, WalkFunction f) {
  WalkFunction g = f(value_spec);
  if (!g) return;
  for (Ident* name : value_spec->names()) {
    Walk(name, g);
  }
  if (value_spec->type() != nullptr) {
    Walk(value_spec->type(), g);
  }
  for (Expr* value : value_spec->values()) {
    Walk(value, g);
  }
  g(nullptr);
}

void Walk(TypeSpec* type_spec, WalkFunction f) {
  WalkFunction g = f(type_spec);
  if (!g) return;
  Walk(type_spec->name(), g);
  if (type_spec->type_params() != nullptr) {
    Walk(type_spec->type_params(), g);
  }
  Walk(type_spec->type(), g);
  g(nullptr);
}

void Walk(FuncDecl* func_decl, WalkFunction f) {
  WalkFunction g = f(func_decl);
  if (!g) return;
  switch (func_decl->kind()) {
    case FuncDecl::Kind::kFunc:
      break;
    case FuncDecl::Kind::kInstanceMethod:
      Walk(func_decl->expr_receiver(), g);
      break;
    case FuncDecl::Kind::kTypeMethod:
      Walk(func_decl->type_receiver(), g);
      break;
    default:
      throw "internal error: unexpected func decl kind";
  }
  Walk(func_decl->name(), g);
  if (func_decl->type_params() != nullptr) {
    Walk(func_decl->type_params(), g);
  }
  Walk(func_decl->func_type(), g);
  Walk(func_decl->body(), g);
  g(nullptr);
}

void Walk(BlockStmt* block_stmt, WalkFunction f) {
  WalkFunction g = f(block_stmt);
  if (!g) return;
  for (Stmt* stmt : block_stmt->stmts()) {
    Walk(stmt, g);
  }
  g(nullptr);
}

void Walk(DeclStmt* decl_stmt, WalkFunction f) {
  WalkFunction g = f(decl_stmt);
  if (!g) return;
  Walk(decl_stmt->decl(), g);
  g(nullptr);
}

void Walk(AssignStmt* assign_stmt, WalkFunction f) {
  WalkFunction g = f(assign_stmt);
  if (!g) return;
  for (Expr* l : assign_stmt->lhs()) {
    Walk(l, g);
  }
  for (Expr* r : assign_stmt->rhs()) {
    Walk(r, g);
  }
  g(nullptr);
}

void Walk(ExprStmt* expr_stmt, WalkFunction f) {
  WalkFunction g = f(expr_stmt);
  if (!g) return;
  Walk(expr_stmt->x(), g);
  g(nullptr);
}

void Walk(IncDecStmt* inc_dec_stmt, WalkFunction f) {
  WalkFunction g = f(inc_dec_stmt);
  if (!g) return;
  Walk(inc_dec_stmt->x(), g);
  g(nullptr);
}

void Walk(ReturnStmt* return_stmt, WalkFunction f) {
  WalkFunction g = f(return_stmt);
  if (!g) return;
  for (Expr* result : return_stmt->results()) {
    Walk(result, g);
  }
  g(nullptr);
}

void Walk(IfStmt* if_stmt, WalkFunction f) {
  WalkFunction g = f(if_stmt);
  if (!g) return;
  if (if_stmt->init_stmt() != nullptr) {
    Walk(if_stmt->init_stmt(), g);
  }
  Walk(if_stmt->cond_expr(), g);
  Walk(if_stmt->body(), g);
  if (if_stmt->else_stmt() != nullptr) {
    Walk(if_stmt->else_stmt(), g);
  }
  g(nullptr);
}

void Walk(ExprSwitchStmt* switch_stmt, WalkFunction f) {
  WalkFunction g = f(switch_stmt);
  if (!g) return;
  if (switch_stmt->init_stmt()) {
    Walk(switch_stmt->init_stmt(), g);
  }
  if (switch_stmt->tag_expr()) {
    Walk(switch_stmt->tag_expr(), g);
  }
  Walk(switch_stmt->body(), g);
  g(nullptr);
}

void Walk(TypeSwitchStmt* switch_stmt, WalkFunction f) {
  WalkFunction g = f(switch_stmt);
  if (!g) return;
  if (switch_stmt->var() != nullptr) {
    Walk(switch_stmt->var(), g);
  }
  Walk(switch_stmt->tag_expr(), g);
  Walk(switch_stmt->body(), g);
  g(nullptr);
}

void Walk(CaseClause* case_clause, WalkFunction f) {
  WalkFunction g = f(case_clause);
  if (!g) return;
  for (Expr* cond_val : case_clause->cond_vals()) {
    Walk(cond_val, g);
  }
  for (Stmt* stmt : case_clause->body()) {
    Walk(stmt, g);
  }
  g(nullptr);
}

void Walk(ForStmt* for_stmt, WalkFunction f) {
  WalkFunction g = f(for_stmt);
  if (!g) return;
  if (for_stmt->init_stmt() != nullptr) {
    Walk(for_stmt->init_stmt(), g);
  }
  if (for_stmt->cond_expr() != nullptr) {
    Walk(for_stmt->cond_expr(), g);
  }
  if (for_stmt->post_stmt() != nullptr) {
    Walk(for_stmt->post_stmt(), g);
  }
  Walk(for_stmt->body(), g);
  g(nullptr);
}

void Walk(LabeledStmt* labeled_stmt, WalkFunction f) {
  WalkFunction g = f(labeled_stmt);
  if (!g) return;
  Walk(labeled_stmt->label(), g);
  Walk(labeled_stmt->stmt(), g);
  g(nullptr);
}

void Walk(BranchStmt* branch_stmt, WalkFunction f) {
  WalkFunction g = f(branch_stmt);
  if (!g) return;
  if (branch_stmt->label() != nullptr) {
    Walk(branch_stmt->label(), g);
  }
  g(nullptr);
}

void Walk(UnaryExpr* unary_expr, WalkFunction f) {
  WalkFunction g = f(unary_expr);
  if (!g) return;
  Walk(unary_expr->x(), g);
  g(nullptr);
}

void Walk(BinaryExpr* binary_expr, WalkFunction f) {
  WalkFunction g = f(binary_expr);
  if (!g) return;
  Walk(binary_expr->x(), g);
  Walk(binary_expr->y(), g);
  g(nullptr);
}

void Walk(CompareExpr* compare_expr, WalkFunction f) {
  WalkFunction g = f(compare_expr);
  if (!g) return;
  for (Expr* operand : compare_expr->operands()) {
    Walk(operand, g);
  }
  g(nullptr);
}

void Walk(ParenExpr* paren_expr, WalkFunction f) {
  WalkFunction g = f(paren_expr);
  if (!g) return;
  Walk(paren_expr->x(), g);
  g(nullptr);
}

void Walk(SelectionExpr* selection_expr, WalkFunction f) {
  WalkFunction g = f(selection_expr);
  if (!g) return;
  Walk(selection_expr->accessed(), g);
  Walk(selection_expr->selection(), g);
  g(nullptr);
}

void Walk(TypeAssertExpr* type_assert_expr, WalkFunction f) {
  WalkFunction g = f(type_assert_expr);
  if (!g) return;
  Walk(type_assert_expr->x(), g);
  Walk(type_assert_expr->type(), g);
  g(nullptr);
}

void Walk(IndexExpr* index_expr, WalkFunction f) {
  WalkFunction g = f(index_expr);
  if (!g) return;
  Walk(index_expr->accessed(), g);
  Walk(index_expr->index(), g);
  g(nullptr);
}

void Walk(CallExpr* call_expr, WalkFunction f) {
  WalkFunction g = f(call_expr);
  if (!g) return;
  Walk(call_expr->func(), g);
  for (Expr* type_arg : call_expr->type_args()) {
    Walk(type_arg, g);
  }
  for (Expr* arg : call_expr->args()) {
    Walk(arg, g);
  }
  g(nullptr);
}

void Walk(FuncLit* func_lit, WalkFunction f) {
  WalkFunction g = f(func_lit);
  if (!g) return;
  Walk(func_lit->type(), g);
  Walk(func_lit->body(), g);
  g(nullptr);
}

void Walk(CompositeLit* composite_lit, WalkFunction f) {
  WalkFunction g = f(composite_lit);
  if (!g) return;
  Walk(composite_lit->type(), g);
  for (Expr* value : composite_lit->values()) {
    Walk(value, g);
  }
  g(nullptr);
}

void Walk(KeyValueExpr* key_value_expr, WalkFunction f) {
  WalkFunction g = f(key_value_expr);
  if (!g) return;
  Walk(key_value_expr->key(), g);
  Walk(key_value_expr->value(), g);
  g(nullptr);
}

void Walk(ArrayType* array_type, WalkFunction f) {
  WalkFunction g = f(array_type);
  if (!g) return;
  if (array_type->len() != nullptr) {
    Walk(array_type->len(), g);
  }
  Walk(array_type->element_type(), g);
  g(nullptr);
}

void Walk(FuncType* func_type, WalkFunction f) {
  WalkFunction g = f(func_type);
  if (!g) return;
  Walk(func_type->params(), g);
  if (func_type->results() != nullptr) {
    Walk(func_type->results(), g);
  }
  g(nullptr);
}

void Walk(InterfaceType* interface_type, WalkFunction f) {
  WalkFunction g = f(interface_type);
  if (!g) return;
  for (Expr* embedded_interface : interface_type->embedded_interfaces()) {
    Walk(embedded_interface, g);
  }
  for (MethodSpec* method : interface_type->methods()) {
    Walk(method, g);
  }
  g(nullptr);
}

void Walk(MethodSpec* method_spec, WalkFunction f) {
  WalkFunction g = f(method_spec);
  if (!g) return;
  Walk(method_spec->name(), g);
  Walk(method_spec->params(), g);
  if (method_spec->results()) {
    Walk(method_spec->results(), g);
  }
  g(nullptr);
}

void Walk(StructType* struct_type, WalkFunction f) {
  WalkFunction g = f(struct_type);
  if (!g) return;
  Walk(struct_type->fields(), g);
  g(nullptr);
}

void Walk(TypeInstance* type_instance, WalkFunction f) {
  WalkFunction g = f(type_instance);
  if (!g) return;
  Walk(type_instance->type(), g);
  for (Expr* type_arg : type_instance->type_args()) {
    Walk(type_arg, g);
  }
  g(nullptr);
}

void Walk(ExprReceiver* receiver, WalkFunction f) {
  WalkFunction g = f(receiver);
  if (!g) return;
  if (receiver->name() != nullptr) {
    Walk(receiver->name(), g);
  }
  Walk(receiver->type_name(), g);
  for (Ident* type_parameter_name : receiver->type_parameter_names()) {
    Walk(type_parameter_name, g);
  }
  g(nullptr);
}

void Walk(TypeReceiver* receiver, WalkFunction f) {
  WalkFunction g = f(receiver);
  if (!g) return;
  Walk(receiver->type_name(), g);
  for (Ident* type_parameter_name : receiver->type_parameter_names()) {
    Walk(type_parameter_name, g);
  }
  g(nullptr);
}

void Walk(FieldList* field_list, WalkFunction f) {
  WalkFunction g = f(field_list);
  if (!g) return;
  for (Field* field : field_list->fields()) {
    Walk(field, g);
  }
  g(nullptr);
}

void Walk(Field* field, WalkFunction f) {
  WalkFunction g = f(field);
  if (!g) return;
  for (ast::Ident* name : field->names()) {
    Walk(name, g);
  }
  Walk(field->type(), g);
  g(nullptr);
}

void Walk(TypeParamList* type_param_list, WalkFunction f) {
  WalkFunction g = f(type_param_list);
  if (!g) return;
  for (TypeParam* param : type_param_list->params()) {
    Walk(param, g);
  }
  g(nullptr);
}

void Walk(TypeParam* type_param, WalkFunction f) {
  WalkFunction g = f(type_param);
  if (!g) return;
  Walk(type_param->name(), g);
  if (type_param->type() != nullptr) {
    Walk(type_param->type(), g);
  }
  g(nullptr);
}

void Walk(BasicLit* basic_lit, WalkFunction f) {
  WalkFunction g = f(basic_lit);
  if (g) g(nullptr);
}

void Walk(Ident* ident, WalkFunction f) {
  WalkFunction g = f(ident);
  if (g) g(nullptr);
}

ast::Expr* Unparen(ast::Expr* expr) {
  while (expr->node_kind() == NodeKind::kParenExpr) {
    expr = static_cast<ParenExpr*>(expr)->x();
  }
  return expr;
}

}  // namespace ast
}  // namespace lang
