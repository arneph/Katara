//
//  scanner_test.cc
//  Katara
//
//  Created by Arne Philipeit on 3/27/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/serialization/scanner.h"

#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"

namespace {

using ::common::positions::File;
using ::common::positions::FileSet;
using ::testing::IsEmpty;
using ::testing::SizeIs;

TEST(ScannerTest, HandlesEmptyStream) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kUnknown);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, HandlesConsumeTokenFailure) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "@");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAtSign);
  EXPECT_EQ(scanner.token_text(), "@");
  EXPECT_THAT(issue_tracker.issues(), IsEmpty());

  scanner.ConsumeToken(::ir_serialization::Scanner::Token::kParenOpen);
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));

  scanner.ConsumeToken(::ir_serialization::Scanner::Token::kParenClose);
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(2));
}

TEST(ScannerTest, ScansIdentifier) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "something");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kIdentifier);
  EXPECT_EQ(scanner.token_text(), "something");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, ScansIdentifierWithNumbersAndUnderscores) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "Something_Else_42");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kIdentifier);
  EXPECT_EQ(scanner.token_text(), "Something_Else_42");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, ConsumesIdentifier) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "something");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kIdentifier);
  EXPECT_EQ(scanner.token_text(), "something");

  std::optional<std::string> ident = scanner.ConsumeIdentifier();
  EXPECT_TRUE(ident.has_value());
  EXPECT_EQ(*ident, "something");
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, HandlesConsumeIdentifierFailure) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "@");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAtSign);
  EXPECT_EQ(scanner.token_text(), "@");
  EXPECT_THAT(issue_tracker.issues(), IsEmpty());

  std::optional<std::string> ident = scanner.ConsumeIdentifier();
  EXPECT_FALSE(ident.has_value());
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));
}

TEST(ScannerTest, HandlesConsumeIdentifierFailureDueToEoF) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  std::optional<std::string> ident = scanner.ConsumeIdentifier();
  EXPECT_FALSE(ident.has_value());
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));
}

TEST(ScannerTest, ScansNumber) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "123");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNumber);
  EXPECT_EQ(scanner.token_text(), "123");
  EXPECT_EQ(scanner.token_number().AsInt64(), 123);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, ScansNegativeNumber) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "-123");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNumber);
  EXPECT_EQ(scanner.token_text(), "-123");
  EXPECT_EQ(scanner.token_number().AsInt64(), -123);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, HandlesUnrepresentableNumber) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "+");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNumber);
  EXPECT_EQ(scanner.token_text(), "+");
  EXPECT_EQ(scanner.token_number().AsInt64(), 0);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));
}

TEST(ScannerTest, ConsumesInt64) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "12345");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNumber);
  EXPECT_EQ(scanner.token_text(), "12345");
  EXPECT_EQ(scanner.token_number().AsInt64(), 12345);

  std::optional<int64_t> int64 = scanner.ConsumeInt64();
  EXPECT_TRUE(int64.has_value());
  EXPECT_EQ(*int64, 12345);
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, HandlesConsumeInt64Failure) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "@");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAtSign);
  EXPECT_EQ(scanner.token_text(), "@");
  EXPECT_THAT(issue_tracker.issues(), IsEmpty());

  std::optional<int64_t> int64 = scanner.ConsumeInt64();
  EXPECT_FALSE(int64.has_value());
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));
}

TEST(ScannerTest, HandlesConsumeInt64FailureDueToEoF) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  std::optional<int64_t> int64 = scanner.ConsumeInt64();
  EXPECT_FALSE(int64.has_value());
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));
}

TEST(ScannerTest, ScansAddress) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "0x123");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAddress);
  EXPECT_EQ(scanner.token_text(), "0x123");
  EXPECT_EQ(scanner.token_address().AsUint64(), 291);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, HandlesUnrepresentableAddress) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "0x");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAddress);
  EXPECT_EQ(scanner.token_text(), "0x");
  EXPECT_EQ(scanner.token_address().AsUint64(), 0);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));
}

TEST(ScannerTest, ScansString) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "\"\\\"hello\\\" \\--\\\\-- \\\"world\\\"\"");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kString);
  EXPECT_EQ(scanner.token_text(), "\"\\\"hello\\\" \\--\\\\-- \\\"world\\\"\"");
  EXPECT_EQ(scanner.token_string(), "\"hello\" --\\-- \"world\"");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, HandlesEOFInsteadOfEscapedCharacter) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "\"abc\\");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kUnknown);

  ASSERT_THAT(issue_tracker.issues(), SizeIs(1));
  EXPECT_EQ(issue_tracker.issues().at(0).kind(),
            ir_issues::IssueKind::kEOFInsteadOfEscapedCharacter);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));
}

