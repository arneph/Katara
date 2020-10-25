//
//  ast.cc
//  Katara
//
//  Created by Arne Philipeit on 5/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "ast.h"

namespace lang {
namespace ast {

pos::pos_t File::start() const {
    return file_start_;
}

pos::pos_t File::end() const {
    return file_end_;
}

pos::pos_t GenDecl::start() const {
    return tok_start_;
}

pos::pos_t GenDecl::end() const {
    if (r_paren_ != pos::kNoPos) {
        return r_paren_;
    }
    return specs_.back()->end();
}

pos::pos_t ImportSpec::start() const {
    if (name_) {
        return name_->start();
    }
    return path_->start();
}

pos::pos_t ImportSpec::end() const {
    return path_->end();
}

pos::pos_t ValueSpec::start() const {
    return names_.front()->start();
}

pos::pos_t ValueSpec::end() const {
    if (!values_.empty()) {
        return values_.back()->end();
    }
    if (type_) {
        return type_->end();
    }
    return names_.back()->end();
}

pos::pos_t TypeSpec::start() const {
    return name_->start();
}

pos::pos_t TypeSpec::end() const {
    return type_->end();
}

pos::pos_t FuncDecl::start() const {
    return type_->start();
}

pos::pos_t FuncDecl::end() const {
    return body_->end();
}

pos::pos_t BlockStmt::start() const {
    return l_brace_;
}

pos::pos_t BlockStmt::end() const {
    return r_brace_;
}

pos::pos_t DeclStmt::start() const {
    return decl_->start();
}

pos::pos_t DeclStmt::end() const {
    return decl_->end();
}

pos::pos_t AssignStmt::start() const {
    return lhs_.front()->start();
}

pos::pos_t AssignStmt::end() const {
    return rhs_.back()->end();
}

pos::pos_t ExprStmt::start() const {
    return x_->start();
}

pos::pos_t ExprStmt::end() const {
    return x_->end();
}

pos::pos_t IncDecStmt::start() const {
    return x_->start();
}

pos::pos_t IncDecStmt::end() const {
    return tok_start_ + 1;
}

pos::pos_t ReturnStmt::start() const {
    return return_;
}

pos::pos_t ReturnStmt::end() const {
    if (!results_.empty()) {
        return results_.back()->end();
    }
    return return_ + 5;
}

pos::pos_t IfStmt::start() const {
    return if_;
}

pos::pos_t IfStmt::end() const {
    if (else_) {
        return else_->end();
    }
    return body_->end();
}

pos::pos_t SwitchStmt::start() const {
    return switch_;
}

pos::pos_t SwitchStmt::end() const {
    return body_->end();
}

pos::pos_t CaseClause::start() const {
    return tok_start_;
}

pos::pos_t CaseClause::end() const {
    if (!body_.empty()) {
        return body_.back()->end();
    }
    return colon_;
}

pos::pos_t ForStmt::start() const {
    return for_;
}

pos::pos_t ForStmt::end() const {
    return body_->end();
}

pos::pos_t LabeledStmt::start() const {
    return label_->start();
}

pos::pos_t LabeledStmt::end() const {
    return stmt_->end();
}

pos::pos_t BranchStmt::start() const {
    return tok_start_;
}

pos::pos_t BranchStmt::end() const {
    if (label_) {
        return label_->end();
    }
    switch (tok_) {
        case tokens::kFallthrough:
            return tok_start_ + 10;
        case tokens::kContinue:
            return tok_start_ + 7;
        case tokens::kBreak:
            return tok_start_ + 4;
        default:
            throw "unexpected ast::BranchStmt token";
    }
}

pos::pos_t UnaryExpr::start() const {
    return op_start_;
}

pos::pos_t UnaryExpr::end() const {
    return x_->end();
}

pos::pos_t BinaryExpr::start() const {
    return x_->start();
}

pos::pos_t BinaryExpr::end() const {
    return y_->end();
}

pos::pos_t ParenExpr::start() const {
    return l_paren_;
}

pos::pos_t ParenExpr::end() const {
    return r_paren_;
}

pos::pos_t SelectionExpr::start() const {
    return accessed_->start();
}

pos::pos_t SelectionExpr::end() const {
    return selection_->end();
}

pos::pos_t TypeAssertExpr::start() const {
    return x_->start();
}

pos::pos_t TypeAssertExpr::end() const {
    return r_angle_;
}

pos::pos_t IndexExpr::start() const {
    return accessed_->start();
}

pos::pos_t IndexExpr::end() const {
    return r_brack_;
}

pos::pos_t CallExpr::start() const {
    return func_->start();
}

pos::pos_t CallExpr::end() const {
    return r_paren_;
}

pos::pos_t KeyValueExpr::start() const {
    return key_->start();
}

pos::pos_t KeyValueExpr::end() const {
    return value_->end();
}

pos::pos_t FuncLit::start() const {
    return type_->start();
}

pos::pos_t FuncLit::end() const {
    return body_->end();
}

pos::pos_t CompositeLit::start() const {
    return type_->start();
}

pos::pos_t CompositeLit::end() const {
    return r_brace_;
}

pos::pos_t ArrayType::start() const {
    return l_brack_;
}

pos::pos_t ArrayType::end() const {
    return element_type_->end();
}

pos::pos_t FuncType::start() const {
    return func_;
}

pos::pos_t FuncType:: end() const {
    if (results_) {
        return results_->end();
    }
    return params_->end();
}

pos::pos_t InterfaceType::start() const {
    return interface_;
}

pos::pos_t InterfaceType::end() const {
    return r_brace_;
}

pos::pos_t MethodSpec::start() const {
    return name_->start();
}

pos::pos_t MethodSpec::end() const {
    if (!results_) {
        return params_->end();
    }
    return results_->end();
}

pos::pos_t StructType::start() const {
    return struct_;
}

pos::pos_t StructType::end() const {
    return r_brace_;
}

pos::pos_t TypeInstance::start() const {
    return type_->start();
}

pos::pos_t TypeInstance::end() const {
    if (!type_args_) {
        return type_->end();
    }
    return type_args_->end();
}

pos::pos_t FieldList::start() const {
    if (l_paren_ != pos::kNoPos) {
        return l_paren_;
    }
    return fields_.front()->start();
}

pos::pos_t FieldList::end() const {
    if (r_paren_ != pos::kNoPos) {
        return r_paren_;
    }
    return fields_.back()->end();
}

pos::pos_t Field::start() const {
    if (!names_.empty()) {
        return names_.front()->start();
    }
    return type_->start();
}

pos::pos_t Field::end() const {
    return type_->end();
}

pos::pos_t TypeArgList::start() const {
    return l_angle_;
}

pos::pos_t TypeArgList::end() const {
    return r_angle_;
}

pos::pos_t TypeParamList::start() const {
    return l_angle_;
}

pos::pos_t TypeParamList::end() const {
    return r_angle_;
}

pos::pos_t TypeParam::start() const {
    return name_->start();
}

pos::pos_t TypeParam::end() const {
    if (!type_) {
        return name_->end();
    }
    return type_->start();
}

pos::pos_t BasicLit::start() const {
    return value_start_;
}

pos::pos_t BasicLit::end() const {
    return value_start_ + value_.size() - 1;
}

pos::pos_t Ident::start() const {
    return name_start_;
}

pos::pos_t Ident::end() const {
    return name_start_ + name_.size() - 1;
}

}
}
