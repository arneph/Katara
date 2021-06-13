//
//  objects.h
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_objects_h
#define lang_types_objects_h

#include <string>

#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/constants/constants.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/types.h"

namespace lang {
namespace types {

class Scope;
class Package;

enum class ObjectKind {
  kTypeName,
  kConstant,
  kVariable,
  kFunc,
  kTypedObjectStart = kTypeName,
  kTypedObjectEnd = kFunc,

  kNil,
  kLabel,
  kBuiltin,
  kPackageName,
};

class Object {
 public:
  virtual ~Object() {}

  Scope* parent() const { return parent_; }
  Package* package() const { return package_; }
  pos::pos_t position() const { return position_; }
  std::string name() const { return name_; }

  bool is_typed() const;

  virtual ObjectKind object_kind() const = 0;
  virtual std::string ToString() const = 0;

 protected:
  Object(Scope* parent, Package* package, pos::pos_t position, std::string name)
      : parent_(parent), package_(package), position_(position), name_(name) {}

 private:
  Scope* parent_;
  Package* package_;
  pos::pos_t position_;
  std::string name_;
};

class TypedObject : public Object {
 public:
  virtual ~TypedObject() {}

  Type* type() const { return type_; }

 protected:
  TypedObject(Scope* parent, Package* package, pos::pos_t position, std::string name)
      : Object(parent, package, position, name), type_(nullptr) {}

 private:
  Type* type_;

  friend class Nil;
  friend class InfoBuilder;
};

class TypeName final : public TypedObject {
 public:
  ObjectKind object_kind() const override { return ObjectKind::kTypeName; }
  std::string ToString() const override { return "type " + name(); }

 private:
  TypeName(Scope* parent, Package* package, pos::pos_t position, std::string name)
      : TypedObject(parent, package, position, name) {}

  friend class InfoBuilder;
};

class Constant final : public TypedObject {
 public:
  constants::Value value() const { return value_; }

  ObjectKind object_kind() const override { return ObjectKind::kConstant; }
  std::string ToString() const override {
    return "const " + name() + " " + type()->ToString(StringRep::kShort) + " = " +
           value().ToString();
  }

 private:
  Constant(Scope* parent, Package* package, pos::pos_t position, std::string name)
      : TypedObject(parent, package, position, name), value_(false) {}

  constants::Value value_;

  friend class InfoBuilder;
};

class Variable final : public TypedObject {
 public:
  bool is_embedded() const { return is_embedded_; }
  bool is_field() const { return is_field_; }

  ObjectKind object_kind() const override { return ObjectKind::kVariable; }
  std::string ToString() const override;

 private:
  Variable(Scope* parent, Package* package, pos::pos_t position, std::string name, bool is_embdeded,
           bool is_field)
      : TypedObject(parent, package, position, name),
        is_embedded_(is_embdeded),
        is_field_(is_field) {}

  bool is_embedded_;
  bool is_field_;

  friend class InfoBuilder;
};

class Func final : public TypedObject {
 public:
  ObjectKind object_kind() const override { return ObjectKind::kFunc; }
  std::string ToString() const override;

 private:
  Func(Scope* parent, Package* package, pos::pos_t position, std::string name)
      : TypedObject(parent, package, position, name) {}

  friend class InfoBuilder;
};

class Nil final : public Object {
 public:
  ObjectKind object_kind() const override { return ObjectKind::kNil; }
  std::string ToString() const override { return "nil"; }

 private:
  Nil(Scope* universe) : Object(universe, nullptr, pos::kNoPos, "nil") {}

  friend class InfoBuilder;
};

class Label final : public Object {
 public:
  ObjectKind object_kind() const override { return ObjectKind::kLabel; }
  std::string ToString() const override { return name() + " (label)"; }

 private:
  Label(Scope* parent, Package* package, pos::pos_t position, std::string name)
      : Object(parent, package, position, name) {}

  friend class InfoBuilder;
};

class Builtin final : public Object {
 public:
  enum class Kind {
    kLen,
    kMake,
    kNew,
  };

  Kind kind() const { return kind_; }

  ObjectKind object_kind() const override { return ObjectKind::kBuiltin; }
  std::string ToString() const override;

 private:
  Builtin(Scope* universe, std::string name, Kind kind)
      : Object(universe, nullptr, pos::kNoPos, name), kind_(kind) {}

  Kind kind_;

  friend class InfoBuilder;
};

class PackageName final : public Object {
 public:
  Package* referenced_package() const { return referenced_package_; }

  ObjectKind object_kind() const override { return ObjectKind::kPackageName; }
  std::string ToString() const override { return name(); }

 private:
  PackageName(Scope* parent, Package* package, pos::pos_t position, std::string name,
              Package* referenced_package)
      : Object(parent, package, position, name), referenced_package_(referenced_package) {}

  Package* referenced_package_;

  friend class InfoBuilder;
};

}  // namespace types
}  // namespace lang

#endif /* lang_types_objects_h */
