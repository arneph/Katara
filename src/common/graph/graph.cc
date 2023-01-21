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

namespace common::graph {

NodeBuilder& NodeBuilder::SetText(std::string text) {
  node_.text_ = text;
  return *this;
}
NodeBuilder& NodeBuilder::SetSubgraph(subgraph_num_t subgraph) {
  node_.subgraph_ = subgraph;
  return *this;
}
NodeBuilder& NodeBuilder::SetColor(Color color) {
  node_.color_ = color;
  return *this;
}

namespace {

void WriteEscapedNumberForDot(std::stringstream& ss, node_num_t number) {
  if (number < 0) {
    ss << "m" << (-number);
  } else {
    ss << number;
  }
}

void WriteEscapedStringForDot(std::stringstream& ss, std::string_view unescaped_string,
                              char line_allignment = 'l') {
  for (const char& c : unescaped_string) {
    switch (c) {
      case '\n':
        ss << "\\" << line_allignment;
        continue;
        ;
      case '"':
        ss << "\\\"";
        continue;
      default:
        ss << c;
    }
  }
}

std::string ToVCGString(Color color) {
  switch (color) {
    case kRed:
      return "red";
    case kYellow:
      return "yellow";
    case kGreen:
      return "green";
    case kBlue:
      return "blue";
    case kTurquoise:
      return "turquoise";
    case kMagenta:
      return "magenta";
    default:
      return "white";
  }
}

std::string ToDotString(Color color) {
  switch (color) {
    case kRed:
      return "#ff0000";
    case kYellow:
      return "#ffff00";
    case kGreen:
      return "#00ff00";
    case kBlue:
      return "#0000ff";
    case kTurquoise:
      return "#00ffff";
    case kMagenta:
      return "#ff007f";
    default:
      return "#ffffff";
  }
}

void WriteNodeForDot(std::stringstream& ss, const Node& node) {
  ss << "n";
  WriteEscapedNumberForDot(ss, node.number());
  ss << " [";
  ss << "label = \"";
  ss << node.title() << "\\l";
  if (!node.text().empty()) {
    WriteEscapedStringForDot(ss, node.text());
    ss << "\\l";
  }
  ss << "\", ";
  ss << "fillcolor = \"" << ToDotString(node.color()) << "\" style = \"filled\"";
  ss << ", shape = box, labeljust = l";
  ss << "];";
}

void WriteEdgeForDot(std::stringstream& ss, const Edge& edge, bool is_directed) {
  ss << "n";
  WriteEscapedNumberForDot(ss, edge.source_number());
  ss << (is_directed ? "->" : "--");
  ss << "n";
  WriteEscapedNumberForDot(ss, edge.target_number());
}

}  // namespace

bool Graph::UsesSubgraphs() const {
  for (const Node& node : nodes_) {
    if (node.subgraph() != kDefaultSubgraph) {
      return true;
    }
  }
  return false;
}

std::unordered_set<subgraph_num_t> Graph::Subgraphs() const {
  std::unordered_set<subgraph_num_t> subgraphs;
  for (const Node& node : nodes_) {
    subgraphs.insert(node.subgraph());
  }
  return subgraphs;
}

std::string Graph::ToVCGFormat() const {
  std::stringstream ss;
  ss << "graph: { title: \"Graph\"\n";

  for (Node node : nodes_) {
    ss << "node: {\n";
    ss << "title: \"" << node.number() << "\"\n";
    ss << "color: " + ToVCGString(node.color()) << "\n";
    ss << "label: \n";
    std::string label = node.title();
    if (!node.text().empty()) {
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
    if (is_directed_)
      ss << "solid";
    else
      ss << "none";
    ss << "}\n";
  }

  ss << "}";
  return ss.str();
}

std::string Graph::ToDotFormat() const {
  return UsesSubgraphs() ? ToDotFormatWithSubgraphs() : ToDotFormatWithoutSubgraphs();
}

std::string Graph::ToDotFormatWithoutSubgraphs() const {
  std::stringstream ss;
  ss << (is_directed_ ? "digraph" : "graph") << " g {\n";

  for (const Node& node : nodes_) {
    ss << "\t";
    WriteNodeForDot(ss, node);
    ss << "\n";
  }

  for (const Edge& edge : edges_) {
    ss << "\t";
    WriteEdgeForDot(ss, edge, is_directed_);
    ss << "\n";
  }

  ss << "}";
  return ss.str();
}

std::string Graph::ToDotFormatWithSubgraphs() const {
  std::stringstream ss;
  ss << (is_directed_ ? "digraph" : "graph") << " g {\n";

  for (subgraph_num_t subgraph : Subgraphs()) {
    ss << "\tsubgraph cluster_sg";
    WriteEscapedNumberForDot(ss, subgraph);
    ss << " {\n";
    ss << "\t\tstyle=filled;\n";
    ss << "\t\tcolor=lightgrey;\n";
    for (const Node& node : nodes_) {
      if (node.subgraph() != subgraph) {
        continue;
      }
      ss << "\t\t";
      WriteNodeForDot(ss, node);
      ss << "\n";
    }
    ss << "\t}\n";
  }
  for (const Edge& edge : edges_) {
    ss << "\t";
    WriteEdgeForDot(ss, edge, is_directed_);
    ss << "\n";
  }

  ss << "}";
  return ss.str();
}

}  // namespace common::graph
