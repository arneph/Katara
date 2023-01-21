//
//  memory_test.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 1/22/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/common/memory/memory.h"

#include "gtest/gtest.h"

namespace common::memory {

TEST(MemoryTest, EmptyConstructorSucceeds) {
  Memory memory;
  EXPECT_EQ(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 0);
  EXPECT_EQ(memory.permissions(), Permissions::kNone);
}

TEST(MemoryTest, CreateAndDeleteZeroSizeSucceeds) {
  Memory memory(/*size=*/0, Permissions::kRead);
  EXPECT_EQ(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 0);
  EXPECT_EQ(memory.permissions(), Permissions::kRead);
}

TEST(MemoryTest, CreateAndDeleteOnePageNoPermissionsSucceeds) {
  Memory memory(kPageSize, Permissions::kNone);
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), kPageSize);
  EXPECT_EQ(memory.permissions(), Permissions::kNone);
}

TEST(MemoryTest, CreateAndDeleteOnePageReadWritePermissionsSucceeds) {
  Memory memory(kPageSize, Permissions(Permissions::kRead | Permissions::kWrite));
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), kPageSize);
  EXPECT_EQ(memory.permissions(), Permissions::kRead | Permissions::kWrite);
  for (int i = 0; i < kPageSize; i++) {
    EXPECT_EQ(memory.data()[i], 0);
  }
  memory.data()[42] = 'A';
  memory.data()[123] = '0';
  memory.data()[1999] = 42;
  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[42], 'A');
  EXPECT_EQ(memory.data()[123], '0');
  EXPECT_EQ(memory.data()[1999], 42);
  EXPECT_EQ(memory.data()[kPageSize - 1], 0);
}

TEST(MemoryTest, CreateAndDeleteOnePageExecutePermissionsSucceeds) {
  Memory memory(kPageSize, Permissions::kExecute);
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), kPageSize);
  EXPECT_EQ(memory.permissions(), Permissions::kExecute);
}

TEST(MemoryTest, MoveConstructorForEmptySucceeds) {
  Memory a;
  Memory b(std::move(a));
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Permissions::kNone);
  EXPECT_EQ(b.data().base(), nullptr);
  EXPECT_EQ(b.data().size(), 0);
  EXPECT_EQ(b.permissions(), Permissions::kNone);
}

TEST(MemoryTest, MoveConstructorForZeroSizeSucceeds) {
  Memory a(/*size=*/0, Permissions::kExecute);
  Memory b(std::move(a));
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Permissions::kNone);
  EXPECT_EQ(b.data().base(), nullptr);
  EXPECT_EQ(b.data().size(), 0);
  EXPECT_EQ(b.permissions(), Permissions::kExecute);
}

TEST(MemoryTest, MoveConstructorForOnePageSucceeds) {
  Memory a(kPageSize, Permissions::kWrite);
  uint8_t* base = a.data().base();
  Memory b(std::move(a));
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Permissions::kNone);
  EXPECT_EQ(b.data().base(), base);
  EXPECT_EQ(b.data().size(), kPageSize);
  EXPECT_EQ(b.permissions(), Permissions::kWrite);
}

TEST(MemoryTest, MoveAssignmentOperatorForEmptySucceeds) {
  Memory a;
  Memory b;
  b = std::move(a);
  EXPECT_EQ(b.data().base(), nullptr);
  EXPECT_EQ(b.data().size(), 0);
  EXPECT_EQ(b.permissions(), Permissions::kNone);
}

TEST(MemoryTest, MoveAssignmentOperatorFromOnePageToEmptySucceeds) {
  Memory a(kPageSize, Permissions::kRead);
  uint8_t* base = a.data().base();
  Memory b;
  b = std::move(a);
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Permissions::kNone);
  EXPECT_EQ(b.data().base(), base);
  EXPECT_EQ(b.data().size(), kPageSize);
  EXPECT_EQ(b.permissions(), Permissions::kRead);
}

TEST(MemoryTest, MoveAssignmentOperatorFromEmptyToOnePageSucceeds) {
  Memory a;
  Memory b(kPageSize, Permissions::kExecute);
  b = std::move(a);
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Permissions::kNone);
  EXPECT_EQ(b.data().base(), nullptr);
  EXPECT_EQ(b.data().size(), 0);
  EXPECT_EQ(b.permissions(), Permissions::kNone);
}

