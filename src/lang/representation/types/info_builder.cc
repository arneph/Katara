//
//  info_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "info_builder.h"

namespace lang {
namespace types {

InfoBuilder::InfoBuilder(Info *info) : info_(info) {}

Info * InfoBuilder::info() const {
    return info_;
}

void InfoBuilder::CreateUniverse() {
    if (info_->universe_ != nullptr) {
        return;
    }
    
    auto universe = std::unique_ptr<Scope>(new Scope());
    universe->parent_ = nullptr;
    
    info_->universe_ = universe.get();
    info_->scope_unique_ptrs_.push_back(std::move(universe));
    
    CreatePredeclaredTypes();
    CreatePredeclaredConstants();
    CreatePredeclaredNil();
    CreatePredeclaredFuncs();
}

void InfoBuilder::CreatePredeclaredTypes() {
    typedef struct{
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
            types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned},
            "uint"},
        {types::Basic::kUint8,
            types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned},
            "uint8"},
        {types::Basic::kUint16,
            types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned},
            "uint16"},
        {types::Basic::kUint32,
            types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned},
            "uint32"},
        {types::Basic::kUint64,
            types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUnsigned},
            "uint64"},
        {types::Basic::kString, types::Basic::kIsString, "string"},
        
        {types::Basic::kUntypedBool,
            types::Basic::Info{types::Basic::kIsBoolean | types::Basic::kIsUntyped},
            "untyped bool"},
        {types::Basic::kUntypedInt,
            types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUntyped},
            "untyped int"},
        {types::Basic::kUntypedRune,
            types::Basic::Info{types::Basic::kIsInteger | types::Basic::kIsUntyped},
            "untyped rune"},
        {types::Basic::kUntypedString,
            types::Basic::Info{types::Basic::kIsString | types::Basic::kIsUntyped},
            "untyped string"},
        {types::Basic::kUntypedNil, types::Basic::kIsUntyped, "untyped nil"},
    });
    for (auto predeclared_type : predeclared_types) {
        auto basic = std::unique_ptr<types::Basic>(new Basic());
        basic->kind_ = predeclared_type.kind;
        basic->info_ = predeclared_type.info;
        
        auto basic_ptr = basic.get();
        info_->type_unique_ptrs_.push_back(std::move(basic));
        info_->basic_types_.insert({predeclared_type.kind, basic_ptr});
        
        auto it = std::find(predeclared_type.name.begin(),
                            predeclared_type.name.end(),
                            ' ');
        if (it != predeclared_type.name.end()) {
            continue;
        }
        
        auto type_name = std::unique_ptr<types::TypeName>(new types::TypeName());
        type_name->parent_ = info_->universe();
        type_name->package_ = nullptr;
        type_name->position_ = pos::kNoPos;
        type_name->name_ = predeclared_type.name;
        type_name->type_ = basic_ptr;
        type_name->is_alias_ = false;
        
        auto type_name_ptr = type_name.get();
        info_->object_unique_ptrs_.push_back(std::move(type_name));
        info_->universe_->named_objects_.insert({predeclared_type.name, type_name_ptr});
    }
}

void InfoBuilder::CreatePredeclaredConstants() {
    typedef struct{
        types::Basic::Kind kind;
        constants::Value value;
        std::string name;
    } predeclared_const_t;
    auto predeclared_consts = std::vector<predeclared_const_t>({
        {types::Basic::kUntypedBool, constants::Value(false), "false"},
        {types::Basic::kUntypedBool, constants::Value(true), "true"},
        {types::Basic::kUntypedInt, constants::Value(int64_t{0}), "iota"},
    });
    for (auto predeclared_const : predeclared_consts) {
        auto constant = std::unique_ptr<types::Constant>(new Constant());
        constant->parent_ = info_->universe();
        constant->package_ = nullptr;
        constant->position_ = pos::kNoPos;
        constant->name_ = predeclared_const.name;
        constant->type_ = info_->basic_types_.at(predeclared_const.kind);
        constant->value_ = predeclared_const.value;
        
        info_->universe_->named_objects_.insert({predeclared_const.name, constant.get()});
        info_->object_unique_ptrs_.push_back(std::move(constant));
    }
}

void InfoBuilder::CreatePredeclaredNil() {
    auto nil = std::unique_ptr<types::Nil>(new Nil());
    nil->parent_ = info_->universe();
    nil->package_ = nullptr;
    nil->position_ = pos::kNoPos;
    nil->name_ = "nil";
    nil->type_ = info_->basic_type(types::Basic::kUntypedNil);
    
    info_->universe_->named_objects_.insert({"nil", nil.get()});
    info_->object_unique_ptrs_.push_back(std::move(nil));
}

