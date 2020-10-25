//
//  tokens.h
//  Katara
//
//  Created by Arne Philipeit on 5/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_tokens_h
#define lang_tokens_h

#include <memory>
#include <string>

namespace lang {
namespace tokens {

typedef int8_t precedence_t;

typedef enum : int8_t {
    // Special tokens:
    kIllegal,
    kEOF,
    kComment,
    
    // Literals:
    kIdent,  // main
    kInt,    // 12345
    kChar,   // 'a'
    kString, // "abc"
    
    // Operators and delimiters:
    kAdd,   // +
    kSub,   // -
    kMul,   // *
    kQuo,   // /
    kRem,   // %
    
    kAnd,       // &
    kOr,        // |
    kXor,       // ^
    kShl,       // <<
    kShr,       // >>
    kAndNot,    // &^
    
    kAddAssign, // +=
    kSubAssign, // -=
    kMulAssign, // *=
    kQuoAssign, // /=
    kRemAssign, // %=
    
    kAndAssign,     // &=
    kOrAssign,      // |=
    kXorAssign,     // ^=
    kShlAssign,     // <<=
    kShrAssign,     // >>=
    kAndNotAssign,  // &^=
    
    kLAnd,  // &&
    kLOr,   // ||
    kInc,   // ++
    kDec,   // --
    
    kEql,       // ==
    kLss,       // <
    kGtr,       // >
    kAssign,    // =
    kNot,       // !
    
    kNeq,       // !=
    kLeq,       // <=
    kGeq,       // >=
    kDefine,    // :=
    
    kLParen,    // (
    kLBrack,    // [
    kLBrace,    // {
    kComma,     // ,
    kPeriod,    // .
    
    kRParen,    // )
    kRBrack,    // ]
    kRBrace,    // }
    kSemicolon, // ;
    kColon,     // :
    
    // Keywords:
    kConst,
    kVar,
    kType,
    kInterface,
    kStruct,
    kIf,
    kElse,
    kFor,
    kSwitch,
    kCase,
    kDefault,
    kFallthrough,
    kContinue,
    kBreak,
    kReturn,
    kFunc,
    kPackage,
    kImport,
} Token;

constexpr precedence_t kMaxPrecedence = 5;

precedence_t prececende(Token token);

}
}

#endif /* lang_tokens_h */
