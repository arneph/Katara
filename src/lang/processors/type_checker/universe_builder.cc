//
//  universe_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "universe_builder.h"

#include <memory>
#include <vector>

namespace lang {
namespace type_checker {

void UniverseBuilder::SetupUniverse(types::TypeInfo *info) {
    if (info->universe_ != nullptr) {
        return;
    }
    
    auto universe = std::unique_ptr<types::Scope>(new types::Scope());
    universe->parent_ = nullptr;
    
    info->universe_ = universe.get();
    info->scope_unique_ptrs_.push_back(std::move(universe));
    
    SetupPredeclaredTypes(info);
    SetupPredeclaredConstants(info);
    SetupPredeclaredNil(info);
}

void UniverseBuilder::SetupPredeclaredTypes(types::TypeInfo *info) {
    typedef struct{
        types::Basic::Kind kind_;
        types::Basic::Info info;
        std::string name_;
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
        auto basic = std::unique_ptr<types::Basic>(new types::Basic(predeclared_type.kind_,
                                                                    predeclared_type.info));
        
        auto basic_ptr = basic.get();
        info->type_unique_ptrs_.push_back(std::move(basic));
        info->basic_types_.insert({predeclared_type.kind_, basic_ptr});
        
        auto it = std::find(predeclared_type.name_.begin(),
                            predeclared_type.name_.end(),
                            ' ');
        if (it != predeclared_type.name_.end()) {
            continue;
        }
        
        auto type_name = std::unique_ptr<types::TypeName>(new types::TypeName());
        type_name->parent_ = info->universe_;
        type_name->package_ = nullptr;
        type_name->position_ = pos::kNoPos;
        type_name->name_ = predeclared_type.name_;
        type_name->type_ = basic_ptr;

        auto type_name_ptr = type_name.get();
        info->object_unique_ptrs_.push_back(std::move(type_name));
        info->universe_->named_objects_.insert({predeclared_type.name_, type_name_ptr});
    }
}

void UniverseBuilder::SetupPredeclaredConstants(types::TypeInfo *info) {
    typedef struct{
        types::Basic::Kind kind_;
        constants::Value value_;
        std::string name_;
    } predeclared_const_t;
    auto predeclared_consts = std::vector<predeclared_const_t>({
        {types::Basic::kUntypedBool, constants::Value(false), "false"},
        {types::Basic::kUntypedBool, constants::Value(true), "true"},
        {types::Basic::kUntypedInt, constants::Value(int64_t{0}), "iota"},
    });
    for (auto predeclared_const : predeclared_consts) {
        auto constant = std::unique_ptr<types::Constant>(new types::Constant());
        constant->parent_ = info->universe_;
        constant->package_ = nullptr;
        constant->position_ = pos::kNoPos;
        constant->name_ = predeclared_const.name_;
        constant->type_ = info->basic_types_.at(predeclared_const.kind_);
        constant->value_ = predeclared_const.value_;
        
        info->universe_->named_objects_.insert({predeclared_const.name_, constant.get()});
        info->object_unique_ptrs_.push_back(std::move(constant));
    }
}

void UniverseBuilder::SetupPredeclaredNil(types::TypeInfo *info) {
    auto nil = std::unique_ptr<types::Nil>(new types::Nil());
    nil->parent_ = info->universe_;
    nil->package_ = nullptr;
    nil->position_ = pos::kNoPos;
    nil->name_ = "nil";
    nil->type_ = info->basic_types_.at(types::Basic::kUntypedNil);
    
    info->universe_->named_objects_.insert({"nil", nil.get()});
    info->object_unique_ptrs_.push_back(std::move(nil));
}

}
}
