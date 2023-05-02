//
//  info_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "info_builder.h"

#include <algorithm>

#include "src/common/atomics/atomics.h"
#include "src/common/logging/logging.h"

namespace lang {
namespace types {

using ::common::atomics::Int;
using ::common::logging::fail;
using ::common::positions::kNoPos;
using ::common::positions::pos_t;

InfoBuilder::InfoBuilder(Info* info) : info_(info) {}

Info* InfoBuilder::info() const { return info_; }

void InfoBuilder::CreateUniverse() {
  if (info_->universe_ != nullptr) {
    return;
  }

  std::unique_ptr<Scope> universe(new Scope());
  universe->parent_ = nullptr;

  info_->universe_ = universe.get();
  info_->scope_unique_ptrs_.push_back(std::move(universe));

  CreatePredeclaredTypes();
  CreatePredeclaredConstants();
  CreatePredeclaredNil();
  CreatePredeclaredFuncs();
}

void InfoBuilder::CreatePredeclaredTypes() {
  typedef struct {
    types::Basic::Kind kind;
    types::Basic::Info info;
    std::string name;
  } predeclared_type_t;
  auto predeclared_types = std::vector<predeclared_type_t>({
      {types::Basic::kBool, types::Basic::kIsBoolean, "bool"},
      {types::Basic::kInt, types::Basic::kIsInteger, "int"},
      {types::Basic::kInt8, types::Basic::kIsInteger, "int8"},
      {types::Basic::kInt16, types::Basic::kIsInteger, "int16"},
      {types::Basic::kInt32, types::Basic::kIsInteger, "int32"},
      {types::Basic::kInt64, types::Basic::kIsInteger, "int64"},
      {types::Basic::kUint,
       types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned}, "uint"},
      {types::Basic::kUint8,
       types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned}, "uint8"},
      {types::Basic::kUint16,
       types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned}, "uint16"},
      {types::Basic::kUint32,
       types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned}, "uint32"},
      {types::Basic::kUint64,
       types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned}, "uint64"},
      {types::Basic::kString, types::Basic::kIsString, "string"},

      {types::Basic::kUntypedBool,
       types::Basic::Info{types::Basic::kIsBoolean | types::Basic::kIsUntyped}, "untyped bool"},
      {types::Basic::kUntypedInt,
       types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUntyped}, "untyped int"},
      {types::Basic::kUntypedRune,
       types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUntyped}, "untyped rune"},
      {types::Basic::kUntypedString,
       types::Basic::Info{types::Basic::kIsString | types::Basic::kIsUntyped}, "untyped string"},
      {types::Basic::kUntypedNil, types::Basic::kIsUntyped, "untyped nil"},

      {types::Basic::kByte,
       types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned}, "byte"},
      {types::Basic::kRune, types::Basic::kIsInteger, "rune"},
  });
  for (auto predeclared_type : predeclared_types) {
    std::unique_ptr<types::Basic> basic(new Basic(predeclared_type.kind, predeclared_type.info));

    types::Basic* basic_ptr = basic.get();
    info_->type_unique_ptrs_.push_back(std::move(basic));
    info_->basic_types_.insert({predeclared_type.kind, basic_ptr});

    auto it = std::find(predeclared_type.name.begin(), predeclared_type.name.end(), ' ');
    if (it != predeclared_type.name.end()) {
      continue;
    }

    std::unique_ptr<types::TypeName> type_name(
        new types::TypeName(info_->universe(), nullptr, kNoPos, predeclared_type.name));
    type_name->type_ = basic_ptr;

    types::TypeName* type_name_ptr = type_name.get();
    info_->object_unique_ptrs_.push_back(std::move(type_name));
    info_->universe_->named_objects_.insert({predeclared_type.name, type_name_ptr});
  }
}

void InfoBuilder::CreatePredeclaredConstants() {
  typedef struct {
    types::Basic::Kind kind;
    constants::Value value;
    std::string name;
  } predeclared_const_t;
  auto predeclared_consts = std::vector<predeclared_const_t>({
      {types::Basic::kUntypedBool, constants::Value(false), "false"},
      {types::Basic::kUntypedBool, constants::Value(true), "true"},
      {types::Basic::kUntypedInt, constants::Value(Int(int64_t{0})), "iota"},
  });
  for (auto predeclared_const : predeclared_consts) {
    std::unique_ptr<types::Constant> constant(
        new Constant(info_->universe(), nullptr, kNoPos, predeclared_const.name));
    constant->type_ = info_->basic_types_.at(predeclared_const.kind);
    constant->value_ = predeclared_const.value;

    info_->universe_->named_objects_.insert({predeclared_const.name, constant.get()});
    info_->object_unique_ptrs_.push_back(std::move(constant));
  }
}