TEST(MemoryTest, MoveAssignmentOperatorForSeveralPageSucceeds) {
  Memory a(kPageSize * 3, Permissions::kWrite);
  uint8_t* base = a.data().base();
  Memory b(kPageSize * 7, Permissions::kExecute);
  b = std::move(a);
  EXPECT_EQ(a.data().base(), nullptr);
  EXPECT_EQ(a.data().size(), 0);
  EXPECT_EQ(a.permissions(), Permissions::kNone);
  EXPECT_EQ(b.data().base(), base);
  EXPECT_EQ(b.data().size(), kPageSize * 3);
  EXPECT_EQ(b.permissions(), Permissions::kWrite);
}

TEST(MemoryTest, ChangePermissionsSucceeds) {
  Memory memory(kPageSize * 23, Permissions::kNone);

  memory.ChangePermissions(Permissions::kWrite);
  memory.data()[321] = 123;
  memory.data()[kPageSize * 11 + 654] = 255;
  memory.data()[kPageSize * 17 + 47] = 1;
  memory.ChangePermissions(Permissions::kExecute);
  memory.ChangePermissions(Permissions::kNone);
  memory.ChangePermissions(Permissions(Permissions::kRead | Permissions::kWrite));

  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[321], 123);
  EXPECT_EQ(memory.data()[kPageSize * 11 + 654], 255);
  EXPECT_EQ(memory.data()[kPageSize * 17 + 47], 1);
  EXPECT_EQ(memory.data()[kPageSize * 23 - 1], 0);

  memory.data()[321] = 'X';
  memory.ChangePermissions(Permissions::kWrite);
  memory.data()[kPageSize * 11 + 654] = 'Y';
  memory.ChangePermissions(Permissions::kNone);
  memory.ChangePermissions(Permissions::kExecute);
  memory.ChangePermissions(Permissions::kWrite);
  memory.ChangePermissions(Permissions::kRead);
  memory.ChangePermissions(Permissions::kNone);
  memory.ChangePermissions(Permissions::kRead);

  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[321], 'X');
  EXPECT_EQ(memory.data()[kPageSize * 11 + 654], 'Y');
  EXPECT_EQ(memory.data()[kPageSize * 17 + 47], 1);
  EXPECT_EQ(memory.data()[kPageSize * 23 - 1], 0);

  memory.ChangePermissions(Permissions::kNone);
  memory.ChangePermissions(Permissions::kExecute);
  memory.ChangePermissions(Permissions::kWrite);
}

TEST(MemoryTest, FreeForEmptySucceeds) {
  Memory memory(/*size=*/0, Permissions::kExecute);
  memory.Free();
  EXPECT_EQ(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 0);
  EXPECT_EQ(memory.permissions(), Permissions::kNone);
}

TEST(MemoryTest, FreeForSeveralPagesSucceeds) {
  Memory memory(/*size=*/kPageSize * 9, Permissions::kExecute);
  memory.Free();
  EXPECT_EQ(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 0);
  EXPECT_EQ(memory.permissions(), Permissions::kNone);
}

TEST(MemoryTest, LessThanPageSizeSucceeds) {
  Memory memory(/*size=*/17, Permissions::kRead);
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), 17);
  EXPECT_EQ(memory.permissions(), Permissions::kRead);
  for (int i = 0; i < 17; i++) {
    EXPECT_EQ(memory.data()[i], 0);
  }

  memory.ChangePermissions(Permissions::kWrite);
  memory.data()[8] = 55;
  memory.ChangePermissions(Permissions::kExecute);
  memory.ChangePermissions(Permissions::kRead);

  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[8], 55);
  EXPECT_EQ(memory.data()[16], 0);
}

TEST(MemoryTest, NotMultipleOfPageSizeSucceeds) {
  Memory memory(/*size=*/kPageSize * 3 + 17, Permissions::kRead);
  EXPECT_NE(memory.data().base(), nullptr);
  EXPECT_EQ(memory.data().size(), kPageSize * 3 + 17);
  EXPECT_EQ(memory.permissions(), Permissions::kRead);
  for (int i = 0; i < kPageSize * 3 + 17; i++) {
    EXPECT_EQ(memory.data()[i], 0);
  }

  memory.ChangePermissions(Permissions::kWrite);
  memory.data()[8] = 55;
  memory.data()[kPageSize * 3 + 11] = 66;
  memory.ChangePermissions(Permissions::kExecute);
  memory.ChangePermissions(Permissions::kRead);

  EXPECT_EQ(memory.data()[0], 0);
  EXPECT_EQ(memory.data()[8], 55);
  EXPECT_EQ(memory.data()[kPageSize * 3 + 11], 66);
  EXPECT_EQ(memory.data()[kPageSize * 3 + 16], 0);
}

}  // namespace common::memory
