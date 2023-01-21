//
//  value_builder.h
//  Katara
//
//  Created by Arne Philipeit on 7/25/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_builder_value_builder_h
#define lang_ir_builder_value_builder_h

#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/lang/processors/ir/builder/context.h"
#include "src/lang/processors/ir/builder/type_builder.h"
#include "src/lang/representation/ast/nodes.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"
#include "src/lang/representation/types/info.h"

namespace lang {
namespace ir_builder {

class ValueBuilder {
 public:
  ValueBuilder(TypeBuilder& type_builder) : type_builder_(type_builder) {}

  std::shared_ptr<ir::Computed> BuildBoolNot(std::shared_ptr<ir::Value> x, IRContext& ir_ctx);
  std::shared_ptr<ir::Computed> BuildBoolBinaryOp(std::shared_ptr<ir::Value> x,
                                                  common::atomics::Bool::BinaryOp op,
                                                  std::shared_ptr<ir::Value> y, IRContext& ir_ctx);

  std::shared_ptr<ir::Computed> BuildIntUnaryOp(common::atomics::Int::UnaryOp op,
                                                std::shared_ptr<ir::Value> x, IRContext& ir_ctx);
  std::shared_ptr<ir::Computed> BuildIntBinaryOp(std::shared_ptr<ir::Value> x,
                                                 common::atomics::Int::BinaryOp op,
                                                 std::shared_ptr<ir::Value> y, IRContext& ir_ctx);
  std::shared_ptr<ir::Computed> BuildIntCompareOp(std::shared_ptr<ir::Value> x,
                                                  common::atomics::Int::CompareOp op,
                                                  std::shared_ptr<ir::Value> y, IRContext& ir_ctx);
  std::shared_ptr<ir::Computed> BuildIntShiftOp(std::shared_ptr<ir::Value> x,
                                                common::atomics::Int::ShiftOp op,
                                                std::shared_ptr<ir::Value> y, IRContext& ir_ctx);

  std::shared_ptr<ir::Computed> BuildStringConcat(std::shared_ptr<ir::Value> x,
                                                  std::shared_ptr<ir::Value> y, IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildStringComparison(std::shared_ptr<ir::Value> x, tokens::Token op,
                                                   std::shared_ptr<ir::Value> y, IRContext& ir_ctx);

  std::shared_ptr<ir::Value> BuildConversion(std::shared_ptr<ir::Value> value,
                                             const ir::Type* desired_type, IRContext& ir_ctx);

  std::shared_ptr<ir::Value> BuildDefaultForType(types::Type* type);
  std::shared_ptr<ir::Value> BuildConstant(constants::Value value) const;

 private:
  TypeBuilder& type_builder_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_value_builder_h */
