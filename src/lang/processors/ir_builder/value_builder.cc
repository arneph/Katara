//
//  value_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "value_builder.h"

#include "src/common/logging.h"

namespace lang {
namespace ir_builder {

std::shared_ptr<ir::Computed> ValueBuilder::BuildBoolNot(std::shared_ptr<ir::Value> x,
                                                         IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(&ir::kBool, ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::BoolNotInstr>(result, x));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildBoolBinaryOp(std::shared_ptr<ir::Value> x,
                                                              common::Bool::BinaryOp op,
                                                              std::shared_ptr<ir::Value> y,
                                                              IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(&ir::kBool, ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::BoolBinaryInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildIntUnaryOp(common::Int::UnaryOp op,
                                                            std::shared_ptr<ir::Value> x,
                                                            IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(x->type(), ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::IntUnaryInstr>(result, op, x));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildIntBinaryOp(std::shared_ptr<ir::Value> x,
                                                             common::Int::BinaryOp op,
                                                             std::shared_ptr<ir::Value> y,
                                                             IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(x->type(), ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::IntBinaryInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildIntCompareOp(std::shared_ptr<ir::Value> x,
                                                              common::Int::CompareOp op,
                                                              std::shared_ptr<ir::Value> y,
                                                              IRContext& ir_ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(&ir::kBool, ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::IntCompareInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Computed> ValueBuilder::BuildIntShiftOp(std::shared_ptr<ir::Value> x,
                                                            common::Int::ShiftOp op,
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
      std::make_shared<ir::Computed>(&ir_ext::kString, ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir_ext::StringConcatInstr>(
      result, std::vector<std::shared_ptr<ir::Value>>{x, y}));
  return result;
}

std::shared_ptr<ir::Value> ValueBuilder::BuildStringComparison(std::shared_ptr<ir::Value> x,
                                                               tokens::Token op,
                                                               std::shared_ptr<ir::Value> y,
                                                               IRContext& ir_ctx) {
  // TODO: implement
  return {std::make_shared<ir::BoolConstant>(true)};
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
    common::fail("unexpected conversion");
  }
}

std::shared_ptr<ir::Value> ValueBuilder::BuildDefaultForType(types::Type* types_type) {
  switch (types_type->type_kind()) {
    case types::TypeKind::kBasic: {
      const ir::Type* ir_type = type_builder_.BuildType(types_type);
      switch (ir_type->type_kind()) {
        case ir::TypeKind::kBool:
          return std::make_shared<ir::BoolConstant>(false);
        case ir::TypeKind::kInt:
          switch (static_cast<const ir::IntType*>(ir_type)->int_type()) {
            case common::IntType::kI8:
              return std::make_shared<ir::IntConstant>(common::Int(int8_t{0}));
            case common::IntType::kI16:
              return std::make_shared<ir::IntConstant>(common::Int(int16_t{0}));
            case common::IntType::kI32:
              return std::make_shared<ir::IntConstant>(common::Int(int32_t{0}));
            case common::IntType::kI64:
              return std::make_shared<ir::IntConstant>(common::Int(int64_t{0}));
            case common::IntType::kU8:
              return std::make_shared<ir::IntConstant>(common::Int(uint8_t{0}));
            case common::IntType::kU16:
              return std::make_shared<ir::IntConstant>(common::Int(uint16_t{0}));
            case common::IntType::kU32:
              return std::make_shared<ir::IntConstant>(common::Int(uint32_t{0}));
            case common::IntType::kU64:
              return std::make_shared<ir::IntConstant>(common::Int(uint64_t{0}));
          }
        case ir::TypeKind::kPointer:
          return std::make_shared<ir::PointerConstant>(0);
        case ir::TypeKind::kFunc:
          return std::make_shared<ir::FuncConstant>(0);
        case ir::TypeKind::kLangString:
          return std::make_shared<ir_ext::StringConstant>("");
        default:
          common::fail("unexpected ir type for basic type");
      }
    }
    default:
      return std::make_shared<ir_ext::StringConstant>("");
      // TODO: implement more types
      // common::fail("unexpected lang type");
  }
}

std::shared_ptr<ir::Value> ValueBuilder::BuildConstant(constants::Value constant) const {
  switch (constant.kind()) {
    case constants::Value::Kind::kBool:
      return std::make_shared<ir::BoolConstant>(constant.AsBool());
    case constants::Value::Kind::kInt:
      return std::make_shared<ir::IntConstant>(constant.AsInt());
    case constants::Value::Kind::kString:
      return std::make_shared<ir_ext::StringConstant>(constant.AsString());
  }
}

}  // namespace ir_builder
}  // namespace lang
