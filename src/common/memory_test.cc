//
//  memory_test.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 1/22/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/common/memory.h"

#include "gtest/gtest.h"

namespace common {

TEST(MemoryTest, EmptyConstructorSucceeds) {
  Memory memory;
  EXPECT_EQ(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 0);
  EXPECT_EQ(memory.permissions(), Memory::kNone);
}

TEST(MemoryTest, CreateAndDeleteZeroSizeSucceeds) {
  Memory memory(/*size=*/0, Memory::kRead);
  EXPECT_EQ(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 0);
  EXPECT_EQ(memory.permissions(), Memory::kRead);
}

TEST(MemoryTest, CreateAndDeleteOnePageNoPermissionsSucceeds) {
  Memory memory(Memory::kPageSize, Memory::kNone);
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), Memory::kPageSize);
  EXPECT_EQ(memory.permissions(), Memory::kNone);
}

TEST(MemoryTest, CreateAndDeleteOnePageReadWritePermissionsSucceeds) {
  Memory memory(Memory::kPageSize, Memory::Permissions(Memory::kRead | Memory::kWrite));
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), Memory::kPageSize);
  EXPECT_EQ(memory.permissions(), Memory::kRead | Memory::kWrite);
  for (int i = 0; i < Memory::kPageSize; i++) {
    EXPECT_EQ(memory.data()[i], 0);
  }
  memory.data()[42] = 'A';
  memory.data()[123] = '0';
  memory.data()[1999] = 42;
  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[42], 'A');
  EXPECT_EQ(memory.data()[123], '0');
  EXPECT_EQ(memory.data()[1999], 42);
  EXPECT_EQ(memory.data()[Memory::kPageSize - 1], 0);
}

TEST(MemoryTest, CreateAndDeleteOnePageExecutePermissionsSucceeds) {
  Memory memory(Memory::kPageSize, Memory::kExecute);
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), Memory::kPageSize);
  EXPECT_EQ(memory.permissions(), Memory::kExecute);
}

TEST(MemoryTest, MoveConstructorForEmptySucceeds) {
  Memory a;
  Memory b(std::move(a));
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Memory::kNone);
  EXPECT_EQ(b.data().base(), nullptr);
  EXPECT_EQ(b.data().size(), 0);
  EXPECT_EQ(b.permissions(), Memory::kNone);
}

TEST(MemoryTest, MoveConstructorForZeroSizeSucceeds) {
  Memory a(/*size=*/0, Memory::kExecute);
  Memory b(std::move(a));
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Memory::kNone);
  EXPECT_EQ(b.data().base(), nullptr);
  EXPECT_EQ(b.data().size(), 0);
  EXPECT_EQ(b.permissions(), Memory::kExecute);
}

TEST(MemoryTest, MoveConstructorForOnePageSucceeds) {
  Memory a(Memory::kPageSize, Memory::kWrite);
  uint8_t* base = a.data().base();
  Memory b(std::move(a));
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Memory::kNone);
  EXPECT_EQ(b.data().base(), base);
  EXPECT_EQ(b.data().size(), Memory::kPageSize);
  EXPECT_EQ(b.permissions(), Memory::kWrite);
}

TEST(MemoryTest, MoveAssignmentOperatorForEmptySucceeds) {
  Memory a;
  Memory b;
  b = std::move(a);
  EXPECT_EQ(b.data().base(), nullptr);
  EXPECT_EQ(b.data().size(), 0);
  EXPECT_EQ(b.permissions(), Memory::kNone);
}

TEST(MemoryTest, MoveAssignmentOperatorFromOnePageToEmptySucceeds) {
  Memory a(Memory::kPageSize, Memory::kRead);
  uint8_t* base = a.data().base();
  Memory b;
  b = std::move(a);
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Memory::kNone);
  EXPECT_EQ(b.data().base(), base);
  EXPECT_EQ(b.data().size(), Memory::kPageSize);
  EXPECT_EQ(b.permissions(), Memory::kRead);
}

TEST(MemoryTest, MoveAssignmentOperatorFromEmptyToOnePageSucceeds) {
  Memory a;
  Memory b(Memory::kPageSize, Memory::kExecute);
  b = std::move(a);
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Memory::kNone);
  EXPECT_EQ(b.data().base(), nullptr);
  EXPECT_EQ(b.data().size(), 0);
  EXPECT_EQ(b.permissions(), Memory::kNone);
}

