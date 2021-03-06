#ifndef LAZY_ENVIRONMENT_H
#define LAZY_ENVIRONMENT_H

#include "../ir/BC_inc.h"
#include "../runtime/RirRuntimeObject.h"
#include "instance.h"
#include "interp_incl.h"

#include <cassert>
#include <cstdint>
#include <functional>

namespace rir {

#define LAZY_ENVIRONMENT_MAGIC 0xe4210e47

/**
 * EnvironmentStub holds the information needed to create an
 * environment lazily.
 */
struct LazyEnvironment
    : public RirRuntimeObject<LazyEnvironment, LAZY_ENVIRONMENT_MAGIC> {
    LazyEnvironment() = delete;
    LazyEnvironment(const LazyEnvironment&) = delete;
    LazyEnvironment& operator=(const LazyEnvironment&) = delete;

    LazyEnvironment(SEXP parent, Immediate* names, size_t nargs, void* frameEnd,
                    InterpreterInstance* ctx)
        : RirRuntimeObject(sizeof(LazyEnvironment), nargs + 1),
          frameEnd(frameEnd), nargs(nargs), names(names) {
        setEntry(0, parent);
        for (long i = nargs - 1; i >= 0; --i) {
            setArg(i, ostack_pop(ctx));
        }
    }

    void* frameEnd;
    size_t nargs;
    Immediate* names;

    SEXP getArg(size_t i) { return getEntry(i + 1); }
    void setArg(size_t i, SEXP val) { setEntry(i + 1, val); }

    SEXP getParent() { return getEntry(0); }
};
} // namespace rir

#endif