void InfoBuilder::CreatePredeclaredNil() {
  std::unique_ptr<types::Nil> nil(new Nil(info_->universe()));
  info_->universe_->named_objects_.insert({"nil", nil.get()});
  info_->object_unique_ptrs_.push_back(std::move(nil));
}

void InfoBuilder::CreatePredeclaredFuncs() {
  typedef struct {
    types::Builtin::Kind kind;
    std::string name;
  } predeclared_builtin_t;
  auto predeclared_builtins = std::vector<predeclared_builtin_t>({
      {types::Builtin::Kind::kLen, "len"},
      {types::Builtin::Kind::kMake, "make"},
      {types::Builtin::Kind::kNew, "new"},
  });
  for (auto predeclared_builtin : predeclared_builtins) {
    std::unique_ptr<types::Builtin> builtin(
        new Builtin(info_->universe(), predeclared_builtin.name, predeclared_builtin.kind));
    info_->universe_->named_objects_.insert({predeclared_builtin.name, builtin.get()});
    info_->object_unique_ptrs_.push_back(std::move(builtin));
  }
}

Pointer* InfoBuilder::CreatePointer(Pointer::Kind kind, Type* element_type) {
  if (element_type == nullptr) {
    fail("attempted to create pointer without element type");
  }

  std::unique_ptr<Pointer> pointer(new Pointer(kind, element_type));
  Pointer* pointer_ptr = pointer.get();
  info_->type_unique_ptrs_.push_back(std::move(pointer));
  return pointer_ptr;
}

Array* InfoBuilder::CreateArray(Type* element_type, uint64_t length) {
  if (element_type == nullptr) {
    fail("attempted to create array without element type");
  }

  std::unique_ptr<Array> array(new Array(element_type, length));
  Array* array_ptr = array.get();
  info_->type_unique_ptrs_.push_back(std::move(array));
  return array_ptr;
}

Slice* InfoBuilder::CreateSlice(Type* element_type) {
  if (element_type == nullptr) {
    fail("attempted to create slice without element type");
  }

  std::unique_ptr<Slice> slice(new Slice(element_type));
  Slice* slice_ptr = slice.get();
  info_->type_unique_ptrs_.push_back(std::move(slice));
  return slice_ptr;
}

TypeParameter* InfoBuilder::CreateTypeParameter(std::string name) {
  std::unique_ptr<TypeParameter> type_parameter(new TypeParameter(name));
  TypeParameter* type_parameter_ptr = type_parameter.get();
  info_->type_unique_ptrs_.push_back(std::move(type_parameter));
  return type_parameter_ptr;
}

NamedType* InfoBuilder::CreateNamedType(bool is_alias, std::string name) {
  std::unique_ptr<NamedType> named_type(new NamedType(is_alias, name));
  NamedType* named_type_ptr = named_type.get();
  info_->type_unique_ptrs_.push_back(std::move(named_type));
  return named_type_ptr;
}

TypeInstance* InfoBuilder::CreateTypeInstance(NamedType* instantiated_type,
                                              std::vector<Type*> type_args) {
  if (instantiated_type == nullptr) {
    fail("attempted to create type instance without instantiated type");
  } else if (type_args.empty()) {
    fail("attempted to create type instance without type arguments");
  } else if (instantiated_type->type_parameters().size() != type_args.size()) {
    fail("attempted to create type instance with mismatched type arguments");
  }

  std::unique_ptr<TypeInstance> type_instance(new TypeInstance(instantiated_type, type_args));
  TypeInstance* type_instance_ptr = type_instance.get();
  info_->type_unique_ptrs_.push_back(std::move(type_instance));
  return type_instance_ptr;
}

Tuple* InfoBuilder::CreateTuple(std::vector<Variable*> variables) {
  std::unique_ptr<Tuple> tuple(new Tuple(variables));
  Tuple* tuple_ptr = tuple.get();
  info_->type_unique_ptrs_.push_back(std::move(tuple));
  return tuple_ptr;
}

