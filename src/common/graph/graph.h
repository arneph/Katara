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
#include <unordered_set>
#include <vector>

namespace common::graph {

typedef int64_t node_num_t;
typedef int64_t subgraph_num_t;
const subgraph_num_t kDefaultSubgraph = 0;

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
  node_num_t number() const { return number_; }
  std::string title() const { return title_; }
  std::string text() const { return text_; }
  subgraph_num_t subgraph() const { return subgraph_; }
  Color color() const { return color_; }

 private:
  Node(node_num_t number, std::string title) : number_(number), title_(title) {}

  node_num_t number_;
  std::string title_;
  std::string text_;
  subgraph_num_t subgraph_ = kDefaultSubgraph;
  Color color_ = kWhite;

  friend class NodeBuilder;
};

class NodeBuilder {
 public:
  NodeBuilder(node_num_t number, std::string title) : node_(number, title) {}

  NodeBuilder& SetText(std::string text);
  NodeBuilder& SetSubgraph(subgraph_num_t subgraph);
  NodeBuilder& SetColor(Color color);

  Node Build() { return node_; }

 private:
  Node node_;
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

  bool UsesSubgraphs() const;
  std::unordered_set<subgraph_num_t> Subgraphs() const;

  std::string ToVCGFormat() const;
  std::string ToDotFormat() const;

 private:
  std::string ToDotFormatWithoutSubgraphs() const;
  std::string ToDotFormatWithSubgraphs() const;

  std::vector<Node> nodes_;
  std::vector<Edge> edges_;
  bool is_directed_;
};

}  // namespace common::graph

#endif /* common_graph_h */
