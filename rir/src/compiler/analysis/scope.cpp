#include "scope.h"
#include "../pir/pir_impl.h"
#include "query.h"

namespace {
using namespace rir::pir;

struct ScopeAnalysisState {
    AbstractREnvironmentHierarchy envs;
    std::unordered_map<Closure*, PirType> funTypes;
    bool merge(const ScopeAnalysisState& other) {
        bool changed = false;
        std::unordered_set<Closure*> ks;
        for (const auto& f : other.funTypes)
            changed = funTypes[f.first].merge(f.second) || changed;
        return envs.merge(other.envs) || changed;
    }
};

class TheScopeAnalysis : public StaticAnalysis<ScopeAnalysisState> {
  public:
    typedef StaticAnalysis<ScopeAnalysisState> Super;

    const std::vector<SEXP>& args;

    static constexpr size_t maxDepth = 5;
    size_t depth;
    Value* staticClosureEnv = Env::notClosed();

    TheScopeAnalysis(Closure* cls, const std::vector<SEXP>& args)
        : Super(cls), args(args), depth(0) {}
    TheScopeAnalysis(Closure* cls, const std::vector<SEXP>& args,
                     Value* staticClosureEnv, BB* bb,
                     const ScopeAnalysisState& initialState, size_t depth)
        : Super(cls, initialState), args(args), depth(depth),
          staticClosureEnv(staticClosureEnv) {}

    void apply(ScopeAnalysisState& state, Instruction* i) const override;

    typedef std::function<void(AbstractLoad)> LoadMaybe;
    void tryLoad(const ScopeAnalysisState& envs, Instruction* i,
                 LoadMaybe) const;

