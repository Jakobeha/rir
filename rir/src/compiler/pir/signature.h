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
    static const unsigned MAX_NUM_ARGS = 4;

    static PirSignature any();
    static PirSignature nothing();
    static PirSignature parse(const std::string& inp);

    const PirType args[MAX_NUM_ARGS];
    const unsigned numArgs;
    const PirType result;

    PirSignature(std::vector<PirType> args, PirType result);
    bool accepts(std::vector<PirType> inArgs) const;
    bool isAny() const { return form == Form::Any; }
    bool isNothing() const { return form == Form::Nothing; }
    PirSignature operator|(const PirSignature& other) const;

    std::ostream& operator<<(std::ostream& out);
};

} // namespace pir
} // namespace rir