Signature* InfoBuilder::CreateSignature(Tuple* parameters, Tuple* results) {
  std::unique_ptr<Signature> signature(new Signature(parameters, results));
  Signature* signature_ptr = signature.get();
  info_->type_unique_ptrs_.push_back(std::move(signature));
  return signature_ptr;
}

Signature* InfoBuilder::CreateSignature(std::vector<TypeParameter*> type_parameters,
                                        Tuple* parameters, Tuple* results) {
  std::unique_ptr<Signature> signature(new Signature(type_parameters, parameters, results));
  Signature* signature_ptr = signature.get();
  info_->type_unique_ptrs_.push_back(std::move(signature));
  return signature_ptr;
}

Signature* InfoBuilder::CreateSignature(Variable* expr_receiver, Tuple* parameters,
                                        Tuple* results) {
  std::unique_ptr<Signature> signature(new Signature(expr_receiver, parameters, results));
  Signature* signature_ptr = signature.get();
  info_->type_unique_ptrs_.push_back(std::move(signature));
  return signature_ptr;
}

Signature* InfoBuilder::CreateSignature(Type* type_receiver, Tuple* parameters, Tuple* results) {
  std::unique_ptr<Signature> signature(new Signature(type_receiver, parameters, results));
  Signature* signature_ptr = signature.get();
  info_->type_unique_ptrs_.push_back(std::move(signature));
  return signature_ptr;
}

Struct* InfoBuilder::CreateStruct(std::vector<Variable*> fields) {
  std::unique_ptr<Struct> struct_(new Struct(fields));
  Struct* struct_ptr = struct_.get();
  info_->type_unique_ptrs_.push_back(std::move(struct_));
  return struct_ptr;
}

Interface* InfoBuilder::CreateInterface() {
  std::unique_ptr<Interface> interface(new Interface());

  Interface* interface_ptr = interface.get();
  info_->type_unique_ptrs_.push_back(std::move(interface));

  return interface_ptr;
}

Signature* InfoBuilder::InstantiateFuncSignature(Signature* parameterized_signature,
                                                 TypeParamsToArgsMap& type_params_to_args) {
  if (parameterized_signature->type_parameters().empty()) {
    fail("attempted to instantiate func signature without type parameters");
  } else if (parameterized_signature->expr_receiver() != nullptr) {
    fail("attempted to instantiate func signature with expr receiver");
  }
  Tuple* parameters = parameterized_signature->parameters();
  if (parameters != nullptr) {
    parameters = static_cast<Tuple*>(InstantiateTuple(parameters, type_params_to_args));
  }
  Tuple* results = parameterized_signature->results();
  if (results != nullptr) {
    results = static_cast<Tuple*>(InstantiateTuple(results, type_params_to_args));
  }
  return CreateSignature(parameters, results);
}

Signature* InfoBuilder::InstantiateMethodSignature(Signature* parameterized_signature,
                                                   TypeParamsToArgsMap& type_params_to_args,
                                                   bool receiver_to_arg) {
  if (!parameterized_signature->type_parameters().empty()) {
    fail("attempted to instantiate method signature with type parameters");
  }
  Tuple* parameters = parameterized_signature->parameters();
  if (parameters != nullptr) {
    parameters = static_cast<Tuple*>(InstantiateTuple(parameters, type_params_to_args));
  }
  Tuple* results = parameterized_signature->results();
  if (results != nullptr) {
    results = static_cast<Tuple*>(InstantiateTuple(results, type_params_to_args));
  }
  if (receiver_to_arg) {
    if (parameterized_signature->expr_receiver() == nullptr) {
      fail("attempted to instantiate missing expr receiver");
    }
    Variable* receiver = parameterized_signature->expr_receiver();
    Type* receiver_type = receiver->type();
    receiver_type = InstantiateType(receiver_type, type_params_to_args);
    receiver = CreateVariable(receiver->parent(), receiver->package(), receiver->position(),
                              receiver->name(), receiver->is_embedded(), receiver->is_field());
    SetObjectType(receiver, receiver_type);
    std::vector<types::Variable*> params = parameters->variables();
    params.insert(params.begin(), receiver);
    parameters = CreateTuple(params);
  }
  return CreateSignature(parameters, results);
}

