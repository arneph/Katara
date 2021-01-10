//
//  scanner.cc
//  Katara
//
//  Created by Arne Philipeit on 5/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "scanner.h"

namespace lang {
namespace scanner {

Scanner::Scanner(pos::File* file) : file_(file), pos_(file->start()), tok_(tokens::kIllegal) {
  Next();
}

tokens::Token Scanner::token() const { return tok_; }

int64_t Scanner::token_start() const { return tok_start_; }

int64_t Scanner::token_end() const { return tok_end_; }

std::string Scanner::token_string() const { return file_->contents(tok_start_, tok_end_); }

void Scanner::Next(bool split_shift_ops) {
  bool insert_semicolon = false;
  switch (tok_) {
    case tokens::kIdent:
    case tokens::kInt:
    case tokens::kChar:
    case tokens::kString:
    case tokens::kFallthrough:
    case tokens::kContinue:
    case tokens::kBreak:
    case tokens::kReturn:
    case tokens::kInc:
    case tokens::kDec:
    case tokens::kGtr:
    case tokens::kRParen:
    case tokens::kRBrack:
    case tokens::kRBrace:
      insert_semicolon = true;
    default:;
  }
  for (; pos_ < file_->end() && (file_->at(pos_) == ' ' || file_->at(pos_) == '\t' ||
                                 (file_->at(pos_) == '\n' && !insert_semicolon));
       pos_++)
    ;
  tok_start_ = pos_;
  if (pos_ == file_->end()) {
    tok_ = tokens::kEOF;
    tok_end_ = pos_;
    return;
  }

  switch (file_->at(pos_++)) {
    case '\n':
      tok_ = tokens::kSemicolon;
      tok_end_ = pos_ - 1;
      return;
    case '+':
      if (pos_ < file_->end() && file_->at(pos_) == '+') {
        tok_ = tokens::kInc;
        tok_end_ = pos_++;
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kAdd);
      return;
    case '-':
      if (pos_ < file_->end() && file_->at(pos_) == '-') {
        tok_ = tokens::kDec;
        tok_end_ = pos_++;
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kSub);
      return;
    case '*':
      NextArithmeticOrBitOpStart(tokens::kMul);
      return;
    case '/':
      if (pos_ < file_->end() && file_->at(pos_) == '/') {
        for (; pos_ < file_->end() && file_->at(pos_) != '\n'; pos_++)
          ;
        tok_ = tokens::kComment;
        tok_end_ = pos_ - 1;
        return;
      } else if (pos_ < file_->end() && file_->at(pos_) == '*') {
        for (; pos_ < file_->end() - 1 && (file_->at(pos_) != '*' || file_->at(pos_ + 1) != '/');
             pos_++)
          ;
        tok_ = tokens::kComment;
        tok_end_ = (pos_ < file_->end() - 1) ? pos_ + 1 : pos_;
        pos_ += 2;
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kQuo);
      return;
    case '%':
      NextArithmeticOrBitOpStart(tokens::kRem);
      return;
    case '&':
      if (pos_ < file_->end() && file_->at(pos_) == '&') {
        tok_ = tokens::kLAnd;
        tok_end_ = pos_++;
        return;
      } else if (pos_ < file_->end() && file_->at(pos_) == '^') {
        pos_++;
        NextArithmeticOrBitOpStart(tokens::kAndNot);
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kAnd);
      return;
    case '|':
      if (pos_ < file_->end() && file_->at(pos_) == '|') {
        tok_ = tokens::kLOr;
        tok_end_ = pos_++;
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kOr);
      return;
    case '^':
      NextArithmeticOrBitOpStart(tokens::kXor);
      return;
    case '<':
      if (!split_shift_ops && pos_ < file_->end() && file_->at(pos_) == '<') {
        pos_++;
        NextArithmeticOrBitOpStart(tokens::kShl);
        return;
      } else if (pos_ < file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kLeq;
        tok_end_ = pos_++;
        return;
      } else {
        tok_ = tokens::kLss;
        tok_end_ = pos_ - 1;
        return;
      }
    case '>':
      if (!split_shift_ops && pos_ < file_->end() && file_->at(pos_) == '>') {
        pos_++;
        NextArithmeticOrBitOpStart(tokens::kShr);
        return;
      } else if (pos_ < file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kGeq;
        tok_end_ = pos_++;
        return;
      } else {
        tok_ = tokens::kGtr;
        tok_end_ = pos_ - 1;
        return;
      }
    case '=':
      if (pos_ < file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kEql;
        tok_end_ = pos_++;
        return;
      } else {
        tok_ = tokens::kAssign;
        tok_end_ = pos_ - 1;
        return;
      }
    case '!':
      if (pos_ < file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kNeq;
        tok_end_ = pos_++;
        return;
      } else {
        tok_ = tokens::kNot;
        tok_end_ = pos_ - 1;
        return;
      }
    case ':':
      if (pos_ < file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kDefine;
        tok_end_ = pos_++;
        return;
      } else {
        tok_ = tokens::kColon;
        tok_end_ = pos_ - 1;
        return;
      }
    case '(':
      tok_ = tokens::kLParen;
      tok_end_ = pos_ - 1;
      return;
    case '[':
      tok_ = tokens::kLBrack;
      tok_end_ = pos_ - 1;
      return;
    case '{':
      tok_ = tokens::kLBrace;
      tok_end_ = pos_ - 1;
      return;
    case ',':
      tok_ = tokens::kComma;
      tok_end_ = pos_ - 1;
      return;
    case '.':
      tok_ = tokens::kPeriod;
      tok_end_ = pos_ - 1;
      return;
    case ')':
      tok_ = tokens::kRParen;
      tok_end_ = pos_ - 1;
      return;
    case ']':
      tok_ = tokens::kRBrack;
      tok_end_ = pos_ - 1;
      return;
    case '}':
      tok_ = tokens::kRBrace;
      tok_end_ = pos_ - 1;
      return;
    case ';':
      tok_ = tokens::kSemicolon;
      tok_end_ = pos_ - 1;
      return;
    case '\'': {
      bool escaped = false;
      for (pos_++; pos_ < file_->end() && !escaped && file_->at(pos_) != '\''; pos_++) {
        if (escaped) {
          escaped = false;
        } else if (file_->at(pos_) == '\\') {
          escaped = true;
        }
      }
      tok_ = tokens::kChar;
      tok_end_ = pos_++;
      return;
    }
    case '\"': {
      bool escaped = false;
      for (pos_++; pos_ < file_->end() && !escaped && file_->at(pos_) != '\"'; pos_++) {
        if (escaped) {
          escaped = false;
        } else if (file_->at(pos_) == '\\') {
          escaped = true;
        }
      }
      tok_ = tokens::kString;
      tok_end_ = pos_++;
      return;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      for (; pos_ < file_->end() && '0' <= file_->at(pos_) && file_->at(pos_) <= '9'; pos_++)
        ;
      tok_ = tokens::kInt;
      tok_end_ = pos_ - 1;
      return;
  }

  for (; pos_ < file_->end() &&
         (('A' <= file_->at(pos_) && file_->at(pos_) <= 'Z') ||
          ('a' <= file_->at(pos_) && file_->at(pos_) <= 'z') ||
          ('0' <= file_->at(pos_) && file_->at(pos_) <= '9') || file_->at(pos_) == '_');
       pos_++)
    ;
  tok_ = tokens::kIdent;
  tok_end_ = pos_ - 1;

  std::string ident = file_->contents(tok_start_, tok_end_);
  if (ident == "const") {
    tok_ = tokens::kConst;
  } else if (ident == "var") {
    tok_ = tokens::kVar;
  } else if (ident == "type") {
    tok_ = tokens::kType;
  } else if (ident == "interface") {
    tok_ = tokens::kInterface;
  } else if (ident == "struct") {
    tok_ = tokens::kStruct;
  } else if (ident == "if") {
    tok_ = tokens::kIf;
  } else if (ident == "else") {
    tok_ = tokens::kElse;
  } else if (ident == "for") {
    tok_ = tokens::kFor;
  } else if (ident == "switch") {
    tok_ = tokens::kSwitch;
  } else if (ident == "case") {
    tok_ = tokens::kCase;
  } else if (ident == "default") {
    tok_ = tokens::kDefault;
  } else if (ident == "fallthrough") {
    tok_ = tokens::kElse;
  } else if (ident == "continue") {
    tok_ = tokens::kContinue;
  } else if (ident == "break") {
    tok_ = tokens::kBreak;
  } else if (ident == "return") {
    tok_ = tokens::kReturn;
  } else if (ident == "func") {
    tok_ = tokens::kFunc;
  } else if (ident == "import") {
    tok_ = tokens::kImport;
  } else if (ident == "package") {
    tok_ = tokens::kPackage;
  }
}

void Scanner::NextArithmeticOrBitOpStart(tokens::Token tok) {
  if (pos_ < file_->end() && file_->at(pos_) == '=') {
    tok_ = tokens::Token(tok + tokens::kAddAssign - tokens::kAdd);
    pos_++;
  } else {
    tok_ = tok;
  }
  tok_end_ = pos_ - 1;
}

void Scanner::SkipPastLine() {
  for (; pos_ < file_->end() && (file_->at(pos_) != '\n'); pos_++)
    ;
  Next();
}

}  // namespace scanner
}  // namespace lang
