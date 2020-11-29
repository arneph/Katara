//
//  universe_builder.h
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_universe_builder_h
#define lang_type_checker_universe_builder_h

#include "lang/representation/types/types.h"

namespace lang {
namespace type_checker {

class UniverseBuilder {
public:
    static void SetupUniverse(types::TypeInfo *info);
    
private:
    static void SetupPredeclaredTypes(types::TypeInfo *info);
    static void SetupPredeclaredConstants(types::TypeInfo *info);
    static void SetupPredeclaredNil(types::TypeInfo *info);
    static void SetupPredeclaredFuncs(types::TypeInfo *info);
};

}
}

#endif /* lang_type_checker_universe_builder_h */