Type* InfoBuilder::InstantiateType(Type* type, TypeParamsToArgsMap& type_params_to_args) {
  switch (type->type_kind()) {
    case types::TypeKind::kBasic:
      return type;
    case types::TypeKind::kPointer:
      return InstantiatePointer(static_cast<Pointer*>(type), type_params_to_args);
    case types::TypeKind::kArray:
      return InstantiateArray(static_cast<Array*>(type), type_params_to_args);
    case types::TypeKind::kSlice:
      return InstantiateSlice(static_cast<Slice*>(type), type_params_to_args);
    case types::TypeKind::kTypeParameter:
      return InstantiateTypeParameter(static_cast<TypeParameter*>(type), type_params_to_args);
    case types::TypeKind::kNamedType:
      return InstantiateNamedType(static_cast<NamedType*>(type));
    case types::TypeKind::kTypeInstance:
      return InstantiateTypeInstance(static_cast<TypeInstance*>(type), type_params_to_args);
    case types::TypeKind::kTuple:
      return InstantiateTuple(static_cast<Tuple*>(type), type_params_to_args);
    case types::TypeKind::kSignature:
      return InstantiateSignature(static_cast<Signature*>(type), type_params_to_args);
    case types::TypeKind::kStruct:
      return InstantiateStruct(static_cast<Struct*>(type), type_params_to_args);
    case types::TypeKind::kInterface:
      return InstantiateInterface(static_cast<Interface*>(type), type_params_to_args);
  }
}

Pointer* InfoBuilder::InstantiatePointer(Pointer* pointer,
                                         TypeParamsToArgsMap& type_params_to_args) {
  Type* element_type = pointer->element_type();
  Type* element_type_instance = InstantiateType(element_type, type_params_to_args);
  if (element_type == element_type_instance) {
    return pointer;
  }
  return CreatePointer(pointer->kind(), element_type_instance);
}

Array* InfoBuilder::InstantiateArray(Array* array, TypeParamsToArgsMap& type_params_to_args) {
  Type* element_type = array->element_type();
  Type* element_type_instance = InstantiateType(element_type, type_params_to_args);
  if (element_type == element_type_instance) {
    return array;
  }
  return CreateArray(element_type_instance, array->length());
}

Slice* InfoBuilder::InstantiateSlice(Slice* slice, TypeParamsToArgsMap& type_params_to_args) {
  Type* element_type = slice->element_type();
  Type* element_type_instance = InstantiateType(element_type, type_params_to_args);
  if (element_type == element_type_instance) {
    return slice;
  }
  return CreateSlice(element_type_instance);
}

Type* InfoBuilder::InstantiateTypeParameter(TypeParameter* type_parameter,
                                            TypeParamsToArgsMap& type_params_to_args) {
  if (!type_params_to_args.contains(type_parameter)) {
    fail("type argument for type parameter not found");
  }
  return type_params_to_args.at(type_parameter);
}

NamedType* InfoBuilder::InstantiateNamedType(NamedType* named_type) {
  if (!named_type->type_parameters().empty()) {
    fail("attempted to instantiate nested named type with type parameters");
  }
  return named_type;
}

TypeInstance* InfoBuilder::InstantiateTypeInstance(TypeInstance* type_instance,
                                                   TypeParamsToArgsMap& type_params_to_args) {
  bool instantiated_type_arg = false;
  std::vector<Type*> type_arg_instances;
  type_arg_instances.reserve(type_instance->type_args().size());
  for (Type* type_arg : type_instance->type_args()) {
    Type* type_arg_instance = InstantiateType(type_arg, type_params_to_args);
    if (type_arg != type_arg_instance) {
      instantiated_type_arg = true;
    }
    type_arg_instances.push_back(type_arg_instance);
  }
  if (!instantiated_type_arg) {
    return type_instance;
  }
  return CreateTypeInstance(type_instance->instantiated_type(), type_arg_instances);
}

Tuple* InfoBuilder::InstantiateTuple(Tuple* tuple, TypeParamsToArgsMap& type_params_to_args) {
  bool instantiated_var_type = false;
  std::vector<Variable*> var_instances;
  var_instances.reserve(tuple->variables().size());
  for (auto var : tuple->variables()) {
    Type* var_type = var->type();
    Type* var_type_instance = InstantiateType(var_type, type_params_to_args);
    if (var_type == var_type_instance) {
      var_instances.push_back(var);
      continue;
    }
    instantiated_var_type = true;
    Variable* var_instance = CreateVariable(var->parent(), var->package(), var->position(),
                                            var->name(), var->is_embedded(), var->is_field());
    SetObjectType(var_instance, var_type_instance);
    var_instances.push_back(var_instance);
  }
  if (!instantiated_var_type) {
    return tuple;
  }
  return CreateTuple(var_instances);
}

