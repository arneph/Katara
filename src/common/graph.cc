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

namespace common {
namespace {

void WriteEscapedForDot(std::stringstream& ss, std::string_view unescaped_string,
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

}  // namespace

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
  std::stringstream ss;
  ss << (is_directed_ ? "digraph" : "graph") << " g {\n";

  for (Node node : nodes_) {
    ss << "\tn" << node.number() << " [";
    ss << "label = \"";
    ss << node.title() << "\\l";
    if (!node.text().empty()) {
      WriteEscapedForDot(ss, node.text());
      ss << "\\l";
    }
    ss << "\", ";
    ss << "fillcolor = \"" << ToDotString(node.color()) << "\" style = \"filled\"";
    ss << ", shape = box, labeljust = l";
    ss << "];\n";
  }

  for (Edge edge : edges_) {
    ss << "\tn" << edge.source_number();
    ss << (is_directed_ ? "->" : "--");
    ss << "n" << edge.target_number() << "\n";
  }

  ss << "}";
  return ss.str();
}

}  // namespace common
