//
//  scanner.h
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_proc_scanner_h
#define ir_proc_scanner_h

#include <istream>
#include <memory>
#include <string>

namespace ir_proc {

class Scanner {
public:
    typedef enum : char {
        kUnknown = 0,
        kIdentifier = 1,
        kNumber = 2,
        kArrow = 3,
        kEoF = EOF,
        kNewLine = '\n',
        kHashSign = '#',
        kPercentSign = '%',
        kColon = ':',
        kCurlyBracketOpen = '{',
        kCurlyBracketClose = '}',
        kAtSign = '@',
        kComma = ',',
        kEqualSign = '=',
        kRoundBracketOpen = '(',
        kRoundBracketClose = ')'
    } Token;
    
    Scanner(std::istream& in_stream);
    ~Scanner();
    
    Token token() const;
    std::string string() const;
    int64_t sign() const;
    uint64_t number() const;
    
    void Next();
    
private:
    std::istream& in_stream_;
    
    Token token_;
    std::string string_;
    int64_t sign_;
    uint64_t number_;
};

}

#endif /* ir_proc_scanner_h */
