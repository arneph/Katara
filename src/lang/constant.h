//
//  constant.h
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_constant_h
#define lang_constant_h

#include <memory>
#include <string>
#include <vector>

namespace lang {
namespace constant {

typedef enum : uint8_t {
    kBool,
    kInt,
} Kind;

class Value {
public:
    Value(Kind kind);
    Value(bool b);
    Value(uint64_t ui64);
    Value(int64_t i64);
    
    Kind kind() const;
    
    bool AsBool() const;
    bool IsPreciseUint64() const;
    uint64_t AsUint64() const;
    bool IsPreciseInt64() const;
    int64_t AsInt64() const;
    
    std::string ToString() const;
    
private:
    typedef bool Sign;
    static const Sign kPlus = false;
    static const Sign kMinus = true;
    
    Kind kind_;
    Sign sign_;
    uint64_t abs_;
};

}
}

#endif /* lang_constant_h */
