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

namespace common {

typedef int64_t node_num_t;

typedef enum : int64_t {
  kWhite = 0,
  kRed = 1,
  kYellow = 2,
  kGreen = 3,
  kBlue = 4,
  kTurquoise = 5,
  kMagenta = 6
} Color;

class Node {
 public:
  Node(node_num_t number, std::string title, std::string text = "", Color color = kWhite)
      : number_(number), title_(title), text_(text), color_(color) {}

  node_num_t number() const { return number_; }
  std::string title() const { return title_; }
  std::string text() const { return text_; }
  Color color() const { return color_; }

 private:
  node_num_t number_;
  std::string title_;
  std::string text_;
  Color color_;
};

class Edge {
 public:
  Edge(node_num_t source_number, node_num_t target_number)
      : source_number_(source_number), target_number_(target_number) {}

  node_num_t source_number() const { return source_number_; }
  node_num_t target_number() const { return target_number_; }

 private:
  node_num_t source_number_;
  node_num_t target_number_;
};

class Graph {
 public:
  Graph(bool is_directed) : is_directed_(is_directed) {}

  std::vector<Node>& nodes() { return nodes_; }
  std::vector<Edge>& edges() { return edges_; }

  std::string ToVCGFormat() const;
  std::string ToDotFormat() const;

 private:
  std::vector<Node> nodes_;
  std::vector<Edge> edges_;
  bool is_directed_;
};

}  // namespace common

#endif /* common_graph_h */
