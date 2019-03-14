#ifndef COMPILER_SIGNATURE_H
#define COMPILER_SIGNATURE_H

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "type.h"

namespace rir {
namespace pir {

/**
 * Function type signatures. PIR can specialize certain instructions' types
 * if the instruction argument types match the signature. This checking is done
 * in Instruction::updateTypeFromSig.
 *
 * Signatures are implemented as a vector of overloads, with fixed argument
 * types and a result type. Earlier overloads have priority - if an ealier
 * overload is resolved a later overload should never specialize further.
 *
 * Additionally, if specScalar is true and every argument is a scalar, the
 * result type will also be a scalar.
 */
struct PirSignature {
    struct Overload {
        std::vector<PirType> args;
        PirType type;

        Overload(std::vector<PirType> args, PirType type)
            : args(args), type(type) {}
        Overload(PirType arg1, PirType type) : args({arg1}), type(type) {}
        Overload(PirType arg1, PirType arg2, PirType type)
            : args({arg1, arg2}), type(type) {}
    };

    std::vector<Overload> overloads;
    // If true and every argument is a scalar, the result is also a scalar
    bool specScalar;

    PirSignature(std::vector<Overload> overloads, bool specScalar)
        : overloads(overloads), specScalar(specScalar) {}

    // Never specializes
    static PirSignature none() { return PirSignature({}, false); }
    // Int Int -> Int, Int|Real Int|Real -> Real
    static PirSignature numBinop() {
        std::vector<Overload> overloads = {
            Overload(PirType::optimistic() | RType::integer,
                     PirType::optimistic() | RType::integer,
                     PirType::optimistic() | RType::integer),
            Overload(PirType::optimistic() | RType::integer | RType::real,
                     PirType::optimistic() | RType::integer | RType::real,
                     PirType::optimistic() | RType::real)};
        return PirSignature(overloads, true);
    }
    // Int|Real Int|Real -> Real
    static PirSignature realBinop() {
        std::vector<Overload> overloads = {
            Overload(PirType::optimistic() | RType::integer | RType::real,
                     PirType::optimistic() | RType::integer | RType::real,
                     PirType::optimistic() | RType::real)};
        return PirSignature(overloads, true);
    }
    // Lgl Lgl -> Lgl, Int|Real Int|Real -> Lgl
    static PirSignature lglBinop() {
        std::vector<Overload> overloads = {
            Overload(PirType::optimistic() | RType::logical,
                     PirType::optimistic() | RType::logical,
                     PirType::optimistic() | RType::logical),
            Overload(PirType::optimistic() | RType::integer | RType::real,
                     PirType::optimistic() | RType::integer | RType::real,
                     PirType::optimistic() | RType::logical)};
        return PirSignature(overloads, true);
    }
    // Int -> Int, Real -> Real
    static PirSignature numUnop() {
        std::vector<Overload> overloads = {
            Overload(PirType::optimistic() | RType::integer,
                     PirType::optimistic() | RType::integer),
            Overload(PirType::optimistic() | RType::real,
                     PirType::optimistic() | RType::real)};
        return PirSignature(overloads, true);
    }
    // Int|Real|Lgl -> Lgl
    static PirSignature lglUnop() {
        std::vector<Overload> overloads = {
            Overload(PirType::optimistic() | RType::integer | RType::real |
                         RType::logical,
                     PirType::optimistic() | RType::logical)};
        return PirSignature(overloads, true);
    }
};

} // namespace pir
} // namespace rir

#endif
