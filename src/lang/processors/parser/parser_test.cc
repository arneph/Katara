//
//  parser_test.cc
//  Katara
//
//  Created by Arne Philipeit on 6/25/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/lang/processors/parser/parser.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/ast/nodes.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/tokens/tokens.h"

using ::testing::IsEmpty;
using ::testing::SizeIs;

class ExprParsingTest : public ::testing::Test {
 protected:
  lang::ast::Expr* ParseExprUnderTest(std::string expr_under_test) {
    lang::pos::File* pos_file =
        pos_file_set_.AddFile("test.kat", "package main\nvar t = " + expr_under_test + "\n");
    lang::ast::ASTBuilder ast_builder = ast_.builder();
    lang::issues::IssueTracker issues(&pos_file_set_);
    lang::ast::File* ast_file = lang::parser::Parser::ParseFile(pos_file, ast_builder, issues);

    auto var_decl = static_cast<lang::ast::GenDecl*>(ast_file->decls().at(0));
    auto value_spec = static_cast<lang::ast::ValueSpec*>(var_decl->specs().at(0));
    return value_spec->values().at(0);
  }

 private:
  lang::pos::FileSet pos_file_set_;
  lang::ast::AST ast_;
};

TEST_F(ExprParsingTest, ParsesExpr1Correctly) {
  lang::ast::Expr* expr_under_test = ParseExprUnderTest("a == 0 || b == 1");
  ASSERT_EQ(expr_under_test->node_kind(), lang::ast::NodeKind::kBinaryExpr);
  auto binary_expr = static_cast<lang::ast::BinaryExpr*>(expr_under_test);
  EXPECT_EQ(binary_expr->op(), lang::tokens::kLOr);

  {
    ASSERT_EQ(binary_expr->x()->node_kind(), lang::ast::NodeKind::kCompareExpr);
    auto x_expr = static_cast<lang::ast::CompareExpr*>(binary_expr->x());
    ASSERT_THAT(x_expr->operands(), SizeIs(2));
    ASSERT_THAT(x_expr->compare_ops(), SizeIs(1));
    EXPECT_EQ(x_expr->compare_ops().at(0), lang::tokens::kEql);
    ASSERT_EQ(x_expr->operands().at(0)->node_kind(), lang::ast::NodeKind::kIdent);
    auto x_expr_operand0 = static_cast<lang::ast::Ident*>(x_expr->operands().at(0));
    EXPECT_EQ(x_expr_operand0->name(), "a");
    ASSERT_EQ(x_expr->operands().at(1)->node_kind(), lang::ast::NodeKind::kBasicLit);
    auto x_expr_operand1 = static_cast<lang::ast::BasicLit*>(x_expr->operands().at(1));
    EXPECT_EQ(x_expr_operand1->value(), "0");
    EXPECT_EQ(x_expr_operand1->kind(), lang::tokens::kInt);
  }
  {
    ASSERT_EQ(binary_expr->y()->node_kind(), lang::ast::NodeKind::kCompareExpr);
    auto y_expr = static_cast<lang::ast::CompareExpr*>(binary_expr->y());
    ASSERT_THAT(y_expr->operands(), SizeIs(2));
    ASSERT_THAT(y_expr->compare_ops(), SizeIs(1));
    EXPECT_EQ(y_expr->compare_ops().at(0), lang::tokens::kEql);
    ASSERT_EQ(y_expr->operands().at(0)->node_kind(), lang::ast::NodeKind::kIdent);
    auto y_expr_operand0 = static_cast<lang::ast::Ident*>(y_expr->operands().at(0));
    EXPECT_EQ(y_expr_operand0->name(), "b");
    ASSERT_EQ(y_expr->operands().at(1)->node_kind(), lang::ast::NodeKind::kBasicLit);
    auto y_expr_operand1 = static_cast<lang::ast::BasicLit*>(y_expr->operands().at(1));
    EXPECT_EQ(y_expr_operand1->value(), "1");
    EXPECT_EQ(y_expr_operand1->kind(), lang::tokens::kInt);
  }
}

