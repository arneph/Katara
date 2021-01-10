//
//  node.h
//  Katara
//
//  Created by Arne Philipeit on 1/5/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef vcg_node_h
#define vcg_node_h

#include <string>

namespace vcg {

typedef enum : int64_t {
  kWhite = 0,
  kRed = 1,
  kYellow = 2,
  kGreen = 3,
  kBlue = 4,
  kTurquoise = 5,
  kMagenta = 6
} Color;

std::string to_string(Color color);

class Node {
 public:
  Node(int64_t number, std::string title, std::string text = "", Color color = kWhite);
  ~Node();

  int64_t number() const;
  std::string title() const;
  std::string text() const;
  Color color() const;

 private:
  const int64_t number_;
  const std::string title_;
  const std::string text_;
  Color color_;
};

}  // namespace vcg

#endif /* vcg_node_h */
