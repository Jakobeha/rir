#include "../analysis/query.h"
#include "../analysis/scope.h"
#include "../pir/pir_impl.h"
#include "../util/cfg.h"
#include "../util/phi_placement.h"
#include "../util/visitor.h"
#include "R/r.h"
#include "pass_definitions.h"

#include <algorithm>
#include <unordered_map>

namespace rir {
namespace pir {

void TypeInference::apply(RirCompiler&, ClosureVersion* function,
                          LogStream& log) const {
    // ScopeAnalysis analysis(function, log);
    // analysis();
    // auto& finalState = analysis.result();

    Visitor::run(function->entry, [&](BB* bb) {
        auto ip = bb->begin();
        while (ip != bb->end()) {
            Instruction* i = *ip;
            auto next = ip + 1;

            if (Add::Cast(i)) {
                i->type = PirType::bottom();

                // Unbox arguments if they're boxed scalars
                // TODO: Move this into separate phase?
                i->eachArg([&](InstrArg& arg) {
                    if (arg.val()->type.isA(PirType::unboxableNum()) &&
                        arg.val()->type.isBoxed()) {
                        auto unbox = new Unbox(arg.val());
                        ip = bb->insert(ip, unbox) + 1;
                        next = ip + 1;
                        arg.val() = unbox;
                        arg.type() = arg.val()->type.unboxed();
                    }
                });
            }

            if (LdVar::Cast(i)) {
                if (i->type.maybePromiseWrapped()) {
                    i->type = PirType::bottom().boxed().orPromiseWrapped();
                } else {
                    i->type = PirType::bottom().boxed();
                }
            }

            /*auto before = analysis.at<ScopeAnalysis::BeforeInstruction>(i);
            auto after = analysis.at<ScopeAnalysis::AfterInstruction>(i);

            analysis.lookup(after, i, [&](const AbstractLoad& aLoad) {
                auto& res = aLoad.result;

                if (!res.isUnknown()) {

                    res.print(std::cout);
                    std::cout << "\n";
                    bool notArg = res.checkEachSource([&](const ValOrig& src) {
                        return !LdArg::Cast(src.val);
                    });
                    if (!notArg) {
                        std::cout << ":)\n";
                        i->type = i->type.orPromiseWrapped();
                    }
                }
                // Narrow down type according to what the analysis reports
                if (i->type.isRType()) {
                    auto inferedType = res.type;
                    if (!i->type.isA(inferedType))
                        i->type = inferedType;
                }
            });*/

            ip = next;
        }
    });
}

} // namespace pir
} // namespace rir
