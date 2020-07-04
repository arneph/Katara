//
//  positions.h
//  Katara
//
//  Created by Arne Philipeit on 7/4/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_positions_h
#define lang_positions_h

#include <memory>
#include <string>

namespace lang {
namespace pos {

typedef int64_t pos_t;
typedef struct{
    int64_t line_;
    int64_t column_;
    
    pos_t line_start_;
    pos_t line_end_;
} position_t;

constexpr pos_t kNoPos = -1;

position_t pos_to_position(std::string raw, pos_t pos);

}
}

#endif /* lang_positions_h */
