//
//  num_types.h
//  Katara
//
//  Created by Arne Philipeit on 5/1/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_num_types_h
#define ir_num_types_h

#include <cstdint>

namespace ir {

typedef int64_t func_num_t;
constexpr func_num_t kNoFuncNum = -1;

typedef int64_t block_num_t;
constexpr block_num_t kNoBlockNum = -1;

typedef int64_t value_num_t;
constexpr value_num_t kNoValueNum = -1;

typedef int64_t tree_num_t;  // tree index in dominator tree

}  // namespace ir

#endif /* ir_num_types_h */
