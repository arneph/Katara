//
//  types_util.cc
//  Katara
//
//  Created by Arne Philipeit on 9/27/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "types_util.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace lang {
namespace types {

std::string TypeInfoToText(pos::FileSet *file_set, TypeInfo *type_info) {
    std::stringstream ss;
    
    {
        ss << "Types:\n";
        size_t max_pos = 0;
        size_t max_expr = 0;
        size_t max_type = 0;
        for (auto& [expr, type] : type_info->types()) {
            pos::Position pos = file_set->PositionFor(expr->start());
            max_pos = std::max(max_pos, pos.ToString().size());
            max_expr = std::max(max_expr, size_t(expr->end() - expr->start() + 1));
            max_type = std::max(max_type, type->ToString().size());
        }
        for (auto& [expr, type] : type_info->types()) {
            pos::Position pos = file_set->PositionFor(expr->start());
            pos::File *file = file_set->FileAt(expr->start());
            
            ss << std::setw(int(max_pos)) << pos.ToString() << " ";
            ss << std::setw(int(max_expr)) << file->contents(expr->start(), expr->end()) << " ";
            ss << std::setw(int(max_type)) << type->ToString() << "\n";
        }
        ss << "\n";
    }
    
    {
        ss << "Constant Values:\n";
        size_t max_pos = 0;
        size_t max_expr = 0;
        size_t max_value = 0;
        for (auto& [expr, value] : type_info->constant_values()) {
            pos::Position pos = file_set->PositionFor(expr->start());
            max_pos = std::max(max_pos, pos.ToString().size());
            max_expr = std::max(max_expr, size_t(expr->end() - expr->start() + 1));
            max_value = std::max(max_value, value.ToString().size());
        }
        for (auto& [expr, value] : type_info->constant_values()) {
            pos::Position pos = file_set->PositionFor(expr->start());
            pos::File *file = file_set->FileAt(expr->start());
            
            ss << std::setw(int(max_pos)) << pos.ToString() << " ";
            ss << std::setw(int(max_expr)) << file->contents(expr->start(), expr->end()) << " ";
            ss << std::setw(int(max_value)) << value.ToString() << "\n";
        }
        ss << "\n";
    }
    
    {
        ss << "Definitions:\n";
        size_t max_pos = 0;
        size_t max_ident = 0;
        size_t max_obj = 0;
        for (auto& [ident, obj] : type_info->definitions()) {
            pos::Position pos = file_set->PositionFor(ident->start());
            max_pos = std::max(max_pos, pos.ToString().size());
            max_ident = std::max(max_ident, ident->name_.size());
            max_obj = std::max(max_obj, obj->ToString().size());
        }
        for (auto& [ident, obj] : type_info->definitions()) {
            pos::Position pos = file_set->PositionFor(ident->start());
            
            ss << std::setw(int(max_pos)) << pos.ToString() << " ";
            ss << std::setw(int(max_ident)) << ident->name_ << " ";
            ss << std::setw(int(max_obj)) << obj->ToString() << "\n";
        }
        ss << "\n";
    }
    
    {
        ss << "Uses:\n";
        size_t max_pos = 0;
        size_t max_ident = 0;
        size_t max_obj = 0;
        for (auto& [ident, obj] : type_info->uses()) {
            pos::Position pos = file_set->PositionFor(ident->start());
            max_pos = std::max(max_pos, pos.ToString().size());
            max_ident = std::max(max_ident, ident->name_.size());
            max_obj = std::max(max_obj, obj->ToString().size());
        }
        for (auto& [ident, obj] : type_info->uses()) {
            pos::Position pos = file_set->PositionFor(ident->start());
            
            ss << std::setw(int(max_pos)) << pos.ToString() << " ";
            ss << std::setw(int(max_ident)) << ident->name_ << " ";
            ss << std::setw(int(max_obj)) << obj->ToString() << "\n";
        }
        ss << "\n";
    }
    
    {
        ss << "Implicits:\n";
        size_t max_pos = 0;
        size_t max_obj = 0;
        for (auto& [node, obj] : type_info->implicits()) {
            pos::Position pos = file_set->PositionFor(node->start());
            max_pos = std::max(max_pos, pos.ToString().size());
            max_obj = std::max(max_obj, obj->ToString().size());
        }
        for (auto& [node, obj] : type_info->implicits()) {
            pos::Position pos = file_set->PositionFor(node->start());
            
            ss << std::setw(int(max_pos)) << pos.ToString() << " ";
            ss << std::setw(int(max_obj)) << obj->ToString() << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}

}
}
