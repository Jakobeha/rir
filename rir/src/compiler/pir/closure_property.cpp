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
    res.signature = signature & o.signature;
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
    if (!props.signature.isAny) {
        if (!props.empty() || props.argumentForceOrder.size() > 0)
            out << ", ";
        out << "Signature: " << props.signature;
    }
    return out;
}

} // namespace pir
} // namespace rir
