//
//  value_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "value_builder.h"

#include "src/common/logging/logging.h"

namespace lang {
namespace ir_builder {

using ::common::atomics::Bool;
using ::common::atomics::Int;
using ::common::logging::fail;

std::shared_ptr<ir::Computed> ValueBuilder::BuildBoolNot(std::shared_ptr<ir::Value> x,
                                                         IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir::bool_type(), ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::BoolNotInstr>(result, x));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildBoolBinaryOp(std::shared_ptr<ir::Value> x,
                                                              Bool::BinaryOp op,
                                                              std::shared_ptr<ir::Value> y,
                                                              IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir::bool_type(), ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::BoolBinaryInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildIntUnaryOp(Int::UnaryOp op,
                                                            std::shared_ptr<ir::Value> x,
                                                            IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(x->type(), ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::IntUnaryInstr>(result, op, x));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildIntBinaryOp(std::shared_ptr<ir::Value> x,
                                                             Int::BinaryOp op,
                                                             std::shared_ptr<ir::Value> y,
                                                             IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(x->type(), ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::IntBinaryInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildIntCompareOp(std::shared_ptr<ir::Value> x,
                                                              Int::CompareOp op,
                                                              std::shared_ptr<ir::Value> y,
                                                              IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir::bool_type(), ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::IntCompareInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildIntShiftOp(std::shared_ptr<ir::Value> x,
                                                            Int::ShiftOp op,
                                                            std::shared_ptr<ir::Value> y,
                                                            IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(x->type(), ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::IntShiftInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildStringConcat(std::shared_ptr<ir::Value> x,
                                                              std::shared_ptr<ir::Value> y,
                                                              IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_ext::string(), ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir_ext::StringConcatInstr>(
      result, std::vector<std::shared_ptr<ir::Value>>{x, y}));
  return result;
}

std::shared_ptr<ir::Value> ValueBuilder::BuildStringComparison(std::shared_ptr<ir::Value> x,
                                                               tokens::Token op,
                                                               std::shared_ptr<ir::Value> y,
                                                               IRContext& ir_ctx) {
  // TODO: implement
  return ir::True();
}

std::shared_ptr<ir::Value> ValueBuilder::BuildConversion(std::shared_ptr<ir::Value> value,
                                                         const ir::Type* desired_type,
                                                         IRContext& ir_ctx) {
  if (value->type() == desired_type) {
    return value;
  } else if (ir::IsAtomicType(value->type()->type_kind()) &&
             ir::IsAtomicType(desired_type->type_kind())) {
    std::shared_ptr<ir::Computed> result =
        std::make_shared<ir::Computed>(desired_type, ir_ctx.func()->next_computed_number());
    ir_ctx.block()->instrs().push_back(std::make_unique<ir::Conversion>(result, value));
    return result;
  } else {
    fail("unexpected conversion");
  }
}

std::shared_ptr<ir::Value> ValueBuilder::BuildDefaultForType(types::Type* types_type) {
  const ir::Type* ir_type = type_builder_.BuildType(types_type);
  switch (types_type->type_kind()) {
    case types::TypeKind::kBasic:
      switch (ir_type->type_kind()) {
        case ir::TypeKind::kBool:
          return ir::False();
        case ir::TypeKind::kInt:
          return ir::ZeroWithType(static_cast<const ir::IntType*>(ir_type)->int_type());
        case ir::TypeKind::kLangString:
          return std::make_shared<ir_ext::StringConstant>("");
        default:
          fail("unexpected ir type for basic type");
      }
    case types::TypeKind::kPointer:
      return ir::NilPointer();
    case types::TypeKind::kSignature:
      return ir::NilFunc();
    default:
      fail("unexpected lang type");
      //      return std::make_shared<ir_ext::StringConstant>("");
      //      // TODO: implement more types
  }
}

std::shared_ptr<ir::Value> ValueBuilder::BuildConstant(constants::Value constant) const {
  switch (constant.kind()) {
    case constants::Value::Kind::kBool:
      return ir::ToBoolConstant(constant.AsBool());
    case constants::Value::Kind::kInt:
      return ir::ToIntConstant(constant.AsInt());
    case constants::Value::Kind::kString:
      return std::make_shared<ir_ext::StringConstant>(constant.AsString());
  }
}

}  // namespace ir_builder
}  // namespace lang