Signature* InfoBuilder::InstantiateSignature(Signature* signature,
                                             TypeParamsToArgsMap& type_params_to_args) {
  if (!signature->type_parameters().empty()) {
    fail("attempted to instantiate nested signature with type parameters");
  } else if (signature->expr_receiver() != nullptr || signature->type_receiver() != nullptr) {
    fail("attempted to instantiate nested signature with receiver");
  }

  Tuple* parameters = signature->parameters();
  if (parameters != nullptr) {
    parameters = static_cast<Tuple*>(InstantiateTuple(parameters, type_params_to_args));
  }
  Tuple* results = signature->results();
  if (results != nullptr) {
    results = static_cast<Tuple*>(InstantiateTuple(results, type_params_to_args));
  }
  if (parameters == signature->parameters() && results == signature->results()) {
    return signature;
  }
  return CreateSignature(parameters, results);
}

Struct* InfoBuilder::InstantiateStruct(Struct* struct_type,
                                       TypeParamsToArgsMap& type_params_to_args) {
  bool instantiated_field = false;
  std::vector<Variable*> field_instances;
  field_instances.reserve(struct_type->fields().size());
  for (Variable* field : struct_type->fields()) {
    Type* field_type = field->type();
    Type* field_type_instance = InstantiateType(field_type, type_params_to_args);
    if (field_type == field_type_instance) {
      field_instances.push_back(field);
      continue;
    }
    instantiated_field = true;
    Variable* field_instance =
        CreateVariable(field->parent(), field->package(), field->position(), field->name(),
                       field->is_embedded(), field->is_field());
    SetObjectType(field_instance, field_type_instance);
    field_instances.push_back(field_instance);
  }
  if (!instantiated_field) {
    return struct_type;
  }
  return CreateStruct(field_instances);
}

Interface* InfoBuilder::InstantiateInterface(Interface* interface,
                                             TypeParamsToArgsMap& type_params_to_args) {
  // TODO: support embedded interfaces
  bool instantiated_method = false;
  std::vector<Func*> method_instances;
  method_instances.reserve(interface->methods().size());
  for (Func* method : interface->methods()) {
    Signature* method_sig = static_cast<Signature*>(method->type());
    Signature* method_sig_instance = InstantiateSignature(method_sig, type_params_to_args);
    if (method_sig == method_sig_instance) {
      method_instances.push_back(method);
      continue;
    }
    instantiated_method = true;
    Func* method_instance =
        CreateFunc(method->parent(), method->package(), method->position(), method->name());
    SetObjectType(method_instance, method_sig_instance);
    method_instances.push_back(method_instance);
  }
  if (!instantiated_method) {
    return interface;
  }
  types::Interface* interface_instance = CreateInterface();
  SetInterfaceMembers(interface_instance, {}, method_instances);
  return interface_instance;
}

void InfoBuilder::SetTypeParameterInstance(TypeParameter* instantiated, TypeParameter* instance) {
  if (instantiated == instance) {
    fail(
        "attempted to set instantiated type parameter of type parameter to "
        "itself");
  } else if (instantiated->instantiated_type_parameter() != nullptr) {
    fail(
        "attempted to set instantiated type parameter of type parameter to type "
        "parameter with its own instantiated type parameter");
  } else if (instance->instantiated_type_parameter() != nullptr) {
    fail("attempted to set instantiated type parameter of type parameter twice");
  } else if (instantiated->interface() == nullptr) {
    fail(
        "attempted to set instantiated type parameter of type parameter to type "
        "parameter without interface");
  } else if (instance->interface() != nullptr) {
    fail(
        "attempted to set instantiated type parameter of type parameter with "
        "already set interface");
  }
  instance->instantiated_type_parameter_ = instantiated;
  instance->interface_ = instantiated->interface();
}

void InfoBuilder::SetTypeParameterInterface(TypeParameter* type_parameter, Interface* interface) {
  if (type_parameter->interface() != nullptr) {
    fail("attempted to set interface of type parameter twice");
  } else if (interface == nullptr) {
    fail("attempted to set interface of type parameter to nullptr");
  }
  type_parameter->interface_ = interface;
}

