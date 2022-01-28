//
//  phi_resolver_test.cpp
//  Katara-tests
//
//  Created by Arne Philipeit on 7/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "src/ir/processors/phi_resolver.h"

#include "gtest/gtest.h"
#include "src/common/atomics/atomics.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/values.h"

namespace {

TEST(PhiResolverTest, ResolvesPhisAfterSimpleBranch) {
  // Define func and blocks:
  ir::Func func(/*fnum=*/0);
  ir::Block* entry_block = func.AddBlock();
  ir::Block* branch_a_block = func.AddBlock();
  ir::Block* branch_b_block = func.AddBlock();
  ir::Block* merge_block = func.AddBlock();
  func.set_entry_block_num(entry_block->number());

  // Define values involved in phi instrs:
  auto value_a = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/0);
  auto value_b = ir::ToIntConstant(common::Int(int64_t{123}));
  auto value_c = std::make_shared<ir::Computed>(ir::i64(), /*vnum=*/1);

  auto value_i = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/2);
  auto value_j = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/3);
  auto value_k = std::make_shared<ir::Computed>(ir::bool_type(), /*vnum=*/4);

  auto value_x = ir::ToIntConstant(common::Int(uint8_t{24}));
  auto value_y = ir::ToIntConstant(common::Int(uint8_t{42}));
  auto value_z = std::make_shared<ir::Computed>(ir::u8(), /*vnum=*/5);

  // Add instrs to entry block:
  auto instr_a = std::make_unique<ir::IntUnaryInstr>(value_a, common::Int::UnaryOp::kNeg,
                                                     ir::ToIntConstant(common::Int(int64_t{321})));
  auto instr_a_ptr = instr_a.get();
  auto instr_b = std::make_unique<ir::IntCompareInstr>(
      value_i, common::Int::CompareOp::kLss, value_a, ir::ToIntConstant(common::Int(int64_t{222})));
  auto instr_b_ptr = instr_b.get();
  auto instr_c = std::make_unique<ir::JumpCondInstr>(value_i, branch_a_block->number(),
                                                     branch_b_block->number());
  auto instr_c_ptr = instr_c.get();

  entry_block->instrs().push_back(std::move(instr_a));
  entry_block->instrs().push_back(std::move(instr_b));
  entry_block->instrs().push_back(std::move(instr_c));

  // Add instrs to branch A block:
  auto instr_d = std::make_unique<ir::JumpInstr>(merge_block->number());
  auto instr_d_ptr = instr_d.get();

  branch_a_block->instrs().push_back(std::move(instr_d));

  // Add instrs to branch B block:
  auto instr_e = std::make_unique<ir::BoolNotInstr>(value_j, value_i);
  auto instr_e_ptr = instr_e.get();
  auto instr_f = std::make_unique<ir::JumpInstr>(merge_block->number());
  auto instr_f_ptr = instr_f.get();

  branch_b_block->instrs().push_back(std::move(instr_e));
  branch_b_block->instrs().push_back(std::move(instr_f));

  // Add instrs to merge block:
  auto instr_g = std::make_unique<ir::ReturnInstr>(
      std::vector<std::shared_ptr<ir::Value>>{value_c, value_k, value_z});
  auto instr_g_ptr = instr_g.get();

  merge_block->instrs().push_back(std::make_unique<ir::PhiInstr>(
      value_c, std::vector<std::shared_ptr<ir::InheritedValue>>{
                   std::make_shared<ir::InheritedValue>(value_a, entry_block->number()),
                   std::make_shared<ir::InheritedValue>(value_b, branch_b_block->number())}));
  merge_block->instrs().push_back(std::make_unique<ir::PhiInstr>(
      value_k, std::vector<std::shared_ptr<ir::InheritedValue>>{
                   std::make_shared<ir::InheritedValue>(value_i, entry_block->number()),
                   std::make_shared<ir::InheritedValue>(value_j, branch_b_block->number())}));
  merge_block->instrs().push_back(std::make_unique<ir::PhiInstr>(
      value_z, std::vector<std::shared_ptr<ir::InheritedValue>>{
                   std::make_shared<ir::InheritedValue>(value_x, branch_a_block->number()),
                   std::make_shared<ir::InheritedValue>(value_y, branch_b_block->number())}));
  merge_block->instrs().push_back(std::move(instr_g));

  // Resolve Phis:
  ir_processors::ResolvePhisInFunc(&func);

  // Check entry block:
  EXPECT_EQ(5, entry_block->instrs().size());
  EXPECT_EQ(instr_a_ptr, entry_block->instrs().at(0).get());
  EXPECT_EQ(instr_b_ptr, entry_block->instrs().at(1).get());
  EXPECT_EQ(ir::InstrKind::kMov, entry_block->instrs().at(2)->instr_kind());
  ir::MovInstr* generated_mov_a = static_cast<ir::MovInstr*>(entry_block->instrs().at(2).get());
  EXPECT_EQ(value_c, generated_mov_a->result());
  EXPECT_EQ(value_a, generated_mov_a->origin());
  EXPECT_EQ(ir::InstrKind::kMov, entry_block->instrs().at(3)->instr_kind());
  ir::MovInstr* generated_mov_b = static_cast<ir::MovInstr*>(entry_block->instrs().at(3).get());
  EXPECT_EQ(value_k, generated_mov_b->result());
  EXPECT_EQ(value_i, generated_mov_b->origin());
  EXPECT_EQ(instr_c_ptr, entry_block->instrs().at(4).get());

  // Check branch A block:
  EXPECT_EQ(2, branch_a_block->instrs().size());
  EXPECT_EQ(ir::InstrKind::kMov, branch_a_block->instrs().at(0)->instr_kind());
  ir::MovInstr* generated_mov_c = static_cast<ir::MovInstr*>(branch_a_block->instrs().at(0).get());
  EXPECT_EQ(value_z, generated_mov_c->result());
  EXPECT_EQ(value_x, generated_mov_c->origin());
  EXPECT_EQ(instr_d_ptr, branch_a_block->instrs().at(1).get());

  // Check branch B block:
  EXPECT_EQ(5, branch_b_block->instrs().size());
  EXPECT_EQ(instr_e_ptr, branch_b_block->instrs().at(0).get());
  EXPECT_EQ(ir::InstrKind::kMov, branch_b_block->instrs().at(1)->instr_kind());
  ir::MovInstr* generated_mov_d = static_cast<ir::MovInstr*>(branch_b_block->instrs().at(1).get());
  EXPECT_EQ(value_c, generated_mov_d->result());
  EXPECT_EQ(value_b, generated_mov_d->origin());
  EXPECT_EQ(ir::InstrKind::kMov, branch_b_block->instrs().at(2)->instr_kind());
  ir::MovInstr* generated_mov_e = static_cast<ir::MovInstr*>(branch_b_block->instrs().at(2).get());
  EXPECT_EQ(value_k, generated_mov_e->result());
  EXPECT_EQ(value_j, generated_mov_e->origin());
  EXPECT_EQ(ir::InstrKind::kMov, branch_b_block->instrs().at(3)->instr_kind());
  ir::MovInstr* generated_mov_f = static_cast<ir::MovInstr*>(branch_b_block->instrs().at(3).get());
  EXPECT_EQ(value_z, generated_mov_f->result());
  EXPECT_EQ(value_y, generated_mov_f->origin());
  EXPECT_EQ(instr_f_ptr, branch_b_block->instrs().at(4).get());

  // Check merge block:
  EXPECT_EQ(1, merge_block->instrs().size());
  EXPECT_EQ(instr_g_ptr, merge_block->instrs().at(0).get());
}

}  // namespace
