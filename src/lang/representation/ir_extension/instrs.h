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

#include "src/ir/representation/instrs.h"
#include "src/lang/representation/ir_extension/types.h"

namespace lang {
namespace ir_ext {

class RefCountMallocInstr : public ir::Computation {
 public:
  RefCountMallocInstr(std::shared_ptr<ir::Computed> result, const ir::Type* type)
      : ir::Computation(result), type_(type) {}

  const ir::Type* type() const { return type_; }

  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override { return {}; }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangRefCountMalloc; }
  std::string ToString() const override {
    return result()->ToStringWithType() + " = rcmalloc:" + type_->ToString();
  }

 private:
  const ir::Type* type_;
};

enum class RefCountUpdate {
  kInc,
  kDec,
};

extern bool IsRefCountUpdateString(std::string op_str);
extern RefCountUpdate ToRefCountUpdate(std::string op_str);
extern std::string ToString(RefCountUpdate op);

class RefCountUpdateInstr : public ir::Instr {
 public:
  RefCountUpdateInstr(RefCountUpdate op, std::shared_ptr<ir::Value> pointer)
      : op_(op), pointer_(pointer) {}

  RefCountUpdate operation() const { return op_; }
  std::shared_ptr<ir::Value> pointer() const { return pointer_; }

  std::vector<std::shared_ptr<ir::Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override { return {pointer_}; }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangRefCountUpdate; }
  std::string ToString() const override {
    return ir_ext::ToString(op_) + " " + pointer_->ToString();
  }

 private:
  RefCountUpdate op_;
  std::shared_ptr<ir::Value> pointer_;
};

class StringIndexInstr : public ir::Computation {
 public:
  StringIndexInstr(std::shared_ptr<ir::Computed> result, std::shared_ptr<ir::Value> string_operand,
                   std::shared_ptr<ir::Value> index_operand)
      : ir::Computation(result), string_operand_(string_operand), index_operand_(index_operand) {}

  std::shared_ptr<ir::Value> string_operand() const { return string_operand_; }
  std::shared_ptr<ir::Value> index_operand() const { return index_operand_; }

  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override {
    return {string_operand_, index_operand_};
  }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangStringIndex; }
  std::string ToString() const override;

 private:
  std::shared_ptr<ir::Value> string_operand_;
  std::shared_ptr<ir::Value> index_operand_;
};

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