TEST(ScannerTest, HandlesEOFInsteadOfStringEndQuote) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "\"abc");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kUnknown);

  ASSERT_THAT(issue_tracker.issues(), SizeIs(1));
  EXPECT_EQ(issue_tracker.issues().at(0).kind(), ir_issues::IssueKind::kEOFInsteadOfStringEndQuote);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));
}

TEST(ScannerTest, ScansCharacterTokens) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", ")(,@}{:%#\n==>><=");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenClose);
  EXPECT_EQ(scanner.token_text(), ")");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenOpen);
  EXPECT_EQ(scanner.token_text(), "(");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kComma);
  EXPECT_EQ(scanner.token_text(), ",");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAtSign);
  EXPECT_EQ(scanner.token_text(), "@");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kCurlyBracketClose);
  EXPECT_EQ(scanner.token_text(), "}");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kCurlyBracketOpen);
  EXPECT_EQ(scanner.token_text(), "{");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kColon);
  EXPECT_EQ(scanner.token_text(), ":");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kPercentSign);
  EXPECT_EQ(scanner.token_text(), "%");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kHashSign);
  EXPECT_EQ(scanner.token_text(), "#");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNewLine);
  EXPECT_EQ(scanner.token_text(), "\n");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEqualSign);
  EXPECT_EQ(scanner.token_text(), "=");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kArrow);
  EXPECT_EQ(scanner.token_text(), "=>");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAngleClose);
  EXPECT_EQ(scanner.token_text(), ">");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAngleOpen);
  EXPECT_EQ(scanner.token_text(), "<");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEqualSign);
  EXPECT_EQ(scanner.token_text(), "=");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, ProvidesCorrectPositions) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir",
                                "@0 main () => () {\n"
                                "  ret\n"
                                "}");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAtSign);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 0);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNumber);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 1);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kIdentifier);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 3);
  EXPECT_EQ(file_set.PositionFor(scanner.token_end()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_end()).column_, 6);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenOpen);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 8);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenClose);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 9);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kArrow);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 11);
  EXPECT_EQ(file_set.PositionFor(scanner.token_end()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_end()).column_, 12);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenOpen);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 14);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenClose);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 15);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kCurlyBracketOpen);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 17);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNewLine);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 1);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 18);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kIdentifier);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 2);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 2);
  EXPECT_EQ(file_set.PositionFor(scanner.token_end()).line_, 2);
  EXPECT_EQ(file_set.PositionFor(scanner.token_end()).column_, 4);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNewLine);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 2);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 5);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kCurlyBracketClose);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 3);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 0);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).line_, 3);
  EXPECT_EQ(file_set.PositionFor(scanner.token_start()).column_, 1);
  EXPECT_EQ(scanner.token_start(), scanner.token_end());

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, HandlesUnexpectedToken) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "hello");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  scanner.ConsumeToken(::ir_serialization::Scanner::Token::kAtSign);
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  ASSERT_THAT(issue_tracker.issues(), SizeIs(1));
  EXPECT_EQ(issue_tracker.issues().at(0).kind(), ir_issues::IssueKind::kUnexpectedToken);

  EXPECT_THAT(issue_tracker.issues(), SizeIs(1));
}

TEST(ScannerTest, SkipsPastOneToken) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "@{hello}42");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  scanner.SkipPastTokenSequence({::ir_serialization::Scanner::Token::kCurlyBracketClose});
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNumber);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, SkipsPastMultipleTokens) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "@{(hello)hello}42");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  scanner.SkipPastTokenSequence({::ir_serialization::Scanner::Token::kIdentifier,
                                 ::ir_serialization::Scanner::Token::kCurlyBracketClose});
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNumber);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

TEST(ScannerTest, SkipsToEOF) {
  FileSet file_set;
  File* file = file_set.AddFile("test.ir", "@{hello}42");
  ir_issues::IssueTracker issue_tracker(&file_set);

  ir_serialization::Scanner scanner(file, issue_tracker);

  scanner.Next();
  scanner.SkipPastTokenSequence({::ir_serialization::Scanner::Token::kParenOpen});
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);

  EXPECT_THAT(issue_tracker.issues(), IsEmpty());
}

}  // namespace
