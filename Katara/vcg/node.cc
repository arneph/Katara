//
//  node.cc
//  Katara
//
//  Created by Arne Philipeit on 1/5/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "node.h"

namespace vcg {

std::string to_string(Color color) {
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

Node::Node(int64_t number,
           std::string title,
           std::string text,
           Color color)
    : number_(number), title_(title), text_(text), color_(color) {}
Node::~Node() {}

int64_t Node::number() const {
    return number_;
}

std::string Node::title() const {
    return title_;
}

std::string Node::text() const {
    return text_;
}

Color Node::color() const {
    return color_;
}

}
