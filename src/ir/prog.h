//
//  prog.h
//  Katara
//
//  Created by Arne Philipeit on 12/25/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_prog_h
#define ir_prog_h

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ir/func.h"
#include "vcg/graph.h"

namespace ir {

class Prog {
 public:
  Prog();
  ~Prog();

  const std::unordered_set<Func*>& funcs() const;

  Func* entry_func() const;
  void set_entry_func(Func* func);

  bool HasFunc(int64_t fnum) const;
  Func* GetFunc(int64_t fnum) const;
  Func* AddFunc(int64_t fnum = -1);
  void RemoveFunc(int64_t fnum);
  void RemoveFunc(Func* func);

  std::string ToString() const;

 private:
  int64_t func_count_;
  std::unordered_set<Func*> funcs_;
  std::unordered_map<int64_t, Func*> func_lookup_;

  Func* entry_func_;
};

}  // namespace ir

#endif /* ir_prog_h */
