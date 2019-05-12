#include "closure_version.h"
#include "../transform/bb.h"
#include "../util/visitor.h"
#include "closure.h"
#include "pir_impl.h"

#include <iostream>

namespace rir {
namespace pir {

void ClosureVersion::print(std::ostream& out, bool tty) const {
    print(DebugStyle::Standard, out, tty, false);
}

void ClosureVersion::print(DebugStyle style, std::ostream& out, bool tty,
                           bool omitDeoptBranches) const {
    switch (style) {
    case DebugStyle::Standard:
        printStandard(out, tty, omitDeoptBranches);
        break;
    case DebugStyle::GraphViz:
        printGraph(out, omitDeoptBranches);
        break;
    case DebugStyle::GraphVizBB:
        printBBGraph(out, omitDeoptBranches);
        break;
    default:
        assert(false);
    }
}

void ClosureVersion::printStandard(std::ostream& out, bool tty,
                                   bool omitDeoptBranches) const {
    out << *this << "\n";
    printCode(out, tty, omitDeoptBranches);
    for (auto p : promises_) {
        if (p) {
            out << "Prom " << p->id << ":\n";
            p->printCode(out, tty, omitDeoptBranches);
        }
    }
}

void ClosureVersion::printGraph(std::ostream& out,
                                bool omitDeoptBranches) const {
    out << "digraph {\n";
    out << "label=\"" << *this << "\";\n";
    printGraphCode(out, omitDeoptBranches);
    for (auto p : promises_) {
        if (p) {
            out << "subgraph p" << p->id << "{\n";
            out << "label = \"Promise " << p->id << "\";\n";
            p->printGraphCode(out, omitDeoptBranches);
            out << "}\n";
        }
    }
    out << "}\n";
}

void ClosureVersion::printBBGraph(std::ostream& out,
                                  bool omitDeoptBranches) const {
    out << "digraph {\n";
    out << "label=\"" << *this << "\";\n";
    printBBGraphCode(out, omitDeoptBranches);
    for (auto p : promises_) {
        if (p) {
            out << "subgraph {\n";
            out << "label=\"Promise " << p->id << "\";\n";
            p->printBBGraphCode(out, omitDeoptBranches);
            out << "}\n";
        }
    }
    out << "}\n";
}

Promise* ClosureVersion::createProm(unsigned srcPoolIdx) {
    Promise* p = new Promise(this, promises_.size(), srcPoolIdx);
    promises_.push_back(p);
    return p;
}

ClosureVersion::~ClosureVersion() {
    for (auto p : promises_) {
        if (p)
            delete p;
    }
}

ClosureVersion* ClosureVersion::clone(const Assumptions& newAssumptions) {
    auto ctx = optimizationContext_;
    ctx.assumptions = ctx.assumptions | newAssumptions;
    auto c = owner_->declareVersion(ctx);
    c->entry = BBTransform::clone(entry, c, c);
    return c;
}

void ClosureVersion::erasePromise(unsigned id) {
    assert(promises_.at(id) && "Promise already deleted");

    // If we delete a corrupt promise it get's hard to debug...
    assert(promises_.at(id)->owner == this);
    assert(promise(promises_.at(id)->id) == promises_.at(id));

    delete promises_[id];
    promises_[id] = nullptr;
}

size_t ClosureVersion::size() const {
    size_t s = 0;
    eachPromise([&s](Promise* p) { s += p->size(); });
    return s + Code::size();
}

size_t ClosureVersion::nargs() const { return owner_->nargs(); }

ClosureVersion::ClosureVersion(Closure* closure,
                               const OptimizationContext& optimizationContext)
    : owner_(closure), optimizationContext_(optimizationContext) {
    auto id = std::stringstream();
    id << closure->name() << "[" << this << "]";
    name_ = id.str();
    id.str("");
    id << this;
    nameSuffix_ = id.str();
}

} // namespace pir
} // namespace rir
