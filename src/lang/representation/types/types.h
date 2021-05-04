//
//  types.h
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_types_h
#define lang_types_types_h

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ir/representation/types.h"

namespace lang {
namespace types {

class Variable;
class Func;

enum class StringRep {
  kShort,
  kExpanded,
};

class Type : public ir::Type {
 public:
  virtual ~Type() {}

  bool is_wrapper() const;
  bool is_container() const;

  virtual std::string ToString(StringRep rep) const = 0;

  std::string ToString() const override { return ToString(StringRep::kShort); }
};

class Basic final : public Type {
 public:
  typedef enum : int8_t {
    kBool,
    kInt,
    kInt8,
    kInt16,
    kInt32,
    kInt64,
    kUint,
    kUint8,
    kUint16,
    kUint32,
    kUint64,
    kString,

    kUntypedBool,
    kUntypedInt,
    kUntypedRune,
    kUntypedString,
    kUntypedNil,

    kByte = kUint8,
    kRune = kInt32,
  } Kind;
  typedef enum : int8_t {
    kIsBoolean = 1 << 0,
    kIsInteger = 1 << 1,
    kIsUnsigned = 1 << 2,
    kIsString = 1 << 3,
    kIsUntyped = 1 << 4,

    kIsOrdered = kIsInteger | kIsString,
    kIsNumeric = kIsInteger,
    kIsConstant = kIsBoolean | kIsNumeric | kIsString,
  } Info;

  Kind kind() const { return kind_; }
  Info info() const { return info_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangBasic; }
  std::string ToString(StringRep rep) const override;

 private:
  Basic(Kind kind, Info info) : kind_(kind), info_(info) {}

  Kind kind_;
  Info info_;

  friend class InfoBuilder;
};

class Wrapper : public Type {
 public:
  virtual ~Wrapper() {}
  virtual Type* element_type() const = 0;
};

class Pointer final : public Wrapper {
 public:
  enum class Kind : int8_t {
    kStrong,
    kWeak,
  };

  Kind kind() const { return kind_; }
  Type* element_type() const override { return element_type_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangPointer; }
  std::string ToString(StringRep rep) const override;

 private:
  Pointer(Kind kind, Type* element_type) : kind_(kind), element_type_(element_type) {}

  Kind kind_;
  Type* element_type_;

  friend class InfoBuilder;
};

class Container : public Wrapper {
 public:
  virtual ~Container() {}
};

class Array final : public Container {
 public:
  Type* element_type() const override { return element_type_; }
  uint64_t length() const { return length_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangArray; }
  std::string ToString(StringRep rep) const override;

 private:
  Array(Type* element_type, uint64_t length) : element_type_(element_type), length_(length) {}

  Type* element_type_;
  uint64_t length_;

  friend class InfoBuilder;
};

class Slice final : public Container {
 public:
  Type* element_type() const override { return element_type_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangSlice; }
  std::string ToString(StringRep rep) const override;

 private:
  Slice(Type* element_type) : element_type_(element_type) {}

  Type* element_type_;

  friend class InfoBuilder;
};

class Interface;

class TypeParameter final : public Type {
 public:
  std::string name() const { return name_; }
  TypeParameter* instantiated_type_parameter() const { return instantiated_type_parameter_; }
  Interface* interface() const { return interface_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangTypeParameter; }
  std::string ToString(StringRep rep) const override;

 private:
  TypeParameter(std::string name)
      : name_(name), instantiated_type_parameter_(nullptr), interface_(nullptr) {}

  std::string name_;
  TypeParameter* instantiated_type_parameter_;
  Interface* interface_;

  friend class InfoBuilder;
};

class NamedType final : public Type {
 public:
  bool is_alias() const { return is_alias_; }
  std::string name() const { return name_; }
  Type* underlying() { return underlying_; }
  const std::vector<TypeParameter*>& type_parameters() const { return type_parameters_; }
  const std::unordered_map<std::string, Func*>& methods() const { return methods_; }

