//
//  scanner.h
//  Katara
//
//  Created by Arne Philipeit on 5/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_scanner_h
#define lang_scanner_h

#include <string>

#include "lang/positions.h"
#include "lang/token.h"

namespace lang {
namespace scanner {

class Scanner {
public:
    Scanner(std::string raw);
    
    token::Token token() const;
    pos::pos_t token_start() const;
    pos::pos_t token_end() const;
    std::string token_string() const;
    
    void Next();
    void SkipPastLine();
    
private:
    const std::string raw_;
    pos::pos_t pos_;
    
    token::Token tok_;
    pos::pos_t tok_start_;
    pos::pos_t tok_end_;
    
    void NextArithmeticOrBitOpStart(token::Token tok);
};

}
}

#endif /* lang_scanner_h */
