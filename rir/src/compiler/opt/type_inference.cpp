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

            if (LdVar::Cast(i) || Add::Cast(i)) {
                i->type = PirType::bottom();
            }

            /*auto before = analysis.at<ScopeAnalysis::BeforeInstruction>(i);
            auto after = analysis.at<ScopeAnalysis::AfterInstruction>(i);

            analysis.lookup(after, i, [&](const AbstractLoad& aLoad) {
                auto& res = aLoad.result;
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
