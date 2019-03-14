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

            // if (LdVar::Cast(i)) {
            //    if (i->type.maybePromiseWrapped()) {
            //        i->type = PirType::bottom().orPromiseWrapped();
            //    } else {
            //        i->type = PirType::bottom();
            //    }
            // i->type = i->type | RType::real;
            //}

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
