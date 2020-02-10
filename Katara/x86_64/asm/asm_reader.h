//
//  asm_reader.h
//  Katara
//
//  Created by Arne Philipeit on 12/7/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_asm_reader_h
#define x86_64_asm_reader_h

#include <istream>

namespace x64 {

class ASMReader {
    std::istream input;
    
public:
    ASMReader(std::istream input);
    ~ASMReader();
    
    
};

}

#endif /* x86_64_asm_reader_h */
