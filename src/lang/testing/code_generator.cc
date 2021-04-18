//
//  code_generator.cc
//  Katara
//
//  Created by Arne Philipeit on 4/15/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "code_generator.h"

namespace lang {
namespace testing {

Context Context::SubContextWithIncreasedDepth() const {
  return Context(max_depth_ - 1);
}

int AtomGenerator::NumOptions(Context&) const { return static_cast<int>(atoms_.size()); }
void AtomGenerator::GenerateOption(int index, Context&, std::stringstream& code) const {
  code << atoms_.at(index);
}

int CombinationGenerator::NumOptions(Context& ctx) const {
  if (ctx.max_depth() == 0){
    return 0;
  }
  Context sub_ctx = ctx.SubContextWithIncreasedDepth();
  int num_options = 1;
  for (auto item_generator : item_generators_) {
    num_options *= item_generator->NumOptions(sub_ctx);
    if (num_options == 0) {
      return 0;
    }
  }
  return num_options;
}

void CombinationGenerator::GenerateOption(int index, Context& ctx, std::stringstream& code) const {
  Context sub_ctx = ctx.SubContextWithIncreasedDepth();
  for (auto item_generator : item_generators_) {
    int sub_num_options = item_generator->NumOptions(sub_ctx);
    int sub_index = index % sub_num_options;
    item_generator->GenerateOption(sub_index, sub_ctx, code);
    index /= sub_num_options;
  }
}

int SequenceGenerator::NumOptions(Context& ctx) const {
  if (ctx.max_depth() == 0){
    return 0;
  }
  Context sub_ctx = ctx.SubContextWithIncreasedDepth();
  int sub_num_options = items_generator_->NumOptions(sub_ctx);
  int num_options = 0;
  for (int length = min_length_; length <= max_length_; length++) {
    num_options += std::pow(sub_num_options, length);
  }
  return num_options;
}

void SequenceGenerator::GenerateOption(int index, Context& ctx, std::stringstream& code) const {
  Context sub_ctx = ctx.SubContextWithIncreasedDepth();
  int sub_num_options = items_generator_->NumOptions(sub_ctx);
  for (int length = min_length_; length <= max_length_; length++) {
    int options_with_length = std::pow(sub_num_options, length);
    if (index >= options_with_length) {
      index -= options_with_length;
      continue;
    }
    for (int i = 0; i < length; i++) {
      int sub_index = index % sub_num_options;
      items_generator_->GenerateOption(sub_index, sub_ctx, code);
      index /= sub_num_options;
    }
    return;
  }
  throw "unexpected index";
}



}  // namespace testing
}  // namespace lang