void InfoBuilder::CreatePredeclaredFuncs() {
    typedef struct{
        types::Builtin::Kind kind;
        std::string name;
    } predeclared_builtin_t;
    auto predeclared_builtins = std::vector<predeclared_builtin_t>({
        {types::Builtin::Kind::kLen, "len"},
        {types::Builtin::Kind::kMake, "make"},
        {types::Builtin::Kind::kNew, "new"},
    });
    for (auto predeclared_builtin : predeclared_builtins) {
        std::unique_ptr<types::Builtin> builtin(new Builtin());
        builtin->parent_ = info_->universe();
        builtin->package_ = nullptr;
        builtin->position_ = pos::kNoPos;
        builtin->name_ = predeclared_builtin.name;
        builtin->type_ = nullptr;
        builtin->kind_ = predeclared_builtin.kind;
        
        info_->universe_->named_objects_.insert({predeclared_builtin.name, builtin.get()});
        info_->object_unique_ptrs_.push_back(std::move(builtin));
    }
}

Pointer * InfoBuilder::CreatePointer(Pointer::Kind kind, Type *element_type) {
    std::unique_ptr<Pointer> pointer(new Pointer());
    pointer->kind_ = kind;
    pointer->element_type_ = element_type;
    
    Pointer *pointer_ptr = pointer.get();
    info_->type_unique_ptrs_.push_back(std::move(pointer));
    
    return pointer_ptr;
}

Array * InfoBuilder::CreateArray(Type *element_type, uint64_t length) {
    std::unique_ptr<Array> array(new Array());
    array->element_type_ = element_type;
    array->length_ = length;
    
    Array *array_ptr = array.get();
    info_->type_unique_ptrs_.push_back(std::move(array));
    
    return array_ptr;
}

Slice * InfoBuilder::CreateSlice(Type *element_type) {
    std::unique_ptr<Slice> slice(new Slice());
    slice->element_type_ = element_type;
    
    Slice *slice_ptr = slice.get();
    info_->type_unique_ptrs_.push_back(std::move(slice));
    
    return slice_ptr;
}

TypeTuple * InfoBuilder::CreateTypeTuple(std::vector<NamedType *> types) {
    std::unique_ptr<TypeTuple> type_tuple(new TypeTuple());
    type_tuple->types_ = types;
    
    TypeTuple *type_tuple_ptr = type_tuple.get();
    info_->type_unique_ptrs_.push_back(std::move(type_tuple));
    
    return type_tuple_ptr;
}

NamedType * InfoBuilder::CreateNamedType(bool is_type_parameter,
                                         std::string name,
                                         Type *type,
                                         TypeTuple *type_parameters) {
    std::unique_ptr<NamedType> named_type(new NamedType());
    named_type->is_type_parameter_ = is_type_parameter;
    named_type->name_ = name;
    named_type->type_ = type;
    named_type->type_parameters_ = type_parameters;
    
    NamedType *named_type_ptr = named_type.get();
    info_->type_unique_ptrs_.push_back(std::move(named_type));
    
    return named_type_ptr;
}

TypeInstance * InfoBuilder::CreateTypeInstance(Type *instantiated_type,
                                               std::vector<Type *> type_args) {
    std::unique_ptr<TypeInstance> type_instance(new TypeInstance());
    type_instance->instantiated_type_ = instantiated_type;
    type_instance->type_args_ = type_args;
    
    TypeInstance *type_instance_ptr = type_instance.get();
    info_->type_unique_ptrs_.push_back(std::move(type_instance));
    
    return type_instance_ptr;
}

Tuple * InfoBuilder::CreateTuple(std::vector<Variable *> variables) {
    std::unique_ptr<Tuple> tuple(new Tuple());
    tuple->variables_ = variables;
    
    Tuple *tuple_ptr = tuple.get();
    info_->type_unique_ptrs_.push_back(std::move(tuple));
    
    return tuple_ptr;
}

Signature * InfoBuilder::CreateSignature(Variable *receiver,
                                         TypeTuple *type_parameters,
                                         Tuple *parameters,
                                         Tuple *results) {
    std::unique_ptr<Signature> signature(new Signature());
    signature->receiver_ = receiver;
    signature->type_parameters_ = type_parameters;
    signature->parameters_ = parameters;
    signature->results_ = results;
    
    Signature *signature_ptr = signature.get();
    info_->type_unique_ptrs_.push_back(std::move(signature));
    
    return signature_ptr;
}

