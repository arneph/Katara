//
//  selection.h
//  Katara
//
//  Created by Arne Philipeit on 12/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_selection_h
#define lang_types_selection_h

#include "lang/representation/types/types.h"
#include "lang/representation/types/objects.h"

namespace lang {
namespace types {

class Selection {
public:
    enum class Kind {
        kFieldVal,
        kMethodVal,
        kMethodExpr,
    };
    
    ~Selection() {}
    
    Kind kind() const;
    Type * receiver_type() const;
    Type * type() const;
    Object * object() const;
    
private:
    Selection() {}
    
    Kind kind_;
    Type *receiver_type_;
    Type *type_;
    Object *object_;
    
    friend class InfoBuilder;
};

}
}

#endif /* lang_types_selection_h */
