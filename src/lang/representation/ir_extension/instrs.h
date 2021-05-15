//
//  instrs.h
//  Katara
//
//  Created by Arne Philipeit on 5/13/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_ext_instrs_h
#define lang_ir_ext_instrs_h

#include <memory>
#include <string>
#include <vector>

#include "ir/representation/instrs.h"

namespace lang {
namespace ir_ext {

class StringConcatInstr : public ir::Computation {
 public:
  StringConcatInstr(std::shared_ptr<ir::Computed> result,
                    std::vector<std::shared_ptr<ir::Value>> operands)
      : ir::Computation(result), operands_(operands) {}

  const std::vector<std::shared_ptr<ir::Value>>& operands() const { return operands_; }

  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override { return operands_; }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangStringConcat; }
  std::string ToString() const override;

 private:
  std::vector<std::shared_ptr<ir::Value>> operands_;
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_instrs_h */
