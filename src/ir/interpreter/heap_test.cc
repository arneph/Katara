//
//  heap_test.cc
//  Katara
//
//  Created by Arne Philipeit on 7/31/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/interpreter/heap.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(HeapDeathTest, CatchesMallocWithZeroSize) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        heap.Malloc(0);
      }(),
      "attempted malloc with non-positive size");
}

TEST(HeapDeathTest, CatchesMallocWithNegativeSize) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        heap.Malloc(-10);
      }(),
      "attempted malloc with non-positive size");
}

TEST(HeapDeathTest, CatchesFreeOfNeverAllocatedMemory) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        heap.Free(0x12345);
      }(),
      "memory was never allocated");
}

TEST(HeapDeathTest, CatchesFreeOfWrongAddress) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        int64_t addr = heap.Malloc(123);
        addr += 12;
        heap.Free(addr);
      }(),
      "address to be freed does not point to start of allocated block");
}

TEST(HeapDeathTest, CatchesDoubleFree) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        int64_t addr = heap.Malloc(42);
        heap.Free(addr);
        heap.Free(addr);
      }(),
      "memory was already freed");
}

TEST(HeapDeathTest, CatchesMemoryNeverFreed) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        heap.Malloc(42);
      }(),
      "not all memory was freed");
}

TEST(HeapDeathTest, CatchesLoadFromNeverAllocatedMemory) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        heap.Load<int8_t>(0x12345);
      }(),
      "attempted to access memory range that doesn't exist");
}

TEST(HeapDeathTest, CatchesLoadFromFreedMemory) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        int64_t addr = heap.Malloc(1);
        heap.Free(addr);
        heap.Load<int8_t>(addr);
      }(),
      "attempted to access memory range that was freed");
}

TEST(HeapDeathTest, CatchesLoadFromPartiallyAllocatedMemory) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        int64_t addr = heap.Malloc(1);
        heap.Load<int16_t>(addr);
      }(),
      "attempted to access memory range that only partially overlaps allocated memory");
}

TEST(HeapDeathTest, CatchesLoadFromCompletelyUninitializedMemory) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        int64_t addr = heap.Malloc(1);
        heap.Load<int8_t>(addr);
      }(),
      "attempted to read uninitialized memory");
}

TEST(HeapDeathTest, CatchesLoadFromPartiallyUninitializedMemory) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        int64_t addr = heap.Malloc(8);
        heap.Store<int8_t>(addr, int8_t{42});
        heap.Store<int8_t>(addr + 6, int16_t{24});
        heap.Load<int64_t>(addr);
      }(),
      "attempted to read uninitialized memory");
}

TEST(HeapDeathTest, CatchesStoreToNeverAllocatedMemory) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        heap.Store<int8_t>(0x12345, int8_t{42});
      }(),
      "attempted to access memory range that doesn't exist");
}

TEST(HeapDeathTest, CatchesStoreToFreedMemory) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        int64_t addr = heap.Malloc(1);
        heap.Free(addr);
        heap.Store<int8_t>(addr, int8_t{42});
      }(),
      "attempted to access memory range that was freed");
}

TEST(HeapDeathTest, CatchesStoreToPartiallyAllocatedMemory) {
  EXPECT_DEATH(
      [] {
        auto heap = ir_interpreter::Heap(/*sanitize=*/true);
        int64_t addr = heap.Malloc(1);
        heap.Store<int16_t>(addr, int16_t{420});
      }(),
      "attempted to access memory range that only partially overlaps allocated memory");
}

TEST(HeapTest, SupportsNormalOperation) {
  auto heap = ir_interpreter::Heap(/*sanitize=*/true);
  int64_t addr_a = heap.Malloc(100);
  heap.Store<int16_t>(addr_a, int16_t{420});
  heap.Store<int32_t>(addr_a + 17, int32_t{1234567});

  EXPECT_EQ(heap.Load<int16_t>(addr_a), 420);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a), 164);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 1), 1);
  EXPECT_EQ(heap.Load<int32_t>(addr_a + 17), 1234567);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 17), 135);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 18), 214);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 19), 18);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 20), 0);

  heap.Store<int8_t>(addr_a + 99, int8_t{42});

  int64_t addr_b = heap.Malloc(5);
  int64_t addr_c = heap.Malloc(17);

  EXPECT_EQ(heap.Load<int8_t>(addr_a + 99), 42);

  heap.Store<int32_t>(addr_b, 7654321);
  heap.Store<uint8_t>(addr_b + 4, 234);

  EXPECT_EQ(heap.Load<uint8_t>(addr_b + 4), 234);
  EXPECT_EQ(heap.Load<int32_t>(addr_b), 7654321);

  heap.Free(addr_b);

  heap.Store<int64_t>(addr_c + 5, int64_t{987654321});

  EXPECT_EQ(heap.Load<int16_t>(addr_a), 420);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a), 164);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 1), 1);
  EXPECT_EQ(heap.Load<int32_t>(addr_a + 17), 1234567);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 17), 135);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 18), 214);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 19), 18);
  EXPECT_EQ(heap.Load<uint8_t>(addr_a + 20), 0);

  EXPECT_EQ(heap.Load<int64_t>(addr_c + 5), 987654321);

  heap.Free(addr_a);
  heap.Free(addr_c);
}
