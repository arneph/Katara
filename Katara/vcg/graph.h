//
//  graph.h
//  Katara
//
//  Created by Arne Philipeit on 1/4/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef vcg_graph_h
#define vcg_graph_h

#include <string>
#include <vector>

#include "edge.h"
#include "node.h"

namespace vcg {

class Graph {
public:
    Graph();
    ~Graph();
    
    std::vector<Node>& nodes();
    std::vector<Edge>& edges();
    
    std::string ToVCGFormat(bool exclude_node_text = false) const;
    
private:
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
};

}

#endif /* vcg_graph_h */
