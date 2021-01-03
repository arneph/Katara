//
//  expr_info.h
//  Katara
//
//  Created by Arne Philipeit on 1/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_expr_info_h
#define lang_types_expr_info_h

#include "lang/representation/types/types.h"

namespace lang {
namespace types {

enum class ExprKind {
    kInvalid,
    kNoValue,
    kBuiltin,
    kType,
    kConstant,
    kVariable,
    kValue,
    kValueOk,
};

// TODO: add optional constant value
class ExprInfo {
public:
    ExprInfo(ExprKind kind, Type *type) : kind_(kind), type_(type) {}
    
    ExprKind kind() const { return kind_; }
    Type * type() const { return type_; }

private:
    ExprKind kind_;
    Type *type_;
};

}
}

#endif /* lang_types_expr_info_h */
