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

class PanicInstr : public ir::Instr {
 public:
  PanicInstr(std::shared_ptr<ir::Value> reason) : reason_(reason) {}

  std::shared_ptr<ir::Value> reason() const { return reason_; }

  std::vector<std::shared_ptr<ir::Computed>> DefinedValues() const override { return {}; };
  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override { return {}; };

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangPanic; };

  std::string OperationString() const override { return "panic"; }
  void WriteRefString(std::ostream& os) const override { os << "panic \"" << reason_ << "\""; }

  bool operator==(const ir::Instr& that) const override;

 private:
  std::shared_ptr<ir::Value> reason_;
};

class MakeSharedPointerInstr : public ir::Computation {
 public:
  // Operand indices in defined values:
  static constexpr std::size_t kResultIndex = 0;
  // Operand indices in used values:
  static constexpr std::size_t kSizeIndex = 0;

  explicit MakeSharedPointerInstr(std::shared_ptr<ir::Computed> result,
                                  std::shared_ptr<ir::Value> size)
      : ir::Computation(result), size_(size) {}

  const ir::Type* element_type() const { return pointer_type()->element(); }
  const ir_ext::SharedPointer* pointer_type() const;

  std::shared_ptr<ir::Value> size() const { return size_; }

  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override { return {size_}; }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangMakeSharedPointer; }
  std::string OperationString() const override { return "make_shared"; }

  bool operator==(const ir::Instr& that) const override;

 private:
  std::shared_ptr<ir::Value> size_;
};

class CopySharedPointerInstr : public ir::Computation {
 public:
  // Operand indices in defined values:
  static constexpr std::size_t kResultIndex = 0;
  // Operand indices in used values:
  static constexpr std::size_t kCopiedSharedPointerIndex = 0;
  static constexpr std::size_t kPointerOffsetIndex = 1;

  CopySharedPointerInstr(std::shared_ptr<ir::Computed> result,
                         std::shared_ptr<ir::Computed> copied_shared_pointer,
                         std::shared_ptr<ir::Value> pointer_offset)
      : ir::Computation(result),
        copied_shared_pointer_(copied_shared_pointer),
        pointer_offset_(pointer_offset) {}

  const ir::Type* element_type() const { return copied_pointer_type()->element(); }
  const ir_ext::SharedPointer* copied_pointer_type() const;
  const ir_ext::SharedPointer* copy_pointer_type() const;

  std::shared_ptr<ir::Computed> copied_shared_pointer() const { return copied_shared_pointer_; }
  std::shared_ptr<ir::Value> underlying_pointer_offset() const { return pointer_offset_; }

  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override {
    return {copied_shared_pointer_, pointer_offset_};
  }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangCopySharedPointer; }
  std::string OperationString() const override { return "copy_shared"; }

  bool operator==(const ir::Instr& that) const override;

 private:
  std::shared_ptr<ir::Computed> copied_shared_pointer_;
  std::shared_ptr<ir::Value> pointer_offset_;
};

class DeleteSharedPointerInstr : public ir::Instr {
 public:
  // Operand indices in used values:
  static constexpr std::size_t kDeletedSharedPointerIndex = 0;

  explicit DeleteSharedPointerInstr(std::shared_ptr<ir::Computed> deleted_shared_pointer)
      : deleted_shared_pointer_(deleted_shared_pointer) {}

  const ir::Type* element_type() const { return pointer_type()->element(); }
  const ir_ext::SharedPointer* pointer_type() const;

  std::shared_ptr<ir::Computed> deleted_shared_pointer() const { return deleted_shared_pointer_; }

  std::vector<std::shared_ptr<ir::Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override {
    return {deleted_shared_pointer_};
  }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangDeleteSharedPointer; }
  std::string OperationString() const override { return "delete_shared"; }

  bool operator==(const ir::Instr& that) const override;

 private:
  std::shared_ptr<ir::Computed> deleted_shared_pointer_;
};

class MakeUniquePointerInstr : public ir::Computation {
 public:
  // Operand indices in defined values:
  static constexpr std::size_t kResultIndex = 0;
  // Operand indices in used values:
  static constexpr std::size_t kSizeIndex = 0;

  explicit MakeUniquePointerInstr(std::shared_ptr<ir::Computed> result,
                                  std::shared_ptr<ir::Value> size)
      : ir::Computation(result), size_(size) {}

  const ir::Type* element_type() const { return pointer_type()->element(); }
  const ir_ext::UniquePointer* pointer_type() const;

  std::shared_ptr<ir::Value> size() const { return size_; }

  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override { return {size_}; }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangMakeUniquePointer; }
  std::string OperationString() const override { return "make_unique"; }

  bool operator==(const ir::Instr& that) const override;

 private:
  std::shared_ptr<ir::Value> size_;
};

class DeleteUniquePointerInstr : public ir::Instr {
 public:
  // Operand indices in used values:
  static constexpr std::size_t kDeletedUniquePointerIndex = 0;

  explicit DeleteUniquePointerInstr(std::shared_ptr<ir::Computed> deleted_unique_pointer)
      : deleted_unique_pointer_(deleted_unique_pointer) {}

  const ir::Type* element_type() const { return pointer_type()->element(); }
  const ir_ext::UniquePointer* pointer_type() const;

  std::shared_ptr<ir::Computed> deleted_unique_pointer() const { return deleted_unique_pointer_; }

  std::vector<std::shared_ptr<ir::Computed>> DefinedValues() const override { return {}; }
  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override {
    return {deleted_unique_pointer_};
  }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangDeleteUniquePointer; }
  std::string OperationString() const override { return "delete_unique"; }

  bool operator==(const ir::Instr& that) const override;

 private:
  std::shared_ptr<ir::Computed> deleted_unique_pointer_;
};

class StringIndexInstr : public ir::Computation {
 public:
  // Operand indices in defined values:
  static constexpr std::size_t kResultIndex = 0;
  // Operand indices in used values:
  static constexpr std::size_t kStringOperandIndex = 0;
  static constexpr std::size_t kIndexOperandIndex = 1;

  StringIndexInstr(std::shared_ptr<ir::Computed> result, std::shared_ptr<ir::Value> string_operand,
                   std::shared_ptr<ir::Value> index_operand)
      : ir::Computation(result), string_operand_(string_operand), index_operand_(index_operand) {}

  std::shared_ptr<ir::Value> string_operand() const { return string_operand_; }
  std::shared_ptr<ir::Value> index_operand() const { return index_operand_; }

  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override {
    return {string_operand_, index_operand_};
  }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangStringIndex; }
  std::string OperationString() const override { return "str_index"; }

  bool operator==(const ir::Instr& that) const override;

 private:
  std::shared_ptr<ir::Value> string_operand_;
  std::shared_ptr<ir::Value> index_operand_;
};

class StringConcatInstr : public ir::Computation {
 public:
  // Operand indices in defined values:
  static constexpr std::size_t kResultIndex = 0;

  StringConcatInstr(std::shared_ptr<ir::Computed> result,
                    std::vector<std::shared_ptr<ir::Value>> operands)
      : ir::Computation(result), operands_(operands) {}

  const std::vector<std::shared_ptr<ir::Value>>& operands() const { return operands_; }

  std::vector<std::shared_ptr<ir::Value>> UsedValues() const override { return operands_; }

  ir::InstrKind instr_kind() const override { return ir::InstrKind::kLangStringConcat; }
  std::string OperationString() const override { return "str_cat"; }

  bool operator==(const ir::Instr& that) const override;

 private:
  std::vector<std::shared_ptr<ir::Value>> operands_;
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_instrs_h */