TEST_F(ExprParsingTest, ParsesExpr2Correctly) {
  lang::ast::Expr* expr_under_test = ParseExprUnderTest("0 <= a < 10 || b == 42");
  ASSERT_EQ(expr_under_test->node_kind(), lang::ast::NodeKind::kBinaryExpr);
  auto binary_expr = static_cast<lang::ast::BinaryExpr*>(expr_under_test);
  EXPECT_EQ(binary_expr->op(), lang::tokens::kLOr);
  ASSERT_EQ(binary_expr->x()->node_kind(), lang::ast::NodeKind::kCompareExpr);

  {
    ASSERT_EQ(binary_expr->x()->node_kind(), lang::ast::NodeKind::kCompareExpr);
    auto x_expr = static_cast<lang::ast::CompareExpr*>(binary_expr->x());
    ASSERT_THAT(x_expr->operands(), SizeIs(3));
    ASSERT_THAT(x_expr->compare_ops(), SizeIs(2));
    EXPECT_EQ(x_expr->compare_ops().at(0), lang::tokens::kLeq);
    EXPECT_EQ(x_expr->compare_ops().at(1), lang::tokens::kLss);
    ASSERT_EQ(x_expr->operands().at(0)->node_kind(), lang::ast::NodeKind::kBasicLit);
    auto x_expr_operand0 = static_cast<lang::ast::BasicLit*>(x_expr->operands().at(0));
    EXPECT_EQ(x_expr_operand0->value(), "0");
    EXPECT_EQ(x_expr_operand0->kind(), lang::tokens::kInt);
    ASSERT_EQ(x_expr->operands().at(1)->node_kind(), lang::ast::NodeKind::kIdent);
    auto x_expr_operand1 = static_cast<lang::ast::Ident*>(x_expr->operands().at(1));
    EXPECT_EQ(x_expr_operand1->name(), "a");
    ASSERT_EQ(x_expr->operands().at(2)->node_kind(), lang::ast::NodeKind::kBasicLit);
    auto x_expr_operand2 = static_cast<lang::ast::BasicLit*>(x_expr->operands().at(2));
    EXPECT_EQ(x_expr_operand2->value(), "10");
    EXPECT_EQ(x_expr_operand2->kind(), lang::tokens::kInt);
  }
  {
    ASSERT_EQ(binary_expr->y()->node_kind(), lang::ast::NodeKind::kCompareExpr);
    auto y_expr = static_cast<lang::ast::CompareExpr*>(binary_expr->y());
    ASSERT_THAT(y_expr->operands(), SizeIs(2));
    ASSERT_THAT(y_expr->compare_ops(), SizeIs(1));
    EXPECT_EQ(y_expr->compare_ops().at(0), lang::tokens::kEql);
    ASSERT_EQ(y_expr->operands().at(0)->node_kind(), lang::ast::NodeKind::kIdent);
    auto y_expr_operand0 = static_cast<lang::ast::Ident*>(y_expr->operands().at(0));
    EXPECT_EQ(y_expr_operand0->name(), "b");
    ASSERT_EQ(y_expr->operands().at(1)->node_kind(), lang::ast::NodeKind::kBasicLit);
    auto y_expr_operand1 = static_cast<lang::ast::BasicLit*>(y_expr->operands().at(1));
    EXPECT_EQ(y_expr_operand1->value(), "42");
    EXPECT_EQ(y_expr_operand1->kind(), lang::tokens::kInt);
  }
}

