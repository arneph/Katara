//
//  prog.h
//  Katara
//
//  Created by Arne Philipeit on 12/25/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_program_h
#define ir_program_h

#include <memory>
#include <string>
#include <vector>

#include "src/common/graph/graph.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/types.h"

namespace ir {

class Program : public Object {
 public:
  Program() : func_count_(0) {}

  constexpr Object::Kind object_kind() const final { return Object::Kind::kProgram; }

  const std::vector<std::unique_ptr<Func>>& funcs() const { return funcs_; }

  Func* entry_func() const { return GetFunc(entry_func_num_); }
  func_num_t entry_func_num() const { return entry_func_num_; }
  void set_entry_func_num(func_num_t entry_func_num) { entry_func_num_ = entry_func_num; }

  bool HasFunc(func_num_t fnum) const { return GetFunc(fnum) != nullptr; }
  Func* GetFunc(func_num_t fnum) const;
  Func* AddFunc(func_num_t fnum = kNoFuncNum);
  void RemoveFunc(func_num_t fnum);

  const TypeTable& type_table() const { return type_table_; }
  TypeTable& type_table() { return type_table_; }

  void WriteRefString(std::ostream&) const override {}

  bool operator==(const Program& that) const;

 private:
  int64_t func_count_;
  std::vector<std::unique_ptr<Func>> funcs_;

  func_num_t entry_func_num_ = kNoFuncNum;

  TypeTable type_table_;
};

constexpr bool IsEqual(const Program* program_a, const Program* program_b) {
  if (program_a == program_b) return true;
  if (program_a == nullptr || program_b == nullptr) return false;
  return *program_a == *program_b;
}

}  // namespace ir

#endif /* ir_program_h */
