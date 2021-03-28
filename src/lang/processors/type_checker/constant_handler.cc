//
//  constant_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "constant_handler.h"

#include "lang/processors/type_checker/type_resolver.h"
#include "lang/representation/types/types_util.h"

namespace lang {
namespace type_checker {

bool ConstantHandler::ProcessConstant(types::Constant* constant, types::Type* type,
                                      ast::Expr* value_expr, int64_t iota) {
  if (type == nullptr && value_expr == nullptr) {
    issues().Add(issues::kMissingTypeOrValueForConstant, constant->position(),
                 "constant needs a type or value: " + constant->name());
    return false;
  }

  types::Basic* basic_type = nullptr;
  constants::Value value(int64_t{0});

  if (type != nullptr) {
    types::Type* underlying = types::UnderlyingOf(type);
    if (underlying == nullptr || underlying->type_kind() != types::TypeKind::kBasic) {
      issues().Add(issues::kConstantWithNonBasicType, constant->position(),
                   "constant can not have non-basic type: " + constant->name());
      return false;
    }
    basic_type = static_cast<types::Basic*>(underlying);
  }

  if (value_expr == nullptr) {
    value = ConvertUntypedInt(value, basic_type->kind());

  } else {
    if (!type_resolver().expr_handler().CheckExpr(
            value_expr, ExprHandler::Context(/*expect_constant=*/true, iota))) {
      return false;
    }
    types::ExprInfo value_expr_info = info()->ExprInfoOf(value_expr).value();
    types::Basic* given_type = static_cast<types::Basic*>(value_expr_info.type());
    constants::Value given_value = value_expr_info.constant_value();

    if (basic_type == nullptr) {
      type = given_type;
      basic_type = given_type;
    }

    if (given_type == basic_type) {
      value = given_value;

    } else if (given_type->info() & types::Basic::kIsUntyped) {
      value = ConvertUntypedInt(given_value, basic_type->kind());

    } else {
      issues().Add(issues::kConstantValueOfWrongType, constant->position(),
                   "constant can not have a value of a different type: " + constant->name());
      return false;
    }
  }

  info_builder().SetObjectType(constant, type);
  info_builder().SetConstantValue(constant, value);
  return true;
}

constants::Value ConstantHandler::ConvertUntypedInt(constants::Value value,
                                                    types::Basic::Kind kind) {
  switch (kind) {
    case types::Basic::kInt8:
      switch (value.value_.index()) {
        case 7:
          return constants::Value(int8_t(std::get<int64_t>(value.value_)));
        case 8:
          return constants::Value(int8_t(std::get<uint64_t>(value.value_)));
      }
    case types::Basic::kInt16:
      switch (value.value_.index()) {
        case 7:
          return constants::Value(int16_t(std::get<int64_t>(value.value_)));
        case 8:
          return constants::Value(int16_t(std::get<uint64_t>(value.value_)));
      }
    case types::Basic::kInt32:
      switch (value.value_.index()) {
        case 7:
          return constants::Value(int32_t(std::get<int64_t>(value.value_)));
        case 8:
          return constants::Value(int32_t(std::get<uint64_t>(value.value_)));
      }
    case types::Basic::kInt64:
    case types::Basic::kInt:
      switch (value.value_.index()) {
        case 7:
          return constants::Value(int64_t(std::get<int64_t>(value.value_)));
        case 8:
          return constants::Value(int64_t(std::get<uint64_t>(value.value_)));
      }
    case types::Basic::kUint8:
      switch (value.value_.index()) {
        case 7:
          return constants::Value(uint8_t(std::get<int64_t>(value.value_)));
        case 8:
          return constants::Value(uint8_t(std::get<uint64_t>(value.value_)));
      }
    case types::Basic::kUint16:
      switch (value.value_.index()) {
        case 7:
          return constants::Value(uint16_t(std::get<int64_t>(value.value_)));
        case 8:
          return constants::Value(uint16_t(std::get<uint64_t>(value.value_)));
      }
    case types::Basic::kUint32:
      switch (value.value_.index()) {
        case 7:
          return constants::Value(uint32_t(std::get<int64_t>(value.value_)));
        case 8:
          return constants::Value(uint32_t(std::get<uint64_t>(value.value_)));
      }
    case types::Basic::kUint64:
    case types::Basic::kUint:
      switch (value.value_.index()) {
        case 7:
          return constants::Value(uint64_t(std::get<int64_t>(value.value_)));
        case 8:
          return constants::Value(uint64_t(std::get<uint64_t>(value.value_)));
      }
    default:;
  }
  throw "internal error";
}

}  // namespace type_checker
}  // namespace lang
