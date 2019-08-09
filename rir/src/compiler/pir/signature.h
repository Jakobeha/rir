#pragma once

#include "type.h"
#include <vector>

namespace rir {
namespace pir {

/**
 * A function signature for PIR function assertions.
 */
struct PirSignature {
  private:
    enum class Form { Regular, Any, Nothing };

    const Form form;

    PirSignature(Form form) : args(), result(), form(form) {}

  public:
    static PirSignature any();
    static PirSignature nothing();
    static PirSignature parse(const std::string& inp);

    const std::vector<PirType> args;
    const PirType result;

    PirSignature(std::vector<PirType> args, PirType result)
        : args(args), result(result), form(Form::Regular) {}
    bool accepts(std::vector<PirType> inArgs) const;
    bool isAny() const { return form == Form::Any; }
    bool isNothing() const { return form == Form::Nothing; }
    PirSignature operator&(PirSignature& other);

    std::ostream& operator<<(std::ostream& out);
};

} // namespace pir
} // namespace rir