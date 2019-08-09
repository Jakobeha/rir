#include "signature.h"
#include <regex>

namespace rir {
namespace pir {

PirSignature PirSignature::any() { return PirSignature(Form::Any); }

PirSignature PirSignature::nothing() { return PirSignature(Form::Nothing); }

PirSignature PirSignature::parse(const std::string& inp) {
    auto fail = []() {
        Rf_error("couldn't parse PIR signature: must be of the form <type>* -> "
                 "<type>");
    };

    std::smatch match;
    if (!std::regex_match(inp, match,
                          std::regex("^\\s*(.*)->\\s*([^ ]+)\\s*$")))
        fail();
    std::string argsInp = match[1];
    PirType retType = PirType::parse(match[2]);

    std::vector<PirType> argTypes;
    while (std::regex_search(argsInp, match, std::regex("([^ ]+)\\s*"))) {
        argTypes.push_back(PirType::parse(match[1]));
        argsInp = match.suffix();
    }

    if (!argsInp.empty())
        fail();

    return PirSignature(argTypes, retType);
}

bool PirSignature::accepts(std::vector<PirType> inArgs) const {
    switch (form) {
    case Form::Regular:
        if (args.size() != inArgs.size())
            return false;
        for (int i = 0; i < args.size(); i++) {
            PirType arg = args[i];
            PirType inArg = inArgs[i];
            if (!inArg.isA(arg))
                return false;
        }
        return true;
    case Form::Any:
        return true;
    case Form::Nothing:
        return false;
    default:
        assert(false);
        return false;
    }
}

PirSignature PirSignature::operator&(PirSignature& other) {
    if (isAny() || other.isNothing())
        return other;
    else if (isNothing() || other.isAny())
        return this;
    else if (args.size() != other.args().size())
        return PirSignature::nothing();
}

std::ostream& PirSignature::operator<<(std::ostream& out) {
    switch (form) {
    case Form::Regular:
        for (PirType arg : args) {
            out << arg << " ";
        }
        out << "-> " << result;
        break;
    case Form::Any:
        out << "[any]";
        break;
    case Form::Nothing:
        out << "[nothing]";
        break;
    default:
        assert(false);
        break;
    }
}

} // namespace pir
} // namespace rir