//
//  code_generator.h
//  Katara
//
//  Created by Arne Philipeit on 4/15/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_testing_code_generator_h
#define lang_testing_code_generator_h

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace lang {
namespace testing {

class Context {
 public:
  Context(int max_depth) : max_depth_(max_depth) {}
  
  int max_depth() const { return max_depth_; }

  Context SubContextWithIncreasedDepth() const;
private:
  int max_depth_;
};

class Generator {
 public:
  Generator() = default;
  virtual ~Generator() = default;

  virtual int NumOptions(Context& ctx) const = 0;
  virtual void GenerateOption(int index, Context& ctx, std::stringstream& code) const = 0;
};

class AtomGenerator final : public Generator {
 public:
  AtomGenerator(std::vector<std::string>& atoms) : atoms_(atoms) {}
  ~AtomGenerator() override = default;

  int NumOptions(Context&) const override;
  void GenerateOption(int index, Context&, std::stringstream& code) const override;

 private:
  std::vector<std::string>& atoms_;
};

class CombinationGenerator final : public Generator {
 public:
  CombinationGenerator(std::vector<Generator*>& item_generators)
      : item_generators_(item_generators) {}
  ~CombinationGenerator() override = default;

  int NumOptions(Context& ctx) const override;
  void GenerateOption(int index, Context& ctx, std::stringstream& code) const override;

 private:
  std::vector<Generator*>& item_generators_;
};

class SequenceGenerator final : public Generator {
 public:
  SequenceGenerator(Generator* items_generator, int min_length, int max_length)
      : items_generator_(items_generator), min_length_(min_length), max_length_(max_length) {}
  ~SequenceGenerator() override = default;

  int NumOptions(Context& ctx) const override;
  void GenerateOption(int index, Context& ctx, std::stringstream& code) const override;

 private:
  Generator* items_generator_;
  int min_length_;
  int max_length_;
};



}  // namespace testing
}  // namespace lang

#endif /* lang_testing_code_generator_h */
