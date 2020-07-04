//
//  positions.cc
//  Katara
//
//  Created by Arne Philipeit on 7/4/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "positions.h"

namespace lang {
namespace pos {

position_t pos_to_position(std::string raw, pos_t pos) {
    int64_t line = 1;
    pos_t line_start = 0;
    pos_t line_end = raw.size() - 1;
    
    for (pos_t i = 0; i < pos; i++) {
        if (raw.at(i) != '\n') {
            continue;
        }
        line++;
        line_start = i + 1;
    }
    for (pos_t i = pos; i < raw.size(); i++) {
        if (raw.at(i) == '\n') {
            line_end = i;
            break;
        }
    }
    
    return position_t{
        line,
        pos - line_start,
        line_start,
        line_end
    };
}

}
}
