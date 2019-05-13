#pragma once

#include "pir.h"
#include <functional>
#include <sstream>
#include <unordered_map>

namespace rir {
namespace pir {

// Remember to update ClosureProperty::LAST
#define LIST_OF_CLOSURE_PROPERTIES(V)                                          \
    V(IsEager)                                                                 \
    V(NoReflection)

enum class ClosureProperty {
#define V(Prop) Prop,
    LIST_OF_CLOSURE_PROPERTIES(V)
#undef V

        FIRST = IsEager,
    LAST = NoReflection
};

struct ClosureProperties : public EnumSet<ClosureProperty> {
    ClosureProperties()
        : EnumSet<ClosureProperty>(), returnType(PirType::any()){};
    explicit ClosureProperties(const EnumSet<ClosureProperty>& other)
        : EnumSet<ClosureProperty>(other), returnType(PirType::any()) {}
    explicit ClosureProperties(const ClosureProperty& other)
        : EnumSet<ClosureProperty>(other), returnType(PirType::any()) {}

    std::vector<size_t> argumentForceOrder;
    PirType returnType;

    ClosureProperties operator|(const ClosureProperties&) const;
    friend std::ostream& operator<<(std::ostream& out,
                                    const ClosureProperties&);
};

std::ostream& operator<<(std::ostream& out, const ClosureProperty&);

} // namespace pir
} // namespace rir