TEST_F(ExprParsingTest, ParsesExpr3Correctly) {
  lang::ast::Expr* expr_under_test = ParseExprUnderTest("b == 42 && 0 <= a < 10");
  ASSERT_EQ(expr_under_test->node_kind(), lang::ast::NodeKind::kBinaryExpr);
  auto binary_expr = static_cast<lang::ast::BinaryExpr*>(expr_under_test);
  EXPECT_EQ(binary_expr->op(), lang::tokens::kLAnd);
  {
    ASSERT_EQ(binary_expr->x()->node_kind(), lang::ast::NodeKind::kCompareExpr);
    auto x_expr = static_cast<lang::ast::CompareExpr*>(binary_expr->x());
    ASSERT_THAT(x_expr->operands(), SizeIs(2));
    ASSERT_THAT(x_expr->compare_ops(), SizeIs(1));
    EXPECT_EQ(x_expr->compare_ops().at(0), lang::tokens::kEql);
    ASSERT_EQ(x_expr->operands().at(0)->node_kind(), lang::ast::NodeKind::kIdent);
    auto x_expr_operand0 = static_cast<lang::ast::Ident*>(x_expr->operands().at(0));
    EXPECT_EQ(x_expr_operand0->name(), "b");
    ASSERT_EQ(x_expr->operands().at(1)->node_kind(), lang::ast::NodeKind::kBasicLit);
    auto x_expr_operand1 = static_cast<lang::ast::BasicLit*>(x_expr->operands().at(1));
    EXPECT_EQ(x_expr_operand1->value(), "42");
    EXPECT_EQ(x_expr_operand1->kind(), lang::tokens::kInt);
  }
  {
    ASSERT_EQ(binary_expr->y()->node_kind(), lang::ast::NodeKind::kCompareExpr);
    auto y_expr = static_cast<lang::ast::CompareExpr*>(binary_expr->y());
    ASSERT_THAT(y_expr->operands(), SizeIs(3));
    ASSERT_THAT(y_expr->compare_ops(), SizeIs(2));
    EXPECT_EQ(y_expr->compare_ops().at(0), lang::tokens::kLeq);
    EXPECT_EQ(y_expr->compare_ops().at(1), lang::tokens::kLss);
    ASSERT_EQ(y_expr->operands().at(0)->node_kind(), lang::ast::NodeKind::kBasicLit);
    auto y_expr_operand0 = static_cast<lang::ast::BasicLit*>(y_expr->operands().at(0));
    EXPECT_EQ(y_expr_operand0->value(), "0");
    EXPECT_EQ(y_expr_operand0->kind(), lang::tokens::kInt);
    ASSERT_EQ(y_expr->operands().at(1)->node_kind(), lang::ast::NodeKind::kIdent);
    auto y_expr_operand1 = static_cast<lang::ast::Ident*>(y_expr->operands().at(1));
    EXPECT_EQ(y_expr_operand1->name(), "a");
    ASSERT_EQ(y_expr->operands().at(2)->node_kind(), lang::ast::NodeKind::kBasicLit);
    auto y_expr_operand2 = static_cast<lang::ast::BasicLit*>(y_expr->operands().at(2));
    EXPECT_EQ(y_expr_operand2->value(), "10");
    EXPECT_EQ(y_expr_operand2->kind(), lang::tokens::kInt);
  }
}

TEST_F(ExprParsingTest, ParsesExpr4Correctly) {
  lang::ast::Expr* expr_under_test = ParseExprUnderTest("a % b == c");
  ASSERT_EQ(expr_under_test->node_kind(), lang::ast::NodeKind::kCompareExpr);
  auto compare_expr = static_cast<lang::ast::CompareExpr*>(expr_under_test);
  ASSERT_THAT(compare_expr->operands(), SizeIs(2));
  ASSERT_THAT(compare_expr->compare_ops(), SizeIs(1));
  EXPECT_EQ(compare_expr->compare_ops().at(0), lang::tokens::kEql);
  {
    ASSERT_EQ(compare_expr->operands().at(0)->node_kind(), lang::ast::NodeKind::kBinaryExpr);
    auto x_expr = static_cast<lang::ast::BinaryExpr*>(compare_expr->operands().at(0));
    EXPECT_EQ(x_expr->op(), lang::tokens::kRem);
    ASSERT_EQ(x_expr->x()->node_kind(), lang::ast::NodeKind::kIdent);
    auto x_expr_operand0 = static_cast<lang::ast::Ident*>(x_expr->x());
    EXPECT_EQ(x_expr_operand0->name(), "a");
    ASSERT_EQ(x_expr->y()->node_kind(), lang::ast::NodeKind::kIdent);
    auto x_expr_operand1 = static_cast<lang::ast::Ident*>(x_expr->y());
    EXPECT_EQ(x_expr_operand1->name(), "b");
  }
  {
    ASSERT_EQ(compare_expr->operands().at(1)->node_kind(), lang::ast::NodeKind::kIdent);
    auto y_expr = static_cast<lang::ast::Ident*>(compare_expr->operands().at(1));
    EXPECT_EQ(y_expr->name(), "c");
  }
}