  Type* InstanceForTypeArgs(const std::vector<Type*>& type_args) const;

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangNamedType; }
  std::string ToString(StringRep rep) const override;

 private:
  NamedType(bool is_alias, std::string name)
      : is_alias_(is_alias), name_(name), underlying_(nullptr) {}

  void SetInstanceForTypeArgs(const std::vector<Type*>& type_args, Type* instance);

  bool is_alias_;
  std::string name_;
  Type* underlying_;
  std::vector<TypeParameter*> type_parameters_;
  std::unordered_map<std::string, Func*> methods_;
  std::vector<std::pair<std::vector<Type*>, Type*>> instances_;

  friend class InfoBuilder;
};

class TypeInstance final : public Type {
 public:
  NamedType* instantiated_type() const { return instantiated_type_; }
  const std::vector<Type*>& type_args() const { return type_args_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangTypeInstance; }
  std::string ToString(StringRep rep) const override;

 private:
  TypeInstance(NamedType* instantiated_type, std::vector<Type*> type_args)
      : instantiated_type_(instantiated_type), type_args_(type_args) {}

  NamedType* instantiated_type_;
  std::vector<Type*> type_args_;

  friend class InfoBuilder;
};

class Tuple final : public Type {
 public:
  const std::vector<Variable*>& variables() const { return variables_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangTuple; }
  std::string ToString(StringRep rep) const override;

 private:
  Tuple(std::vector<Variable*> variables) : variables_(variables) {}

  std::vector<Variable*> variables_;

  friend class InfoBuilder;
};

class Signature final : public Type {
 public:
  bool has_expr_receiver() const { return expr_receiver_ != nullptr; }
  Variable* expr_receiver() const { return expr_receiver_; }
  bool has_type_receiver() const { return type_receiver_ != nullptr; }
  Type* type_receiver() const { return type_receiver_; }
  const std::vector<TypeParameter*>& type_parameters() const { return type_parameters_; }
  Tuple* parameters() const { return parameters_; }
  Tuple* results() const { return results_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangSignature; }
  std::string ToString(StringRep rep) const override;

 private:
  Signature(Tuple* parameters, Tuple* results)
      : expr_receiver_(nullptr),
        type_receiver_(nullptr),
        parameters_(parameters),
        results_(results) {}

  Signature(std::vector<TypeParameter*> type_parameters, Tuple* parameters, Tuple* results)
      : expr_receiver_(nullptr),
        type_receiver_(nullptr),
        type_parameters_(type_parameters),
        parameters_(parameters),
        results_(results) {}

  Signature(Variable* expr_receiver, Tuple* parameters, Tuple* results)
      : expr_receiver_(expr_receiver),
        type_receiver_(nullptr),
        parameters_(parameters),
        results_(results) {}

  Signature(Type* type_receiver, Tuple* parameters, Tuple* results)
      : expr_receiver_(nullptr),
        type_receiver_(type_receiver),
        parameters_(parameters),
        results_(results) {}

  Variable* expr_receiver_;
  Type* type_receiver_;
  std::vector<TypeParameter*> type_parameters_;
  Tuple* parameters_;
  Tuple* results_;

  friend class InfoBuilder;
};

class Struct final : public Type {
 public:
  const std::vector<Variable*>& fields() const { return fields_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangStruct; }
  std::string ToString(StringRep rep) const override;

 private:
  Struct(std::vector<Variable*> fields) : fields_(fields) {}

  std::vector<Variable*> fields_;

  friend class InfoBuilder;
};

class Interface final : public Type {
 public:
  bool is_empty() const { return embedded_interfaces_.empty() && methods_.empty(); }
  const std::vector<NamedType*>& embedded_interfaces() const { return embedded_interfaces_; }
  const std::vector<Func*>& methods() const { return methods_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangInterface; }
  std::string ToString(StringRep rep) const override;

 private:
  Interface() {}

  std::vector<NamedType*> embedded_interfaces_;
  std::vector<Func*> methods_;

  friend class InfoBuilder;
};

}  // namespace types
}  // namespace lang

#endif /* lang_types_types_h */
