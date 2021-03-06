#ifndef COMPILER_PROMISE_H
#define COMPILER_PROMISE_H

#include "code.h"

namespace rir {
namespace pir {

class Promise : public Code {
  public:
    const unsigned id;
    ClosureVersion* owner;

    friend std::ostream& operator<<(std::ostream& out, const Promise& p) {
        out << "Prom(" << p.id << ")";
        return out;
    }

    unsigned srcPoolIdx() const;

  private:
    const unsigned srcPoolIdx_;
    friend class ClosureVersion;
    Promise(ClosureVersion* owner, unsigned id, unsigned src);
};

} // namespace pir
} // namespace rir

#endif
