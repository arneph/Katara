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

namespace {

using ::testing::IsEmpty;

TEST(ScannerTest, HandlesEmptyStream) {
  std::stringstream ss;
  ir_serialization::Scanner scanner(ss);

  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kUnknown);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
}

TEST(ScannerTest, ScansIdentifier) {
  std::stringstream ss;
  ss << "something";
  ir_serialization::Scanner scanner(ss);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kIdentifier);
  EXPECT_EQ(scanner.token_text(), "something");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
}

TEST(ScannerTest, ScansIdentifierWithNumbersAndUnderscores) {
  std::stringstream ss;
  ss << "Something_Else_42";
  ir_serialization::Scanner scanner(ss);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kIdentifier);
  EXPECT_EQ(scanner.token_text(), "Something_Else_42");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
}

TEST(ScannerTest, ScansNumber) {
  std::stringstream ss;
  ss << "123";
  ir_serialization::Scanner scanner(ss);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNumber);
  EXPECT_EQ(scanner.token_text(), "123");
  EXPECT_EQ(scanner.token_number().AsInt64(), 123);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
}

TEST(ScannerTest, ScansAddress) {
  std::stringstream ss;
  ss << "0x123";
  ir_serialization::Scanner scanner(ss);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAddress);
  EXPECT_EQ(scanner.token_text(), "0x123");
  EXPECT_EQ(scanner.token_address().AsUint64(), 291);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
}

TEST(ScannerTest, ScansString) {
  std::stringstream ss;
  ss << "\"\\\"hello\\\" \\--\\\\-- \\\"world\\\"\"";
  ir_serialization::Scanner scanner(ss);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kString);
  EXPECT_EQ(scanner.token_text(), "\"\\\"hello\\\" \\--\\\\-- \\\"world\\\"\"");
  EXPECT_EQ(scanner.token_string(), "\"hello\" --\\-- \"world\"");

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
}

TEST(ScannerTest, ScansCharacterTokens) {
  std::stringstream ss;
  ss << ")(,@}{:%#\n==>><=";
  ir_serialization::Scanner scanner(ss);

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
}

TEST(ScannerTest, ProvidesCorrectPositions) {
  std::stringstream ss;
  ss << "@0 main () => () {\n"
     << "  ret\n"
     << "}";
  ir_serialization::Scanner scanner(ss);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kAtSign);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 1);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNumber);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 2);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kIdentifier);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 4);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenOpen);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 9);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenClose);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 10);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kArrow);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 12);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenOpen);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 15);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kParenClose);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 16);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kCurlyBracketOpen);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 18);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNewLine);
  EXPECT_EQ(scanner.line(), 1);
  EXPECT_EQ(scanner.column(), 19);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kIdentifier);
  EXPECT_EQ(scanner.line(), 2);
  EXPECT_EQ(scanner.column(), 3);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kNewLine);
  EXPECT_EQ(scanner.line(), 2);
  EXPECT_EQ(scanner.column(), 6);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kCurlyBracketClose);
  EXPECT_EQ(scanner.line(), 3);
  EXPECT_EQ(scanner.column(), 1);

  scanner.Next();
  EXPECT_EQ(scanner.token(), ::ir_serialization::Scanner::Token::kEoF);
  EXPECT_EQ(scanner.line(), 3);
  EXPECT_EQ(scanner.column(), 2);
}

}  // namespace
