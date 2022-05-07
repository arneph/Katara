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
    common::fail("token has no associated string");
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

}  // namespace ir_serialization
