//
//  graph.cc
//  Katara
//
//  Created by Arne Philipeit on 1/4/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "graph.h"

#include <iomanip>
#include <sstream>

namespace vcg {

Graph::Graph() {}
Graph::~Graph() {}

std::vector<Node>& Graph::nodes() { return nodes_; }

std::vector<Edge>& Graph::edges() { return edges_; }

std::string Graph::ToVCGFormat(bool exclude_node_text) const {
  std::stringstream ss;
  ss << "graph: { title: \"Graph\"\n";

  for (Node node : nodes_) {
    ss << "node: {\n";
    ss << "title: \"" << node.number() << "\"\n";
    ss << "color: " + to_string(node.color()) << "\n";
    ss << "label: \n";
    std::string label = node.title();
    if (!exclude_node_text && !node.text().empty()) {
      label += "\n" + node.text();
    }
    ss << std::quoted(label);
    ss << "\n";
    ss << "}\n";
  }

  for (Edge edge : edges_) {
    ss << "edge: { sourcename: \"";
    ss << edge.source_number();
    ss << "\" targetname: \"";
    ss << edge.target_number();
    ss << "\" arrowstyle: ";
    if (edge.is_directed())
      ss << "solid";
    else
      ss << "none";
    ss << "}\n";
  }

  ss << "}";

  return ss.str();
}

}  // namespace vcg
