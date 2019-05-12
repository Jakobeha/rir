#include "closure_property.h"
#include "../transform/bb.h"
#include "../util/visitor.h"
#include "closure.h"
#include "pir_impl.h"

#include <iostream>

namespace rir {
namespace pir {

ClosureProperties ClosureProperties::
operator|(const ClosureProperties& o) const {
    ClosureProperties res((EnumSet<ClosureProperty>)*this |
                          (EnumSet<ClosureProperty>)o);
    if (o.argumentForceOrder.size() > 0)
        res.argumentForceOrder = o.argumentForceOrder;
    else
        res.argumentForceOrder = argumentForceOrder;
    if (o.returnType.isA(returnType))
        res.returnType = o.returnType;
    else
        res.returnType = returnType;
    return res;
}

std::ostream& operator<<(std::ostream& out, const ClosureProperty& p) {
    switch (p) {
    case ClosureProperty::IsEager:
        out << "Eager";
        break;
    case ClosureProperty::NoReflection:
        out << "!Reflection";
        break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const ClosureProperties& props) {
    for (auto p = props.begin(); p != props.end(); ++p) {
        out << *p;
        if ((p + 1) != props.end())
            out << ", ";
    }
    if (props.argumentForceOrder.size() > 0) {
        if (!props.empty())
            out << ", ";
        out << "ForceOrd: ";
        for (auto o = props.argumentForceOrder.begin();
             o != props.argumentForceOrder.end(); ++o) {
            out << *o;
            if ((o + 1) != props.argumentForceOrder.end())
                out << " ";
        }
    }
    if (props.returnType != PirType::any()) {
        if (!props.empty() || props.argumentForceOrder.size() > 0)
            out << ", ";
        out << "ReturnType: " << props.returnType;
    }
    return out;
}

} // namespace pir
} // namespace rir
