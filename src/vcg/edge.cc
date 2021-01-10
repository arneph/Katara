//
//  edge.cc
//  Katara
//
//  Created by Arne Philipeit on 1/5/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "edge.h"

namespace vcg {

Edge::Edge(int64_t source_number, int64_t target_number, bool is_directed)
    : source_number_(source_number), target_number_(target_number), is_directed_(is_directed) {}
Edge::~Edge() {}

int64_t Edge::source_number() const { return source_number_; }

int64_t Edge::target_number() const { return target_number_; }

bool Edge::is_directed() const { return is_directed_; }

}  // namespace vcg
