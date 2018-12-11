#include "../pir/pir_impl.h"
#include "../transform/bb.h"
#include "../translations/rir_compiler.h"
#include "../util/cfg.h"
#include "../util/visitor.h"
#include "R/Symbols.h"
#include "pass_definitions.h"

#include <unordered_set>

namespace {

using namespace rir::pir;

static LdConst* isConst(Value* instr) {
    if (auto cst = LdConst::Cast(instr->followCastsAndForce())) {
        return cst;
    }
    return nullptr;
}

#define FOLD(Instruction, Operation)                                           \
    do {                                                                       \
        if (auto instr = Instruction::Cast(i)) {                               \
            if (auto lhs = isConst(instr->arg<0>().val())) {                   \
                if (auto rhs = isConst(instr->arg<1>().val())) {               \
                    auto res = Rf_eval(Rf_lang3(Operation, lhs->c, rhs->c),    \
                                       R_BaseEnv);                             \
                    cmp.preserve(res);                                         \
                    auto resi = new LdConst(res);                              \
                    instr->replaceUsesWith(resi);                              \
                    bb->replace(ip, resi);                                     \
                }                                                              \
            }                                                                  \
        }                                                                      \
    } while (false)

#define FOLD2(__i__, Instruction, Operation)                                   \
    do {                                                                       \
        if (auto instr = Instruction::Cast(__i__)) {                           \
            if (auto lhs = isConst(instr->arg<0>().val())) {                   \
                if (auto rhs = isConst(instr->arg<1>().val())) {               \
                    Operation(lhs->c, rhs->c);                                 \
                }                                                              \
            }                                                                  \
        }                                                                      \
    } while (false)

} // namespace

namespace rir {
namespace pir {

void Constantfold::apply(RirCompiler& cmp, Closure* function,
                         LogStream&) const {
    std::unordered_map<BB*, bool> branchRemoval;
    DominanceGraph dom(function);

    Visitor::run(function->entry, [&](BB* bb) {
        if (bb->isEmpty())
          return;
        auto ip = bb->begin();
        while (ip != bb->end()) {
            auto i = *ip;
            auto next = ip + 1;

            // Constantfolding of some common operations
            FOLD(Add, symbol::Add);
            FOLD(Sub, symbol::Sub);
            FOLD(Mul, symbol::Mul);
            FOLD(Div, symbol::Div);
            FOLD(IDiv, symbol::Idiv);
            FOLD(Lt, symbol::Lt);
            FOLD(Gt, symbol::Gt);
            FOLD(Lte, symbol::Le);
            FOLD(Gte, symbol::Ge);
            FOLD(Eq, symbol::Eq);
            FOLD(Neq, symbol::Ne);
            FOLD(Pow, symbol::Pow);

            if (auto assume = Assume::Cast(i)) {
                auto condition = assume->arg<0>().val();
                if (auto isObj = IsObject::Cast(condition))
                    if (!isObj->arg<0>().val()->type.maybeObj())
                        next = bb->remove(ip);

                FOLD2(condition, Identical,
                      [&](SEXP a, SEXP b) { next = bb->remove(ip); });
            }

            ip = next;
        }

        if (auto branch = Branch::Cast(bb->last())) {
            // TODO: if we had native true/false values we could use constant
            // folding to reduce the branch condition and then only remove
            // branches which are constant true/false. But for now we look at
            // the actual condition and do the constantfolding and branch
            // removal in one go.

            auto condition = branch->arg<0>().val();
            if (auto tst = AsTest::Cast(condition)) {
                // Try to detect constant branch conditions and mark such
                // branches for removal
                auto cnst = isConst(tst->arg<0>().val());
                if (!cnst)
                    if (auto lgl = AsLogical::Cast(tst->arg<0>().val()))
                        cnst = isConst(lgl->arg<0>().val());
                if (cnst) {
                    SEXP c = cnst->c;
                    // Non length 1 condition throws warning
                    if (Rf_length(c) == 1) {
                        auto cond = Rf_asLogical(c);
                        // NA throws an error
                        if (cond != NA_LOGICAL) {
                            branchRemoval.emplace(bb, cond);
                        }
                    }
                }

                // If the `Identical` instruction can be resolved statically,
                // use it for branch removal as well.
                FOLD2(tst->arg<0>().val(), Identical, [&](SEXP a, SEXP b) {
                    branchRemoval.emplace(bb, a == b);
                });
            }

            if (auto isObj = IsObject::Cast(condition))
                if (!isObj->arg<0>().val()->type.maybeObj())
                    branchRemoval.emplace(bb, false);
        }
    });

    std::unordered_set<BB*> toDelete;
    // Find all dead basic blocks
    for (auto e : branchRemoval) {
        auto bb = e.first;
        auto deadBranch = e.second ? bb->falseBranch() : bb->trueBranch();
        deadBranch->collectDominated(toDelete, dom);
        toDelete.insert(deadBranch);
    }

    BBTransform::removeBBs(function, toDelete);

    for (auto e : branchRemoval) {
        auto branch = e.first;
        auto condition = e.second;
        branch->remove(branch->end() - 1);
        if (condition) {
            branch->next1 = nullptr;
        } else {
            branch->next0 = branch->next1;
            branch->next1 = nullptr;
        }
    }
}
} // namespace pir
} // namespace rir
