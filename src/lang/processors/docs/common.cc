//
//  common.cc
//  Katara
//
//  Created by Arne Philipeit on 1/2/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "common.h"

#include <sstream>

namespace lang {
namespace docs {

namespace html {

std::string Escape(std::string text) {
    std::ostringstream ss;
    for (char c : text) {
        if (c == '\n') {
            ss << "<br/>\n";
        } else if (c == ' ') {
            ss << "&nbsp;";
        } else if (c <= 63) {
            ss << "&#" << std::to_string(c) << ";";
        } else {
            ss << c;
        }
    }
    return ss.str();
}

std::string TagsForText(std::string text,
                        TextFormat format,
                        std::string id,
                        std::string classs,
                        std::optional<GroupLink> link) {
    std::ostringstream ss;
    if (link.has_value()) {
        ss << "<a href=\"" << link.value().link << "\" "
           << "onmouseover=\""
           << "var xs = document.getElementsByClassName('" << link.value().group_class << "');"
           << "for (var i = 0; i < xs.length; i++) {"
           << "    xs.item(i).style.backgroundColor='whitesmoke';"
           << "} "
           << "document.getElementById('"
           << link.value().linked_id
           << "').style.backgroundColor='yellow';"
           << "\" "
           << "onmouseout=\""
           << "var xs = document.getElementsByClassName('" << link.value().group_class << "');"
           << "for (var i = 0; i < xs.length; i++) {"
           << "    xs.item(i).style.backgroundColor='white';"
           << "} "
           << "\" "
           << "style=\"text-decoration:none\">";
    }
    ss << "<span ";
    if (!id.empty()) {
        ss << "id=\"" << id << "\" ";
    }
    if (!classs.empty()) {
        ss << "class=\"" << classs << "\" ";
    }
    ss << "style=\"color:" << format.color << "\">";
    if (format.bold) ss << "<b>";
    ss << text;
    if (format.bold) ss << "</b>";
    ss << "</span>";
    if (link.has_value()) {
        ss << "</a>";
    }
    return ss.str();
}

}

html::TextFormat FormatForIdent(ast::Ident *ident,
                                types::Info *type_info) {
    types::Object *obj = type_info->ObjectOf(ident);
    if (obj == nullptr) {
        return formats::kDefault;
    } else if (obj->package() == nullptr) {
        return formats::kUniverse;
    }
    switch (obj->object_kind()) {
        case types::ObjectKind::kTypeName:
            switch (static_cast<types::TypeName *>(obj)->type()->type_kind()) {
                case types::TypeKind::kNamedType:
                    return formats::kNamedType;
                case types::TypeKind::kTypeParameter:
                    return formats::kTypeParameter;
                default:
                    return formats::kDefault;
            }
        case types::ObjectKind::kConstant:
            return formats::kConstant;
        case types::ObjectKind::kVariable:
            return formats::kVariable;
        case types::ObjectKind::kFunc:
            return formats::kFunc;
        case types::ObjectKind::kLabel:
            return formats::kLabel;
        case types::ObjectKind::kNil:
        case types::ObjectKind::kBuiltin:
            return formats::kUniverse;
        case types::ObjectKind::kPackageName:
            return formats::kPackageName;
    }
}

html::TextFormat FormatForToken(tokens::Token token) {
    switch (token) {
        case tokens::kInt:
        case tokens::kChar:
        case tokens::kString:
            return formats::kUniverse;
        case tokens::kConst:
        case tokens::kVar:
        case tokens::kType:
        case tokens::kInterface:
        case tokens::kStruct:
        case tokens::kIf:
        case tokens::kElse:
        case tokens::kFor:
        case tokens::kSwitch:
        case tokens::kCase:
        case tokens::kDefault:
        case tokens::kFallthrough:
        case tokens::kContinue:
        case tokens::kBreak:
        case tokens::kReturn:
        case tokens::kFunc:
        case tokens::kPackage:
        case tokens::kImport:
            return formats::kKeyword;
        default:
            return formats::kDefault;
    }
}

}
}