void InfoBuilder::SetTypeParametersOfNamedType(NamedType* named_type,
                                               std::vector<TypeParameter*> type_parameters) {
  if (!named_type->type_parameters().empty()) {
    fail("attempted to set type parameters of named type twice");
  }
  named_type->type_parameters_ = type_parameters;
}

void InfoBuilder::SetUnderlyingTypeOfNamedType(NamedType* named_type, Type* underlying_type) {
  if (named_type->underlying() != nullptr) {
    fail("attempted to set underlying type of named type twice");
  } else if (underlying_type == nullptr) {
    fail("attempted to set underlying type of named type to nullptr");
  }
  named_type->underlying_ = underlying_type;
}

void InfoBuilder::AddMethodToNamedType(NamedType* named_type, Func* method) {
  if (named_type->methods().contains(method->name())) {
    fail("attempted to add two methods with the same name to named type");
  }
  named_type->methods_[method->name()] = method;
}

void InfoBuilder::AddInstanceToNamedType(NamedType* named_type, std::vector<types::Type*> type_args,
                                         Type* instance) {
  named_type->SetInstanceForTypeArgs(type_args, instance);
}

void InfoBuilder::SetInterfaceMembers(Interface* interface,
                                      std::vector<NamedType*> embdedded_interfaces,
                                      std::vector<Func*> methods) {
  if (!interface->embedded_interfaces().empty()) {
    fail("attempted to add set embdedded interfaces twice");
  }
  if (!interface->methods().empty()) {
    fail("attempted to add set interface methods twice");
  }
  interface->embedded_interfaces_ = embdedded_interfaces;
  interface->methods_ = methods;
}

TypeName* InfoBuilder::CreateTypeNameForTypeParameter(Scope* parent, Package* package,
                                                      pos_t position, std::string name) {
  CheckObjectArgs(parent, package);
  std::unique_ptr<TypeName> type_name(new TypeName(parent, package, position, name));
  type_name->type_ = CreateTypeParameter(name);
  TypeName* type_name_ptr = type_name.get();
  info_->object_unique_ptrs_.push_back(std::move(type_name));
  return type_name_ptr;
}

TypeName* InfoBuilder::CreateTypeNameForNamedType(Scope* parent, Package* package, pos_t position,
                                                  std::string name, bool is_alias) {
  CheckObjectArgs(parent, package);
  std::unique_ptr<TypeName> type_name(new TypeName(parent, package, position, name));
  type_name->type_ = CreateNamedType(is_alias, name);
  TypeName* type_name_ptr = type_name.get();
  info_->object_unique_ptrs_.push_back(std::move(type_name));
  return type_name_ptr;
}

Constant* InfoBuilder::CreateConstant(Scope* parent, Package* package, pos_t position,
                                      std::string name) {
  CheckObjectArgs(parent, package);
  std::unique_ptr<Constant> constant(new Constant(parent, package, position, name));
  Constant* constant_ptr = constant.get();
  info_->object_unique_ptrs_.push_back(std::move(constant));
  return constant_ptr;
}

Variable* InfoBuilder::CreateVariable(Scope* parent, Package* package, pos_t position,
                                      std::string name, bool is_embedded, bool is_field) {
  CheckObjectArgs(parent, package);
  std::unique_ptr<Variable> variable(
      new Variable(parent, package, position, name, is_embedded, is_field));
  Variable* variable_ptr = variable.get();
  info_->object_unique_ptrs_.push_back(std::move(variable));
  return variable_ptr;
}

Func* InfoBuilder::CreateFunc(Scope* parent, Package* package, pos_t position, std::string name) {
  CheckObjectArgs(parent, package);
  std::unique_ptr<Func> func(new Func(parent, package, position, name));
  Func* func_ptr = func.get();
  info_->object_unique_ptrs_.push_back(std::move(func));
  return func_ptr;
}

Label* InfoBuilder::CreateLabel(Scope* parent, Package* package, pos_t position, std::string name) {
  std::unique_ptr<Label> label(new Label(parent, package, position, name));
  Label* label_ptr = label.get();
  info_->object_unique_ptrs_.push_back(std::move(label));
  return label_ptr;
}

