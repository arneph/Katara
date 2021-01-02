//
//  common.h
//  Katara
//
//  Created by Arne Philipeit on 1/2/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_docs_common_h
#define lang_docs_common_h

#include <optional>
#include <string>

#include "lang/representation/positions/positions.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/types/info.h"

namespace lang {
namespace docs {

namespace html {

struct TextFormat {
    std::string_view color;
    bool bold;
};

struct GroupLink {
    std::string link;
    std::string linked_id;
    std::string group_class;
};

std::string Escape(std::string text);
std::string TagsForText(std::string text,
                        TextFormat format,
                        std::string id = "",
                        std::string classs = "",
                        std::optional<GroupLink> link = std::nullopt);

}

namespace formats {

constexpr html::TextFormat kDefault {
    .color = "black",
    .bold = false,
};

constexpr html::TextFormat kKeyword {
    .color = "crimson",
    .bold = false,
};

constexpr html::TextFormat kUniverse {
    .color = "blue",
    .bold = false,
};

constexpr html::TextFormat kNamedType {
    .color = "forestgreen",
    .bold = false,
};

constexpr html::TextFormat kTypeParameter {
    .color = "seagreen",
    .bold = false,
};

constexpr html::TextFormat kConstant {
    .color = "royalblue",
    .bold = false,
};

constexpr html::TextFormat kVariable {
    .color = "black",
    .bold = false,
};

constexpr html::TextFormat kFunc {
    .color = "blueviolet",
    .bold = false,
};

constexpr html::TextFormat kLabel {
    .color = "black",
    .bold = false,
};

constexpr html::TextFormat kPackageName {
    .color = "darkgray",
    .bold = false,
};

constexpr html::TextFormat kWarning {
    .color = "yellow",
    .bold = true,
};

constexpr html::TextFormat kError {
    .color = "red",
    .bold = true,
};

}

html::TextFormat FormatForIdent(ast::Ident *ident,
                                types::Info *type_info);

html::TextFormat FormatForToken(tokens::Token token);

}
}

#endif /* lang_docs_common_h */