    void print(std::ostream& out = std::cout);
};

void TheScopeAnalysis::tryLoad(const ScopeAnalysisState& s, Instruction* i,
                               LoadMaybe aLoad) const {
    if (auto ld = LdVar::Cast(i)) {
        aLoad(s.envs.get(ld->env(), ld->varName));
    } else if (auto sld = LdVarSuper::Cast(i)) {
        aLoad(s.envs.superGet(sld->env(), sld->varName));
    } else if (auto ldf = LdFun::Cast(i)) {
        aLoad(s.envs.get(ldf->env(), ldf->varName));
    } else if (auto sts = StVarSuper::Cast(i)) {
        aLoad(s.envs.superGet(sts->env(), sts->varName));
    }
}

void TheScopeAnalysis::apply(ScopeAnalysisState& state, Instruction* i) const {
    bool handled = false;

    if (auto ret = Return::Cast(i)) {
        auto t = ret->arg<0>().val()->type;
        if (state.funTypes.count(closure))
            state.funTypes.at(closure).merge(t);
        else
            state.funTypes.emplace(closure, t);
        handled = true;
    }
    if (auto mk = MkEnv::Cast(i)) {
        Value* lexicalEnv = mk->lexicalEnv();
        // If we know the caller, we can fill in the parent env
        if (lexicalEnv == Env::notClosed() &&
            staticClosureEnv != Env::notClosed()) {
            lexicalEnv = staticClosureEnv;
        }
        state.envs[mk].parentEnv(lexicalEnv);
        mk->eachLocalVar(
            [&](SEXP name, Value* val) { state.envs[mk].set(name, val, mk); });
        handled = true;
    } else if (auto s = StVar::Cast(i)) {
        state.envs[s->env()].set(s->varName, s->val(), s);
        handled = true;
    } else if (auto ss = StVarSuper::Cast(i)) {
        auto superEnv = state.envs[ss->env()].parentEnv();
        if (superEnv != AbstractREnvironment::UnknownParent) {
            state.envs[superEnv].set(ss->varName, ss->val(), ss);
            handled = true;
        }
    } else if (CallInstruction::CastCall(i) && depth < maxDepth) {
        auto calli = CallInstruction::CastCall(i);
        if (auto call = Call::Cast(i)) {
            auto trg = call->cls()->baseValue();
            assert(trg);
            MkFunCls* cls = state.envs.findClosure(call->callerEnv(), trg);
            if (cls != AbstractREnvironment::UnknownClosure) {
                if (cls->fun->argNames.size() == calli->nCallArgs()) {
                    TheScopeAnalysis nextFun(cls->fun, cls->fun->argNames,
                                             cls->lexicalEnv(), cls->fun->entry,
                                             state, depth + 1);
                    nextFun();
                    state.merge(nextFun.result());
                    handled = true;
                }
            }
        } else if (auto call = StaticCall::Cast(i)) {
            auto trg = call->cls();
            if (trg && trg->argNames.size() == calli->nCallArgs()) {
                TheScopeAnalysis nextFun(trg, trg->argNames, trg->closureEnv(),
                                         trg->entry, state, depth + 1);
                nextFun();
                state.merge(nextFun.result());
                handled = true;
            }
        } else {
            // TODO: support for NamedCall
            assert((CallBuiltin::Cast(i) || CallSafeBuiltin::Cast(i) ||
                    NamedCall::Cast(i)) &&
                   "New call instruction not handled?");
        }
    }

    // Keep track of closures
    auto mkfun = MkFunCls::Cast(i);
    tryLoad(state, i,
            [&](AbstractLoad load) {
                handled = true;
                // let's check if we loaded a closure
                if (!mkfun)
                    load.result.ifSingleValue(
                        [&](Value* val) { mkfun = MkFunCls::Cast(val); });
            });
    if (mkfun)
        state.envs[mkfun->lexicalEnv()].mkClosures[i] = mkfun;

    if (!handled) {
        if (i->leaksEnv()) {
            state.envs[i->env()].leaked = true;
        }
        if (i->changesEnv()) {
            state.envs[i->env()].taint();
        }
    }
}

void TheScopeAnalysis::print(std::ostream& out) {
    for (size_t id = 0; id < getMergepoints().size(); ++id) {
        auto& m = getMergepoints()[id];
        if (!m.empty()) {
            out << "---- BB_" << id << " -----------------------------\n";
            size_t segment = 0;
            for (auto& e : m) {
                out << "    segm -- " << segment++ << "\n";
                for (auto& entry : e.envs) {
                    auto ptr = entry.first;
                    auto env = entry.second;
                    out << "Env(" << ptr << "), leaked " << env.leaked << ":\n";
                    env.print(out);
                }
            }
            out << "-------------------------------------\n";
        }
    }
    out << "---- exit -----------------------------\n";
    for (auto& entry : result().envs) {
        auto ptr = entry.first;
        auto env = entry.second;
        out << "Env(" << ptr << "), leaked " << env.leaked << ":\n";
        env.print(out);
    }
    out << "-------------------------------------\n";
}
}

namespace rir {
namespace pir {

ScopeAnalysis::ScopeAnalysis(Closure* function) {
    TheScopeAnalysis analysis(function, function->argNames);
    analysis();
    if (false)
        analysis.print(std::cout);

    // if (function->entry->isExit()) {
    //     analysis.foreach<PositioningStyle::AfterInstruction>(
    //         [&](auto res, Instruction* i) {
    //             i->printRef(std::cout);
    //             std::cout << ": \n";
    //             for (auto& entry : res) {
    //                 auto ptr = entry.first;
    //                 auto env = entry.second;
    //                 std::cout << "Env(" << ptr << "), leaked " << env.leaked
    //                           << ":\n";
    //                 env.print(std::cout);
    //             }
    //         });
    // }

    // Collect all abstract values of all loads
    analysis.foreach<PositioningStyle::BeforeInstruction>(
        [&](const ScopeAnalysisState& state, Instruction* i) {
            analysis.tryLoad(state, i, [&](AbstractLoad load) {
                loads.emplace(i, load);
                if (load.result.isUnknown()) {
                    for (auto env : state.envs.potentialParents(i->env()))
                        allStoresObserved.insert(env);
                } else {
                    load.result.eachSource([&](ValOrig& src) {
                        observedStores.insert(src.origin);
                    });
                }
            });
            if (i->leaksEnv()) {
                for (auto env : state.envs.potentialParents(i->env()))
                    allStoresObserved.insert(env);
            }
        });
    funTypes = std::move(analysis.result().funTypes);
}
}
}
