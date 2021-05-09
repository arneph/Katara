//
//  graph.h
//  Katara
//
//  Created by Arne Philipeit on 1/4/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef common_graph_h
#define common_graph_h

#include <string>
#include <vector>

#include "edge.h"
#include "node.h"

namespace common {

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

}  // namespace common

#endif /* common_graph_h */