Struct * InfoBuilder::CreateStruct(std::vector<Variable *> fields) {
    std::unique_ptr<Struct> struct_(new Struct());
    struct_->fields_ = fields;
    
    Struct *struct_ptr = struct_.get();
    info_->type_unique_ptrs_.push_back(std::move(struct_));
    
    return struct_ptr;
}

Interface * InfoBuilder::CreateInterface(std::vector<NamedType *> embedded_interfaces,
                                         std::vector<Func *> methods) {
    std::unique_ptr<Interface> interface(new Interface());
    interface->embedded_interfaces_ = embedded_interfaces;
    interface->methods_ = methods;
    
    Interface *interface_ptr = interface.get();
    info_->type_unique_ptrs_.push_back(std::move(interface));
    
    return interface_ptr;
}

Type * InfoBuilder::InstantiateType(Type *parameterized_type,
                                    std::unordered_map<NamedType *, Type *> type_params_to_args) {
    // TODO: implement
    return nullptr;
}

TypeName * InfoBuilder::CreateTypeName(Scope *parent,
                                       Package *package,
                                       pos::pos_t position,
                                       std::string name,
                                       bool is_alias) {
    CheckObjectArgs(parent, package);
    
    std::unique_ptr<TypeName> type_name(new TypeName());
    type_name->parent_ = parent;
    type_name->package_ = package;
    type_name->position_ = position;
    type_name->name_ = name;
    type_name->type_ = nullptr;
    type_name->is_alias_ = is_alias;
    
    TypeName *type_name_ptr = type_name.get();
    info_->object_unique_ptrs_.push_back(std::move(type_name));
    
    return type_name_ptr;
}

Constant * InfoBuilder::CreateConstant(Scope *parent,
                                       Package *package,
                                       pos::pos_t position,
                                       std::string name) {
    CheckObjectArgs(parent, package);
    
    std::unique_ptr<Constant> constant(new Constant());
    constant->parent_ = parent;
    constant->package_ = package;
    constant->position_ = position;
    constant->name_ = name;
    constant->type_ = nullptr;
    
    Constant *constant_ptr = constant.get();
    info_->object_unique_ptrs_.push_back(std::move(constant));
    
    return constant_ptr;
}

Variable * InfoBuilder::CreateVariable(Scope *parent,
                                       Package *package,
                                       pos::pos_t position,
                                       std::string name,
                                       bool is_embedded,
                                       bool is_field) {
    CheckObjectArgs(parent, package);
    
    std::unique_ptr<Variable> variable(new Variable());
    variable->parent_ = parent;
    variable->package_ = package;
    variable->position_ = position;
    variable->name_ = name;
    variable->type_ = nullptr;
    variable->is_embedded_ = is_embedded;
    variable->is_field_ = is_field;
    
    Variable *variable_ptr = variable.get();
    info_->object_unique_ptrs_.push_back(std::move(variable));
    
    return variable_ptr;
}

Func * InfoBuilder::CreateFunc(Scope *parent,
                               Package *package,
                               pos::pos_t position,
                               std::string name) {
    CheckObjectArgs(parent, package);
    
    std::unique_ptr<Func> func(new Func());
    func->parent_ = parent;
    func->package_ = package;
    func->position_ = position;
    func->name_ = name;
    func->type_ = nullptr;
    
    Func *func_ptr = func.get();
    info_->object_unique_ptrs_.push_back(std::move(func));
    
    return func_ptr;
}

Label * InfoBuilder::CreateLabel(Scope *parent,
                                 Package *package,
                                 pos::pos_t position,
                                 std::string name) {
    std::unique_ptr<Label> label(new Label());
    label->parent_ = parent;
    label->package_ = package;
    label->position_ = position;
    label->name_ = name;
    label->type_ = nullptr;
    
    Label *label_ptr = label.get();
    info_->object_unique_ptrs_.push_back(std::move(label));
    
    return label_ptr;
}

PackageName * InfoBuilder::CreatePackageName(Scope *parent,
                                             Package *package,
                                             pos::pos_t position,
                                             std::string name,
                                             Package *referenced_package) {
    CheckObjectArgs(parent, package);
    
    std::unique_ptr<PackageName> package_name(new PackageName());
    package_name->parent_ = parent;
    package_name->package_ = package;
    package_name->position_ = position;
    package_name->name_ = name;
    package_name->type_ = nullptr;
    package_name->referenced_package_ = referenced_package;
    
    PackageName *package_name_ptr = package_name.get();
    info_->object_unique_ptrs_.push_back(std::move(package_name));
    
    return package_name_ptr;
}