TEST(MemoryTest, MoveAssignmentOperatorForSeveralPageSucceeds) {
  Memory a(Memory::kPageSize * 3, Memory::kWrite);
  uint8_t* base = a.data().base();
  Memory b(Memory::kPageSize * 7, Memory::kExecute);
  b = std::move(a);
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Memory::kNone);
  EXPECT_EQ(b.data().base(), base);
  EXPECT_EQ(b.data().size(), Memory::kPageSize * 3);
  EXPECT_EQ(b.permissions(), Memory::kWrite);
}

TEST(MemoryTest, ChangePermissionsSucceeds) {
  Memory memory(Memory::kPageSize * 23, Memory::kNone);

  memory.ChangePermissions(Memory::kWrite);
  memory.data()[321] = 123;
  memory.data()[Memory::kPageSize * 11 + 654] = 255;
  memory.data()[Memory::kPageSize * 17 + 47] = 1;
  memory.ChangePermissions(Memory::kExecute);
  memory.ChangePermissions(Memory::kNone);
  memory.ChangePermissions(Memory::Permissions(Memory::kRead | Memory::kWrite));

  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[321], 123);
  EXPECT_EQ(memory.data()[Memory::kPageSize * 11 + 654], 255);
  EXPECT_EQ(memory.data()[Memory::kPageSize * 17 + 47], 1);
  EXPECT_EQ(memory.data()[Memory::kPageSize * 23 - 1], 0);

  memory.data()[321] = 'X';
  memory.ChangePermissions(Memory::kWrite);
  memory.data()[Memory::kPageSize * 11 + 654] = 'Y';
  memory.ChangePermissions(Memory::kNone);
  memory.ChangePermissions(Memory::kExecute);
  memory.ChangePermissions(Memory::kWrite);
  memory.ChangePermissions(Memory::kRead);
  memory.ChangePermissions(Memory::kNone);
  memory.ChangePermissions(Memory::kRead);

  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[321], 'X');
  EXPECT_EQ(memory.data()[Memory::kPageSize * 11 + 654], 'Y');
  EXPECT_EQ(memory.data()[Memory::kPageSize * 17 + 47], 1);
  EXPECT_EQ(memory.data()[Memory::kPageSize * 23 - 1], 0);

  memory.ChangePermissions(Memory::kNone);
  memory.ChangePermissions(Memory::kExecute);
  memory.ChangePermissions(Memory::kWrite);
}

TEST(MemoryTest, FreeForEmptySucceeds) {
  Memory memory(/*size=*/0, Memory::kExecute);
  memory.Free();
  EXPECT_EQ(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 0);
  EXPECT_EQ(memory.permissions(), Memory::kNone);
}

TEST(MemoryTest, FreeForSeveralPagesSucceeds) {
  Memory memory(/*size=*/Memory::kPageSize * 9, Memory::kExecute);
  memory.Free();
  EXPECT_EQ(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 0);
  EXPECT_EQ(memory.permissions(), Memory::kNone);
}

TEST(MemoryTest, LessThanPageSizeSucceeds) {
  Memory memory(/*size=*/17, Memory::kRead);
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 17);
  EXPECT_EQ(memory.permissions(), Memory::kRead);
  for (int i = 0; i < 17; i++) {
    EXPECT_EQ(memory.data()[i], 0);
  }

  memory.ChangePermissions(Memory::kWrite);
  memory.data()[8] = 55;
  memory.ChangePermissions(Memory::kExecute);
  memory.ChangePermissions(Memory::kRead);

  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[8], 55);
  EXPECT_EQ(memory.data()[16], 0);
}

TEST(MemoryTest, NotMultipleOfPageSizeSucceeds) {
  Memory memory(/*size=*/Memory::kPageSize * 3 + 17, Memory::kRead);
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), Memory::kPageSize * 3 + 17);
  EXPECT_EQ(memory.permissions(), Memory::kRead);
  for (int i = 0; i < Memory::kPageSize * 3 + 17; i++) {
    EXPECT_EQ(memory.data()[i], 0);
  }

  memory.ChangePermissions(Memory::kWrite);
  memory.data()[8] = 55;
  memory.data()[Memory::kPageSize * 3 + 11] = 66;
  memory.ChangePermissions(Memory::kExecute);
  memory.ChangePermissions(Memory::kRead);

  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[8], 55);
  EXPECT_EQ(memory.data()[Memory::kPageSize * 3 + 11], 66);
  EXPECT_EQ(memory.data()[Memory::kPageSize * 3 + 16], 0);
}

}  // namespace common