PackageName* InfoBuilder::CreatePackageName(Scope* parent, Package* package, pos_t position,
                                            std::string name, Package* referenced_package) {
  CheckObjectArgs(parent, package);
  std::unique_ptr<PackageName> package_name(
      new PackageName(parent, package, position, name, referenced_package));
  PackageName* package_name_ptr = package_name.get();
  info_->object_unique_ptrs_.push_back(std::move(package_name));
  return package_name_ptr;
}

void InfoBuilder::CheckObjectArgs(Scope* parent, Package* package) const {
  if (parent == nullptr) {
    fail("attemtped to create object without parent scope");
  }
  if (package == nullptr) {
    fail("attempted to create object without package");
  }
}

void InfoBuilder::SetObjectType(TypedObject* object, Type* type) {
  if (object->object_kind() == ObjectKind::kTypeName) {
    fail("attempted to set type name type as regular object type");
  } else if (object->type() != nullptr) {
    fail("attempted to set object type twice");
  }
  object->type_ = type;
}

void InfoBuilder::SetConstantValue(Constant* constant, constants::Value value) {
  constant->value_ = value;
}

void InfoBuilder::SetExprInfo(ast::Expr* expr, ExprInfo kind) {
  if (info_->expr_infos().contains(expr)) {
    fail("attempted to set expression info twice");
  }
  info_->expr_infos_.insert({expr, kind});
}

void InfoBuilder::SetDefinedObject(ast::Ident* ident, Object* object) {
  if (info_->definitions().contains(ident)) {
    fail("attempted to set defined object of identifier twice");
  }
  info_->definitions_.insert({ident, object});
}

void InfoBuilder::SetUsedObject(ast::Ident* ident, Object* object) {
  if (info_->uses().contains(ident)) {
    fail("attempted to set used object of identifier twice");
  }
  info_->uses_.insert({ident, object});
}

void InfoBuilder::SetImplicitObject(ast::Node* node, Object* object) {
  if (info_->implicits().contains(node)) {
    fail("attempted to set implicit object of node twice");
  }
  info_->implicits_.insert({node, object});
}

Scope* InfoBuilder::CreateScope(ast::Node* node, Scope* parent) {
  if (node == nullptr) {
    fail("attempted to create scope without associated node");
  }
  if (parent == nullptr) {
    fail("attempted to create scope without parent");
  }

  std::unique_ptr<Scope> scope(new Scope());
  scope->parent_ = parent;

  Scope* scope_ptr = scope.get();
  info_->scope_unique_ptrs_.push_back(std::move(scope));
  info_->scopes_.insert({node, scope_ptr});
  parent->children_.push_back(scope_ptr);

  return scope_ptr;
}

void InfoBuilder::AddObjectToScope(Scope* scope, Object* object) {
  if (!object->name().empty() && scope->named_objects().contains(object->name())) {
    fail("attempted to add two objects with the same name to scope");
  }

  if (object->name().empty()) {
    scope->unnamed_objects_.insert(object);
  } else {
    scope->named_objects_.insert({object->name(), object});
  }
}

Package* InfoBuilder::CreatePackage(std::string path, std::string name) {
  if (name.empty()) {
    fail("attempted to create package with empty name");
  }

  std::unique_ptr<Scope> package_scope(new Scope());
  package_scope->parent_ = info_->universe();

  Scope* package_scope_ptr = package_scope.get();
  info_->scope_unique_ptrs_.push_back(std::move(package_scope));
  info_->universe_->children_.push_back(package_scope_ptr);

  std::unique_ptr<Package> package = std::unique_ptr<Package>(new Package());
  package->path_ = path;
  package->name_ = name;
  package->scope_ = package_scope_ptr;

  Package* package_ptr = package.get();
  info_->package_unique_ptrs_.push_back(std::move(package));
  info_->packages_.insert(package_ptr);

  return package_ptr;
}

void InfoBuilder::AddImportToPackage(Package* importer, Package* imported) {
  importer->imports_.insert(imported);
}

void InfoBuilder::SetSelection(ast::SelectionExpr* selection_expr, types::Selection selection) {
  if (info_->selections_.contains(selection_expr)) {
    fail("attempted to set selection of selection expr twice");
  }
  info_->selections_.insert({selection_expr, selection});
}

void InfoBuilder::AddInitializer(Initializer initializer) {
  info_->init_order_.push_back(initializer);
}

}  // namespace types
}  // namespace lang
