//
//  info_util.cc
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "info_util.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "lang/representation/constants/constants.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/scope.h"
#include "lang/representation/types/package.h"
#include "lang/representation/types/initializer.h"

namespace lang {
namespace types {

void TypesToText(pos::FileSet *file_set,
                 Info *info,
                 std::stringstream& ss);
void ConstantExpressionsToText(pos::FileSet *file_set,
                               Info *info,
                               std::stringstream& ss);
void ConstantsToText(pos::FileSet *file_set,
                     Info *info,
                     std::stringstream& ss);
void DefinitionsToText(pos::FileSet *file_set,
                       Info *info,
                       std::stringstream& ss);
void UsesToText(pos::FileSet *file_set,
                Info *info,
                std::stringstream& ss);
void ImplicitsToText(pos::FileSet *file_set,
                     Info *info,
                     std::stringstream& ss);

std::string InfoToText(pos::FileSet *file_set, Info *info) {
    std::stringstream ss;
    
    TypesToText(file_set, info, ss);
    ConstantExpressionsToText(file_set, info, ss);
    ConstantsToText(file_set, info, ss);
    DefinitionsToText(file_set, info, ss);
    UsesToText(file_set, info, ss);
    ImplicitsToText(file_set, info, ss);
    
    return ss.str();
}

void TypesToText(pos::FileSet *file_set,
                 Info *info,
                 std::stringstream& ss) {
    ss << "Types:\n";
    size_t max_pos = 0;
    size_t max_expr = 0;
    size_t max_type = 0;
    for (auto& [expr, expr_info] : info->expr_infos()) {
        if (expr_info.type() == nullptr) continue;
        pos::Position pos = file_set->PositionFor(expr->start());
        max_pos = std::max(max_pos, pos.ToString().size());
        max_expr = std::max(max_expr, size_t(expr->end() - expr->start() + 1));
        max_type = std::max(max_type, expr_info.type()->ToString(StringRep::kExpanded).size());
    }
    for (auto& [expr, expr_info] : info->expr_infos()) {
        if (expr_info.type() == nullptr) continue;
        pos::Position pos = file_set->PositionFor(expr->start());
        pos::File *file = file_set->FileAt(expr->start());
        
        ss << std::setw(int(max_pos)) << std::left << pos.ToString() << " ";
        ss << std::setw(int(max_expr)) << std::left
           << file->contents(expr->start(), expr->end()) << " ";
        ss << std::setw(int(max_type)) << std::left
           << expr_info.type()->ToString(StringRep::kExpanded) << "\n";
    }
    ss << "\n";
}

void ConstantExpressionsToText(pos::FileSet *file_set,
                               Info *info,
                               std::stringstream& ss) {
    ss << "Constant Expressions:\n";
    size_t max_pos = 0;
    size_t max_expr = 0;
    size_t max_value = 0;
    for (auto& [expr, value] : info->constant_values()) {
        pos::Position pos = file_set->PositionFor(expr->start());
        max_pos = std::max(max_pos, pos.ToString().size());
        max_expr = std::max(max_expr, size_t(expr->end() - expr->start() + 1));
        max_value = std::max(max_value, value.ToString().size());
    }
    for (auto& [expr, value] : info->constant_values()) {
        pos::Position pos = file_set->PositionFor(expr->start());
        pos::File *file = file_set->FileAt(expr->start());
        
        ss << std::setw(int(max_pos)) << std::left << pos.ToString() << " ";
        ss << std::setw(int(max_expr)) << std::left
        << file->contents(expr->start(), expr->end()) << " ";
        ss << std::setw(int(max_value)) << std::left << value.ToString() << "\n";
    }
    ss << "\n";
}

void ConstantsToText(pos::FileSet *file_set,
                     Info *info,
                     std::stringstream& ss) {
    ss << "Constants:\n";
    size_t max_pos = 0;
    size_t max_ident = 0;
    size_t max_value = 0;
    for (auto& [ident, obj] : info->definitions()) {
        if (obj->object_kind() != ObjectKind::kConstant) continue;
        Constant *constant = static_cast<Constant *>(obj);
        pos::Position pos = file_set->PositionFor(ident->start());
        max_pos = std::max(max_pos, pos.ToString().size());
        max_ident = std::max(max_ident, ident->name().size());
        max_value = std::max(max_value, constant->value().ToString().size());
    }
    for (auto& [expr, value] : info->constant_values()) {
        pos::Position pos = file_set->PositionFor(expr->start());
        pos::File *file = file_set->FileAt(expr->start());
        
        ss << std::setw(int(max_pos)) << std::left << pos.ToString() << " ";
        ss << std::setw(int(max_ident)) << std::left
           << file->contents(expr->start(), expr->end()) << " ";
        ss << std::setw(int(max_value)) << std::left << value.ToString() << "\n";
    }
    ss << "\n";
}

void DefinitionsToText(pos::FileSet *file_set,
                       Info *info,
                       std::stringstream& ss) {
    ss << "Definitions:\n";
    size_t max_pos = 0;
    size_t max_ident = 0;
    size_t max_obj = 0;
    for (auto& [ident, obj] : info->definitions()) {
        pos::Position pos = file_set->PositionFor(ident->start());
        max_pos = std::max(max_pos, pos.ToString().size());
        max_ident = std::max(max_ident, ident->name().size());
        max_obj = std::max(max_obj, obj->ToString().size());
    }
    for (auto& [ident, obj] : info->definitions()) {
        pos::Position pos = file_set->PositionFor(ident->start());
        
        ss << std::setw(int(max_pos)) << std::left << pos.ToString() << " ";
        ss << std::setw(int(max_ident)) << std::left << ident->name() << " ";
        ss << std::setw(int(max_obj)) << std::left << obj->ToString() << "\n";
    }
    ss << "\n";
}

void UsesToText(pos::FileSet *file_set,
                Info *info,
                std::stringstream& ss) {
    ss << "Uses:\n";
    size_t max_pos = 0;
    size_t max_ident = 0;
    size_t max_obj = 0;
    for (auto& [ident, obj] : info->uses()) {
        pos::Position pos = file_set->PositionFor(ident->start());
        max_pos = std::max(max_pos, pos.ToString().size());
        max_ident = std::max(max_ident, ident->name().size());
        max_obj = std::max(max_obj, obj->ToString().size());
    }
    for (auto& [ident, obj] : info->uses()) {
        pos::Position pos = file_set->PositionFor(ident->start());
        
        ss << std::setw(int(max_pos)) << std::left << pos.ToString() << " ";
        ss << std::setw(int(max_ident)) << std::left << ident->name() << " ";
        ss << std::setw(int(max_obj)) << std::left << obj->ToString() << "\n";
    }
    ss << "\n";
}

void ImplicitsToText(pos::FileSet *file_set,
                     Info *info,
                     std::stringstream& ss) {
    ss << "Implicits:\n";
    size_t max_pos = 0;
    size_t max_obj = 0;
    for (auto& [node, obj] : info->implicits()) {
        pos::Position pos = file_set->PositionFor(node->start());
        max_pos = std::max(max_pos, pos.ToString().size());
        max_obj = std::max(max_obj, obj->ToString().size());
    }
    for (auto& [node, obj] : info->implicits()) {
        pos::Position pos = file_set->PositionFor(node->start());
        
        ss << std::setw(int(max_pos)) << std::left << pos.ToString() << " ";
        ss << std::setw(int(max_obj)) << std::left << obj->ToString() << "\n";
    }
    ss << "\n";
}

}
}
