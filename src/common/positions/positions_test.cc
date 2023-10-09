//
//  positions_test.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "src/common/positions/positions.h"

#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace common::positions {

using ::testing::IsEmpty;

constexpr std::string_view kTestFileAContents = R"txt(Lorem ipsum
dolor sit amet, consectetur
adipiscing elit, sed do eiusmod tempor
incididunt ut labore et dolore magna aliqua.
Ut enim ad minim veniam, quis nostrud
exercitation ullamco laboris nisi ut aliquip ex
ea commodo consequat. Duis aute irure
dolor in reprehenderit in voluptate velit esse
cillum dolore eu fugiat nulla pariatur.
Excepteur sint occaecat cupidatat non proident,
sunt in culpa qui officia deserunt
mollit anim id est laborum.)txt";

constexpr std::string_view kTestFileBContents = R"txt(We
choose to go to the Moon in this decade and do the other things, not
because they are easy, but because they are hard; because that goal will
serve to organize and measure the best of our energies and skills, because
that challenge is one that we are willing to accept, one we are unwilling
to postpone, and one we intend to win, and the others, too.

)txt";

TEST(FileTest, ReturnsCorrectContents) {
  common::positions::FileSet file_set;
  common::positions::File* file_a = file_set.AddFile("test.txt", std::string(kTestFileAContents));

  EXPECT_EQ(file_a->contents(), kTestFileAContents);
  EXPECT_EQ(file_a->contents(range_t{file_a->start(), file_a->end()}), kTestFileAContents);
  EXPECT_EQ(file_a->contents(range_t{file_a->start(), file_a->start()}), "L");
  EXPECT_EQ(file_a->contents(range_t{file_a->start(), file_a->start() + 4}), "Lorem");
  EXPECT_EQ(file_a->contents(range_t{file_a->start() + 6, file_a->start() + 10}), "ipsum");
  EXPECT_EQ(file_a->contents(range_t{file_a->end() - 7, file_a->end()}), "laborum.");
  EXPECT_EQ(file_a->contents(range_t{file_a->end(), file_a->end()}), ".");
}

TEST(FileTest, ReturnsCorrectLineWithNumber) {
  common::positions::FileSet file_set;
  common::positions::File* file_a = file_set.AddFile("test.txt", std::string(kTestFileAContents));
  common::positions::File* file_b = file_set.AddFile("testB.txt", std::string(kTestFileBContents));

  EXPECT_THAT(file_a->LineWithNumber(common::positions::kNoLineNumber), IsEmpty());
  EXPECT_EQ(file_a->LineWithNumber(1), "Lorem ipsum");
  EXPECT_EQ(file_a->LineWithNumber(2), "dolor sit amet, consectetur");
  EXPECT_EQ(file_a->LineWithNumber(9), "cillum dolore eu fugiat nulla pariatur.");
  EXPECT_EQ(file_a->LineWithNumber(12), "mollit anim id est laborum.");
  EXPECT_THAT(file_a->LineWithNumber(13), IsEmpty());
  EXPECT_THAT(file_a->LineWithNumber(14), IsEmpty());

  EXPECT_THAT(file_b->LineWithNumber(common::positions::kNoLineNumber), IsEmpty());
  EXPECT_EQ(file_b->LineWithNumber(1), "We");
  EXPECT_EQ(file_b->LineWithNumber(6),
            "to postpone, and one we intend to win, and the others, too.");
  EXPECT_THAT(file_b->LineWithNumber(7), IsEmpty());
  EXPECT_THAT(file_b->LineWithNumber(8), IsEmpty());
  EXPECT_THAT(file_b->LineWithNumber(9), IsEmpty());
}

TEST(FileSetTest, ReturnsCorrectFiles) {
  common::positions::FileSet file_set;
  common::positions::File* file_a = file_set.AddFile("testA.txt", std::string(kTestFileAContents));
  common::positions::File* file_b = file_set.AddFile("testB.txt", std::string(kTestFileBContents));

  EXPECT_EQ(file_set.FileAt(common::positions::kNoPos), nullptr);
  EXPECT_EQ(file_set.FileAt(file_a->start() - 1), nullptr);
  EXPECT_EQ(file_set.FileAt(file_a->start()), file_a);
  EXPECT_EQ(file_set.FileAt(file_a->start() + 1), file_a);
  EXPECT_EQ(file_set.FileAt(file_a->start() + 42), file_a);
  EXPECT_EQ(file_set.FileAt(file_a->end() - 1), file_a);
  EXPECT_EQ(file_set.FileAt(file_a->end()), file_a);
  EXPECT_EQ(file_set.FileAt(file_a->end() + 1), file_a);
  EXPECT_EQ(file_set.FileAt(file_a->end() + 2), nullptr);
  EXPECT_EQ(file_set.FileAt(file_b->start() - 1), nullptr);
  EXPECT_EQ(file_set.FileAt(file_b->start()), file_b);
  EXPECT_EQ(file_set.FileAt(file_b->start() + 1), file_b);
  EXPECT_EQ(file_set.FileAt(file_b->start() + 123), file_b);
  EXPECT_EQ(file_set.FileAt(file_b->end() - 1), file_b);
  EXPECT_EQ(file_set.FileAt(file_b->end()), file_b);
  EXPECT_EQ(file_set.FileAt(file_b->end() + 1), file_b);
  EXPECT_EQ(file_set.FileAt(file_b->end() + 2), nullptr);
}

}  // namespace common::positions
