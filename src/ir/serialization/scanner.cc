//
//  scanner.cc
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "scanner.h"

#include <cctype>

#include "src/common/logging/logging.h"

namespace ir_serialization {

std::string Scanner::TokenToString(Token token) {
  switch (token) {
    case kUnknown:
      return "unknown";
    case kIdentifier:
      return "identifier";
    case kNumber:
      return "number";
    case kAddress:
      return "address";
    case kArrow:
      return "'=>'";
    case kEoF:
      return "end of file";
    case kNewLine:
      return "new line";
    default:
      return "'" + std::string(1, token) + "'";
  }
}

std::string Scanner::PositionString() const {
  return std::to_string(line_) + ":" + std::to_string(column_);
}

std::string Scanner::token_text() const {
  if (token_ == kUnknown || token_ == kEoF) {
    common::fail("token has no associated text");
  }
  return token_text_;
}

common::Int Scanner::token_number() const {
  if (token_ != kNumber) {
    common::fail("token has no associated number");
  }
  std::optional<common::Int> number = common::ToI64(token_text_);
  if (number.has_value()) {
    return *number;
  }
  number = common::ToU64(token_text_);
  if (number.has_value()) {
    return *number;
  }
  common::fail("number can not be represented");
}

common::Int Scanner::token_address() const {
  if (token_ != kAddress) {
    common::fail("token has no associated address");
  }
  std::optional<common::Int> address = common::ToU64(token_text_, /*base=*/16);
  if (!address.has_value()) {
    common::fail("address can not be represented");
  }
  return *address;
}

std::string Scanner::token_string() const {
  if (token_ != kString) {
    common::fail("token has no associated string");
  }
  std::string str;
  for (std::size_t i = 1; i < token_text_.length() - 1; i++) {
    char c = token_text_.at(i);
    if (c == '\\') {
      c = token_text_.at(++i);
    }
    str += std::string(1, c);
  }
  return str;
}

void Scanner::Next() {
  if (token_ == kEoF) {
    common::fail("can not advance Scanner at EoF");
  } else if (token_ == kNewLine) {
    line_++;
    column_ = 1;
  } else {
    column_ += token_text_.size();
  }
  SkipWhitespace();

  char c = in_stream_.peek();
  switch (c) {
    case EOF:
      token_ = kEoF;
      return;
    case '\n':
    case '#':
    case '%':
    case ':':
    case '{':
    case '}':
    case '@':
    case ',':
    case '(':
    case ')':
    case '<':
    case '>':
      in_stream_.get();
      token_ = (Token)c;
      token_text_ = std::string(1, c);
      return;
    case '=':
      in_stream_.get();
      token_ = kEqualSign;
      token_text_ = "=";
      if (in_stream_.peek() == '>') {
        in_stream_.get();
        token_ = kArrow;
        token_text_ = "=>";
      }
      return;
    case '"':
      NextString();
      return;
    default:
      break;
  }
  if (std::isalpha(c)) {
    NextIdentifier();
  } else if (c == '+' || c == '-' || std::isdigit(c)) {
    NextNumberOrAddress();
  } else {
    in_stream_.get();
    token_ = kUnknown;
  }
}

void Scanner::SkipWhitespace() {
  for (char c = in_stream_.peek(); c != '\n' && std::isspace(c); c = in_stream_.peek()) {
    in_stream_.get();
    column_++;
  }
}

void Scanner::NextIdentifier() {
  token_ = kIdentifier;
  token_text_ = std::string(1, in_stream_.get());
  for (char c = in_stream_.peek(); std::isalnum(c) || c == '_'; c = in_stream_.peek()) {
    token_text_ += std::string(1, in_stream_.get());
  }
}

void Scanner::NextNumberOrAddress() {
  token_ = kNumber;
  token_text_ = std::string(1, in_stream_.get());
  for (char c = in_stream_.peek(); std::isalnum(c); c = in_stream_.peek()) {
    token_text_ += std::string(1, in_stream_.get());
  }
  if (token_text_.starts_with("0x") || token_text_.starts_with("0X")) {
    token_ = kAddress;
  }
}

void Scanner::NextString() {
  token_ = kString;
  token_text_ = std::string(1, in_stream_.get());
  for (char c = in_stream_.get(); c != '"'; c = in_stream_.get()) {
    token_text_ += std::string(1, c);
    if (c == '\\') {
      token_text_ += std::string(1, in_stream_.get());
    }
  }
  token_text_ += std::string(1, '"');
}

int64_t Scanner::ConsumeInt64() {
  if (token_ != Scanner::kNumber) {
    FailForUnexpectedToken({kNumber});
  }
  int64_t number = token_number().AsInt64();
  Next();
  return number;
}

std::string Scanner::ConsumeIdentifier() {
  if (token_ != Scanner::kIdentifier) {
    FailForUnexpectedToken({kIdentifier});
  }
  std::string identifier = token_text();
  Next();
  return identifier;
}

void Scanner::ConsumeToken(Scanner::Token token) {
  if (token_ != token) {
    FailForUnexpectedToken({token});
  }
  Next();
}

void Scanner::FailForUnexpectedToken(std::vector<Token> expected_tokens) {
  std::string error = PositionString() + ": expected ";
  for (std::size_t i = 0; i < expected_tokens.size(); i++) {
    Scanner::Token expected_token = expected_tokens.at(i);
    if (i > 0) {
      error += ", ";
      if (i == expected_tokens.size() - 1) {
        error += "or ";
      }
    }
    error += Scanner::TokenToString(expected_token);
  }
  error += "; got ";
  switch (token()) {
    case kUnknown:
    case kNewLine:
    case kEoF:
      error += Scanner::TokenToString(token());
      break;
    default:
      error += "'" + token_text() + "'";
      break;
  }
  common::fail(error);
}

}  // namespace ir_serialization
