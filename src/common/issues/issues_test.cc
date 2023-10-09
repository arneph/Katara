//
//  issues_test.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "src/common/issues/issues.h"

#include <sstream>
#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/common/positions/positions.h"

namespace common::issues {

using ::testing::IsEmpty;

enum class TestIssueKind {
  kCodeSmellsBad = 123,
  kCodeIsUgly = 456,
};

enum class TestOrigin {
  kNose,
  kEyes,
};

class TestIssue : public Issue<TestIssueKind, TestOrigin> {
 public:
  TestIssue(TestIssueKind kind, std::vector<common::positions::range_t> positions,
            std::string message)
      : Issue<TestIssueKind, TestOrigin>(kind, positions, message) {}

  TestOrigin origin() const override {
    return kind() == TestIssueKind::kCodeSmellsBad ? TestOrigin::kNose : TestOrigin::kEyes;
  }
  common::issues::Severity severity() const override {
    return kind() == TestIssueKind::kCodeSmellsBad ? Severity::kError : Severity::kWarning;
  }
};

typedef IssueTracker<TestIssueKind, TestOrigin, TestIssue> TestIssueTracker;

TEST(PrintIssuesTest, HandlesNoIssues) {
  TestIssueTracker issue_tracker(/*file_set=*/nullptr);
  std::stringstream ss_plain;
  std::stringstream ss_terminal;

  issue_tracker.PrintIssues(Format::kPlain, &ss_plain);
  EXPECT_THAT(ss_plain.str(), IsEmpty());

  issue_tracker.PrintIssues(Format::kPlain, &ss_terminal);
  EXPECT_THAT(ss_terminal.str(), IsEmpty());
}

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

TEST(PrintIssuesTest, HandlesOneIssueWithOnePositionOnSingleLine) {
  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("test.txt", std::string(kTestFileAContents));
  common::positions::range_t line_6 = file->RangeOfLineWithNumber(6);

  TestIssueTracker issue_tracker(&file_set);
  issue_tracker.Add(
      TestIssueKind::kCodeSmellsBad,
      common::positions::range_t{.start = line_6.start + 13, .end = line_6.start + 19},
      "The word ullamco smells bad!");
  std::stringstream ss;

  issue_tracker.PrintIssues(Format::kPlain, &ss);
  EXPECT_EQ(ss.str(), R"txt(Error: The word ullamco smells bad! [123]
  test.txt:6:13: exercitation ullamco laboris nisi ut aliquip ex
                              ~~~~~~~
)txt");
}

TEST(PrintIssuesTest, HandlesMultipleIssuesWithOnePositionOnSingleLines) {
  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("test.txt", std::string(kTestFileAContents));
  common::positions::range_t line_6 = file->RangeOfLineWithNumber(6);
  common::positions::range_t line_12 = file->RangeOfLineWithNumber(12);

  TestIssueTracker issue_tracker(&file_set);
  issue_tracker.Add(
      TestIssueKind::kCodeIsUgly,
      common::positions::range_t{.start = line_12.start + 26, .end = line_12.start + 26},
      "You should have used an exlamation mark.");
  issue_tracker.Add(
      TestIssueKind::kCodeSmellsBad,
      common::positions::range_t{.start = line_6.start + 13, .end = line_6.start + 19},
      "The word ullamco smells bad!");
  std::stringstream ss;

  issue_tracker.PrintIssues(Format::kPlain, &ss);
  EXPECT_EQ(ss.str(), R"txt(Warning: You should have used an exlamation mark. [456]
  test.txt:12:26: mollit anim id est laborum.
                                            ^
Error: The word ullamco smells bad! [123]
  test.txt:6:13: exercitation ullamco laboris nisi ut aliquip ex
                              ~~~~~~~
)txt");
}

TEST(PrintIssuesTest, HandlesOneIssueWithMultiplePositionsOnMultipleLines) {
  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("test.txt", std::string(kTestFileAContents));
  common::positions::range_t line_6 = file->RangeOfLineWithNumber(6);
  common::positions::range_t line_9 = file->RangeOfLineWithNumber(9);

  TestIssueTracker issue_tracker(&file_set);
  issue_tracker.Add(
      TestIssueKind::kCodeSmellsBad,
      {common::positions::range_t{.start = line_9.start + 17, .end = line_9.start + 22},
       common::positions::range_t{.start = line_6.start + 13, .end = line_6.start + 19}},
      "The words ullamco and fugiat smell bad!");
  std::stringstream ss;

  issue_tracker.PrintIssues(Format::kPlain, &ss);
  EXPECT_EQ(ss.str(), R"txt(Error: The words ullamco and fugiat smell bad! [123]
  test.txt:6:13: exercitation ullamco laboris nisi ut aliquip ex
                              ~~~~~~~
  test.txt:9:17: cillum dolore eu fugiat nulla pariatur.
                                  ~~~~~~
)txt");
}

TEST(PrintIssuesTest, HandlesOneIssueWithMultiplePositionsWithoutConflictsOnOneLine) {
  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("test.txt", std::string(kTestFileAContents));
  common::positions::range_t line_6 = file->RangeOfLineWithNumber(6);

  TestIssueTracker issue_tracker(&file_set);
  issue_tracker.Add(
      TestIssueKind::kCodeIsUgly,
      {common::positions::range_t{.start = line_6.start + 37, .end = line_6.start + 43},
       common::positions::range_t{.start = line_6.start + 13, .end = line_6.start + 19}},
      "The words ullamco and aliquip are ugly.");
  std::stringstream ss;

  issue_tracker.PrintIssues(Format::kPlain, &ss);
  EXPECT_EQ(ss.str(), R"txt(Warning: The words ullamco and aliquip are ugly. [456]
  test.txt:6: exercitation ullamco laboris nisi ut aliquip ex
                           ~~~~~~~                 ~~~~~~~
)txt");
}

TEST(PrintIssuesTest, HandlesOneIssueWithMultiplePositionsWithConflictsOnOneLine) {
  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("test.txt", std::string(kTestFileAContents));
  common::positions::range_t line_6 = file->RangeOfLineWithNumber(6);

  TestIssueTracker issue_tracker(&file_set);
  issue_tracker.Add(
      TestIssueKind::kCodeSmellsBad,
      {common::positions::range_t{.start = line_6.start + 13, .end = line_6.start + 19},
       common::positions::range_t{.start = line_6.start + 21, .end = line_6.start + 32},
       common::positions::range_t{.start = line_6.start, .end = line_6.start + 27}},
      "The word ullamco cannot be used in this context.");
  std::stringstream ss;

  issue_tracker.PrintIssues(Format::kPlain, &ss);
  EXPECT_EQ(ss.str(), R"txt(Error: The word ullamco cannot be used in this context. [123]
  test.txt:6: exercitation ullamco laboris nisi ut aliquip ex
              ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                           ~~~~~~~ ~~~~~~~~~~~~
)txt");
}

TEST(PrintIssuesTest, HandlesOneIssueWithOnePositionOnMulipleLines) {
  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("test.txt", std::string(kTestFileAContents));
  common::positions::range_t line_7 = file->RangeOfLineWithNumber(7);
  common::positions::range_t line_9 = file->RangeOfLineWithNumber(9);

  TestIssueTracker issue_tracker(&file_set);
  issue_tracker.Add(TestIssueKind::kCodeSmellsBad,
                    common::positions::range_t{.start = line_7.start + 22, .end = line_9.end},
                    "This sentence is too long.");
  std::stringstream ss;

  issue_tracker.PrintIssues(Format::kPlain, &ss);
  EXPECT_EQ(ss.str(), R"txt(Error: This sentence is too long. [123]
  test.txt:
       |                       v~~~~~~~~~~~~~~
    07 | ea commodo consequat. Duis aute irure
    08 | dolor in reprehenderit in voluptate velit esse
    09 | cillum dolore eu fugiat nulla pariatur.
       | ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^
)txt");
}

TEST(PrintIssuesTest, HandlesSeveralIssuesAcrossFiles) {
  common::positions::FileSet file_set;
  common::positions::File* file_b = file_set.AddFile("testB.txt", std::string(kTestFileBContents));
  common::positions::range_t line_b2 = file_b->RangeOfLineWithNumber(2);

  common::positions::File* file_a = file_set.AddFile("testA.txt", std::string(kTestFileAContents));
  common::positions::range_t line_a2 = file_a->RangeOfLineWithNumber(2);
  common::positions::range_t line_a6 = file_a->RangeOfLineWithNumber(6);
  common::positions::range_t line_a7 = file_a->RangeOfLineWithNumber(7);
  common::positions::range_t line_a9 = file_a->RangeOfLineWithNumber(9);

  TestIssueTracker issue_tracker(&file_set);
  issue_tracker.Add(
      TestIssueKind::kCodeIsUgly,
      {common::positions::range_t{.start = line_a2.start, .end = line_a2.start + 4},
       common::positions::range_t{.start = line_b2.start + 20, .end = line_b2.start + 23}},
      "One text is pseudo-Latin the other English.");
  issue_tracker.Add(TestIssueKind::kCodeSmellsBad,
                    common::positions::range_t{.start = line_a7.start + 22, .end = line_a9.end},
                    "This sentence is too long.");
  issue_tracker.Add(
      TestIssueKind::kCodeSmellsBad,
      {common::positions::range_t{.start = line_a6.start + 13, .end = line_a6.start + 19},
       common::positions::range_t{.start = line_a6.start + 21, .end = line_a6.start + 32},
       common::positions::range_t{.start = line_a6.start, .end = line_a6.start + 27}},
      "The word ullamco cannot be used in this context.");
  std::stringstream ss;

  issue_tracker.PrintIssues(Format::kPlain, &ss);
  EXPECT_EQ(ss.str(), R"txt(Warning: One text is pseudo-Latin the other English. [456]
  testA.txt:2: dolor sit amet, consectetur
               ~~~~~
  testB.txt:2:20: choose to go to the Moon in this decade and do the other things, not
                                      ~~~~
Error: This sentence is too long. [123]
  testA.txt:
       |                       v~~~~~~~~~~~~~~
    07 | ea commodo consequat. Duis aute irure
    08 | dolor in reprehenderit in voluptate velit esse
    09 | cillum dolore eu fugiat nulla pariatur.
       | ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^
Error: The word ullamco cannot be used in this context. [123]
  testA.txt:6: exercitation ullamco laboris nisi ut aliquip ex
               ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                            ~~~~~~~ ~~~~~~~~~~~~
)txt");
}

}  // namespace common::issues
