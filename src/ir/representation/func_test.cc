//
//  func_test.cc
//  Katara
//
//  Created by Arne Philipeit on 3/20/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/representation/func.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/num_types.h"

namespace {

using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

TEST(FuncTest, CreatesDominatorTreeForSingleBlock) {
  ir::Func func(/*fnum=*/0);
  ir::Block* block = func.AddBlock();
  func.set_entry_block_num(block->number());

  EXPECT_EQ(func.DominatorOf(block->number()), ir::kNoBlockNum);
  EXPECT_THAT(func.DomineesOf(block->number()), IsEmpty());
  EXPECT_THAT(func.GetBlocksInDominanceOrder(), ElementsAre(block->number()));
}

TEST(FuncTest, CreatesDominatorTreeForTwoBlocks) {
  ir::Func func(/*fnum=*/0);
  ir::Block* block_a = func.AddBlock();
  ir::Block* block_b = func.AddBlock();
  func.set_entry_block_num(block_a->number());
  func.AddControlFlow(block_a->number(), block_b->number());

  EXPECT_EQ(func.DominatorOf(block_a->number()), ir::kNoBlockNum);
  EXPECT_THAT(func.DomineesOf(block_a->number()), ElementsAre(block_b->number()));
  EXPECT_EQ(func.DominatorOf(block_b->number()), block_a->number());
  EXPECT_THAT(func.DomineesOf(block_b->number()), IsEmpty());
  EXPECT_THAT(func.GetBlocksInDominanceOrder(), ElementsAre(block_a->number(), block_b->number()));
}

TEST(FuncTest, CreatesDominatorTreeForThreeBlockFork) {
  ir::Func func(/*fnum=*/0);
  ir::Block* block_a = func.AddBlock();
  ir::Block* block_b = func.AddBlock();
  ir::Block* block_c = func.AddBlock();
  func.set_entry_block_num(block_a->number());
  func.AddControlFlow(block_a->number(), block_b->number());
  func.AddControlFlow(block_a->number(), block_c->number());
  func.AddControlFlow(block_b->number(), block_c->number());

  EXPECT_EQ(func.DominatorOf(block_a->number()), ir::kNoBlockNum);
  EXPECT_THAT(func.DomineesOf(block_a->number()),
              UnorderedElementsAre(block_b->number(), block_c->number()));
  EXPECT_EQ(func.DominatorOf(block_b->number()), block_a->number());
  EXPECT_THAT(func.DomineesOf(block_b->number()), IsEmpty());
  EXPECT_EQ(func.DominatorOf(block_c->number()), block_a->number());
  EXPECT_THAT(func.DomineesOf(block_c->number()), IsEmpty());

  std::vector<ir::block_num_t> dom_order = func.GetBlocksInDominanceOrder();
  ASSERT_THAT(dom_order, SizeIs(3));
  EXPECT_THAT(dom_order,
              UnorderedElementsAre(block_a->number(), block_b->number(), block_c->number()));
  EXPECT_EQ(dom_order.front(), block_a->number());
}

TEST(FuncTest, CreatesDominatorTreeForFourBlockFork) {
  ir::Func func(/*fnum=*/0);
  ir::Block* block_a = func.AddBlock();
  ir::Block* block_b = func.AddBlock();
  ir::Block* block_c = func.AddBlock();
  ir::Block* block_d = func.AddBlock();
  func.set_entry_block_num(block_a->number());
  func.AddControlFlow(block_a->number(), block_b->number());
  func.AddControlFlow(block_a->number(), block_c->number());
  func.AddControlFlow(block_b->number(), block_d->number());
  func.AddControlFlow(block_c->number(), block_d->number());

  EXPECT_EQ(func.DominatorOf(block_a->number()), ir::kNoBlockNum);
  EXPECT_THAT(func.DomineesOf(block_a->number()),
              UnorderedElementsAre(block_b->number(), block_c->number(), block_d->number()));
  EXPECT_EQ(func.DominatorOf(block_b->number()), block_a->number());
  EXPECT_THAT(func.DomineesOf(block_b->number()), IsEmpty());
  EXPECT_EQ(func.DominatorOf(block_c->number()), block_a->number());
  EXPECT_THAT(func.DomineesOf(block_c->number()), IsEmpty());
  EXPECT_EQ(func.DominatorOf(block_d->number()), block_a->number());
  EXPECT_THAT(func.DomineesOf(block_d->number()), IsEmpty());

  std::vector<ir::block_num_t> dom_order = func.GetBlocksInDominanceOrder();
  ASSERT_THAT(dom_order, SizeIs(4));
  EXPECT_THAT(dom_order, UnorderedElementsAre(block_a->number(), block_b->number(),
                                              block_c->number(), block_d->number()));
  EXPECT_EQ(dom_order.front(), block_a->number());
}

TEST(FuncTest, CreatesDominatorTreeForLoop) {
  ir::Func func(/*fnum=*/0);
  ir::Block* block_a = func.AddBlock();
  ir::Block* block_b = func.AddBlock();
  ir::Block* block_c = func.AddBlock();
  ir::Block* block_d = func.AddBlock();
  func.set_entry_block_num(block_a->number());
  func.AddControlFlow(block_a->number(), block_b->number());
  func.AddControlFlow(block_b->number(), block_c->number());
  func.AddControlFlow(block_b->number(), block_d->number());
  func.AddControlFlow(block_c->number(), block_b->number());

  EXPECT_EQ(func.DominatorOf(block_a->number()), ir::kNoBlockNum);
  EXPECT_THAT(func.DomineesOf(block_a->number()), ElementsAre(block_b->number()));
  EXPECT_EQ(func.DominatorOf(block_b->number()), block_a->number());
  EXPECT_THAT(func.DomineesOf(block_b->number()),
              UnorderedElementsAre(block_c->number(), block_d->number()));
  EXPECT_EQ(func.DominatorOf(block_c->number()), block_b->number());
  EXPECT_THAT(func.DomineesOf(block_c->number()), IsEmpty());
  EXPECT_EQ(func.DominatorOf(block_d->number()), block_b->number());
  EXPECT_THAT(func.DomineesOf(block_d->number()), IsEmpty());

  std::vector<ir::block_num_t> dom_order = func.GetBlocksInDominanceOrder();
  ASSERT_THAT(dom_order, SizeIs(4));
  EXPECT_THAT(dom_order, UnorderedElementsAre(block_a->number(), block_b->number(),
                                              block_c->number(), block_d->number()));
  EXPECT_EQ(dom_order.at(0), block_a->number());
  EXPECT_EQ(dom_order.at(1), block_b->number());
}

TEST(FuncTest, CreatesDominatorTreeForLoopWithForkContinueAndBreak) {
  ir::Func func(/*fnum=*/0);
  ir::Block* block_a = func.AddBlock();  // func entry block
  ir::Block* block_b = func.AddBlock();  // loop header
  ir::Block* block_c = func.AddBlock();  // loop body begin
  ir::Block* block_d = func.AddBlock();  // loop conditional block
  ir::Block* block_e = func.AddBlock();  // loop block with continue
  ir::Block* block_f = func.AddBlock();  // loop block with break
  ir::Block* block_g = func.AddBlock();  // loop block with return
  ir::Block* block_h = func.AddBlock();  // loop body end
  ir::Block* block_i = func.AddBlock();  // func exit block
  func.set_entry_block_num(block_a->number());
  func.AddControlFlow(block_a->number(), block_b->number());
  func.AddControlFlow(block_b->number(), block_c->number());
  func.AddControlFlow(block_b->number(), block_i->number());
  func.AddControlFlow(block_c->number(), block_d->number());
  func.AddControlFlow(block_c->number(), block_e->number());
  func.AddControlFlow(block_c->number(), block_f->number());
  func.AddControlFlow(block_c->number(), block_g->number());
  func.AddControlFlow(block_c->number(), block_h->number());
  func.AddControlFlow(block_d->number(), block_h->number());
  func.AddControlFlow(block_e->number(), block_b->number());
  func.AddControlFlow(block_f->number(), block_i->number());
  func.AddControlFlow(block_h->number(), block_b->number());

  EXPECT_EQ(func.DominatorOf(block_a->number()), ir::kNoBlockNum);
  EXPECT_THAT(func.DomineesOf(block_a->number()), ElementsAre(block_b->number()));
  EXPECT_EQ(func.DominatorOf(block_b->number()), block_a->number());
  EXPECT_THAT(func.DomineesOf(block_b->number()),
              UnorderedElementsAre(block_c->number(), block_i->number()));
  EXPECT_EQ(func.DominatorOf(block_c->number()), block_b->number());
  EXPECT_THAT(func.DomineesOf(block_c->number()),
              UnorderedElementsAre(block_d->number(), block_e->number(), block_f->number(),
                                   block_g->number(), block_h->number()));
  EXPECT_EQ(func.DominatorOf(block_d->number()), block_c->number());
  EXPECT_THAT(func.DomineesOf(block_d->number()), IsEmpty());
  EXPECT_EQ(func.DominatorOf(block_e->number()), block_c->number());
  EXPECT_THAT(func.DomineesOf(block_e->number()), IsEmpty());
  EXPECT_EQ(func.DominatorOf(block_f->number()), block_c->number());
  EXPECT_THAT(func.DomineesOf(block_f->number()), IsEmpty());
  EXPECT_EQ(func.DominatorOf(block_g->number()), block_c->number());
  EXPECT_THAT(func.DomineesOf(block_g->number()), IsEmpty());
  EXPECT_EQ(func.DominatorOf(block_h->number()), block_c->number());
  EXPECT_THAT(func.DomineesOf(block_h->number()), IsEmpty());
  EXPECT_EQ(func.DominatorOf(block_i->number()), block_b->number());
  EXPECT_THAT(func.DomineesOf(block_i->number()), IsEmpty());

  std::vector<ir::block_num_t> dom_order = func.GetBlocksInDominanceOrder();
  ASSERT_THAT(dom_order, SizeIs(9));
  EXPECT_THAT(dom_order,
              UnorderedElementsAre(block_a->number(), block_b->number(), block_c->number(),
                                   block_d->number(), block_e->number(), block_f->number(),
                                   block_g->number(), block_h->number(), block_i->number()));
  EXPECT_EQ(dom_order.at(0), block_a->number());
  EXPECT_EQ(dom_order.at(1), block_b->number());
  EXPECT_THAT(dom_order.at(2), AnyOf(block_c->number(), block_i->number()));
}

}  // namespace