void InfoBuilder::CheckObjectArgs(Scope *parent,
                                  Package *package) const {
    if (parent == nullptr) {
        throw "internal error: attemtped to create object without parent scope";
    }
    if (package == nullptr) {
        throw "internal error: attempted to create object without package";
    }
}

void InfoBuilder::SetObjectType(Object *object, Type *type) {
    if (object->type() != nullptr) {
        throw "internal error: attempted to set object type twice";
    }
    object->type_ = type;
}

void InfoBuilder::SetConstantValue(Constant *constant, constants::Value value) {
    constant->value_ = value;
}

void InfoBuilder::SetExprType(ast::Expr *expr, Type *type) {
    if (info_->types().contains(expr)) {
        throw "internal error: attempted to set expression type twice";
    }
    info_->types_.insert({expr, type});
}

void InfoBuilder::SetExprKind(ast::Expr *expr, ExprKind kind) {
    if (info_->expr_kinds().contains(expr)) {
        throw "internal error: attempted to set expression kind twice";
    }
    info_->expr_kinds_.insert({expr, kind});
}

void InfoBuilder::SetExprConstantValue(ast::Expr *expr, constants::Value value) {
    if (info_->constant_values().contains(expr)) {
        throw "internal error: attempted to set expression value twice";
    }
    info_->constant_values_.insert({expr, value});
}

void InfoBuilder::SetDefinedObject(ast::Ident *ident, Object *object) {
    if (info_->definitions().contains(ident)) {
        throw "internal error: attempted to set defined object of identifier twice";
    }
    info_->definitions_.insert({ident, object});
}

void InfoBuilder::SetUsedObject(ast::Ident *ident, Object *object) {
    if (info_->uses().contains(ident)) {
        throw "internal error: attempted to set used object of identifier twice";
    }
    info_->uses_.insert({ident, object});
}

void InfoBuilder::SetImplicitObject(ast::Node *node, Object *object) {
    if (info_->implicits().contains(node)) {
        throw "internal error: attempted to set implicit object of node twice";
    }
    info_->implicits_.insert({node, object});
}

Scope * InfoBuilder::CreateScope(ast::Node *node, Scope *parent) {
    if (node == nullptr) {
        throw "internal error: attempted to create scope without associated node";
    }
    if (parent == nullptr) {
        throw "internal error: attempted to create scope without parent";
    }
    
    std::unique_ptr<Scope> scope(new Scope());
    scope->parent_ = parent;
    
    Scope *scope_ptr = scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(scope));
    info_->scopes_.insert({node, scope_ptr});
    parent->children_.push_back(scope_ptr);
    
    return scope_ptr;
}

void InfoBuilder::AddObjectToScope(Scope *scope, Object *object) {
    if (!object->name().empty() &&
        scope->named_objects().contains(object->name())) {
        throw "internal error: attempted to add two objects with the same name to scope";
    }
    
    if (object->name().empty()) {
        scope->unnamed_objects_.insert(object);
    } else {
        scope->named_objects_.insert({object->name(), object});
    }
}

Package * InfoBuilder::CreatePackage(std::string path, std::string name) {
    if (name.empty()) {
        throw "internal error: attempted to create package with empty name";
    }
    
    std::unique_ptr<Scope> package_scope(new Scope());
    package_scope->parent_ = info_->universe();
    
    Scope *package_scope_ptr = package_scope.get();
    info_->scope_unique_ptrs_.push_back(std::move(package_scope));
    info_->universe_->children_.push_back(package_scope_ptr);
    
    std::unique_ptr<Package> package = std::unique_ptr<Package>(new Package());
    package->path_ = path;
    package->name_ = name;
    package->scope_ = package_scope_ptr;
    
    Package *package_ptr = package.get();
    info_->package_unique_ptrs_.push_back(std::move(package));
    info_->packages_.insert(package_ptr);
    
    return package_ptr;
}

void InfoBuilder::AddImportToPackage(Package *importer, Package *imported) {
    importer->imports_.insert(imported);
}

void InfoBuilder::AddInitializer(std::vector<Variable *> lhs, ast::Expr *rhs) {
    std::unique_ptr<Initializer> initializer(new Initializer());
    initializer->lhs_ = lhs;
    initializer->rhs_ = rhs;
    
    Initializer *initializer_ptr = initializer.get();
    info_->initializer_unique_ptrs_.push_back(std::move(initializer));
    info_->init_order_.push_back(initializer_ptr);
}

}
}
