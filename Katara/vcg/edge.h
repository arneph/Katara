//
//  edge.h
//  Katara
//
//  Created by Arne Philipeit on 1/5/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef vcg_edge_h
#define vcg_edge_h

#include "node.h"

namespace vcg {

class Edge {
public:
    Edge(int64_t source_number,
         int64_t target_number,
         bool is_directed = false);
    ~Edge();
    
    int64_t source_number() const;
    int64_t target_number() const;
    bool is_directed() const;
    
private:
    const int64_t source_number_;
    const int64_t target_number_;
    const bool is_directed_;
};

}

#endif /* vcg_edge_h */
