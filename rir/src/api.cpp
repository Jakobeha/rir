/** Enables the use of R internals for us so that we can manipulate R structures
 * in low level.
 */

#include <cassert>

#include "api.h"

#include "compiler/parameter.h"
#include "compiler/pir/closure_property.h"
#include "compiler/test/PirCheck.h"
#include "compiler/test/PirTests.h"
#include "compiler/translations/pir_2_rir/pir_2_rir.h"
#include "compiler/translations/rir_2_pir/rir_2_pir.h"
#include "compiler/translations/rir_2_pir/rir_2_pir_compiler.h"
#include "interpreter/interp_incl.h"
#include "ir/BC.h"
#include "ir/Compiler.h"

#include <list>
#include <memory>
#include <string>

using namespace rir;

int R_ENABLE_JIT = getenv("R_ENABLE_JIT") ? atoi(getenv("R_ENABLE_JIT")) : 3;

static bool parseDebugStyle(const char* str, pir::DebugStyle& s) {
#define V(style)                                                               \
    if (strcmp(str, #style) == 0) {                                            \
        s = pir::DebugStyle::style;                                            \
        return true;                                                           \
    } else
    LIST_OF_DEBUG_STYLES(V)
#undef V
    {
        return false;
    }
}

REXPORT SEXP rir_disassemble(SEXP what, SEXP verbose) {
    if (!what || TYPEOF(what) != CLOSXP)
        Rf_error("Not a rir compiled code");
    DispatchTable* t = DispatchTable::check(BODY(what));

    if (!t)
        Rf_error("Not a rir compiled code");

    std::cout << "* closure " << what << " (vtable " << t << ", env "
              << CLOENV(what) << ")\n";
    for (size_t entry = 0; entry < t->size(); ++entry) {
        Function* f = t->get(entry);
        std::cout << "= vtable slot <" << entry << "> (" << f << ", invoked "
                  << f->invocationCount() << ") =\n";
        std::cout << "# ";
        f->signature().print(std::cout);
        std::cout << "\n";
        f->disassemble(std::cout);
    }

    return R_NilValue;
}

REXPORT SEXP rir_compile(SEXP what, SEXP env) {
    if (TYPEOF(what) == CLOSXP) {
        SEXP body = BODY(what);
        if (TYPEOF(body) == EXTERNALSXP)
            return what;

        // Change the input closure inplace
        Compiler::compileClosure(what);

        return what;
    } else {
        if (TYPEOF(what) == BCODESXP) {
            what = VECTOR_ELT(CDR(what), 0);
        }
        SEXP result = Compiler::compileExpression(what);
        return result;
    }
}

REXPORT SEXP rir_markOptimize(SEXP what) {
    // TODO(mhyee): This is to mark a function for optimization.
    // However, now that we have vtables, does this still make sense? Maybe it
    // might be better to mark a specific version for optimization.
    // For now, we just mark the first version in the vtable.
    if (TYPEOF(what) != CLOSXP)
        return R_NilValue;
    SEXP b = BODY(what);
    DispatchTable* dt = DispatchTable::unpack(b);
    Function* fun = dt->baseline();
    fun->markOpt = true;
    return R_NilValue;
}

REXPORT SEXP rir_eval(SEXP what, SEXP env) {
    if (Function* f = Function::check(what))
        return evalRirCodeExtCaller(f->body(), globalContext(), env);

    if (isValidClosureSEXP(what))
        return rirEval_f(BODY(what), env);

    Rf_error("Not rir compiled code");
}

REXPORT SEXP rir_body(SEXP cls) {
    if (!isValidClosureSEXP(cls))
        Rf_error("Not a valid rir compiled function");
    return DispatchTable::unpack(BODY(cls))->baseline()->container();
}

REXPORT SEXP pir_debugFlags(
#define V(n) SEXP n,
    LIST_OF_PIR_DEBUGGING_FLAGS(V)
#undef V
        SEXP IHaveTooManyCommasDummy) {
    pir::DebugOptions opts;

#define V(n)                                                                   \
    if (Rf_asLogical(n))                                                       \
        opts.flags.set(pir::DebugFlag::n);
    LIST_OF_PIR_DEBUGGING_FLAGS(V)
#undef V

    SEXP res = Rf_allocVector(INTSXP, 1);
    INTEGER(res)[0] = (int)opts.flags.to_i();
    return res;
}

static pir::PirType parsePirType(SEXP typeSexp) {
    // TODO: Valid format checks
    if (typeSexp == R_NilValue)
        return pir::PirType::any();
    if (TYPEOF(typeSexp) != STRSXP)
        Rf_error("can't parse pir type");
    return pir::PirType::parse(CHAR(Rf_asChar(typeSexp)));
}

REXPORT SEXP pir_assumeProps(
#define V(n) SEXP n##Sexp,
    LIST_OF_CLOSURE_PROPERTIES(V)
#undef V
        SEXP forceOrderSexp,
    SEXP typeSexp) {
    pir::ClosureProperties opts;

#define V(n)                                                                   \
    if (Rf_asLogical(n##Sexp))                                                 \
        opts.set(pir::ClosureProperty::n);
    LIST_OF_CLOSURE_PROPERTIES(V)
#undef V

    RList forceOrder(forceOrderSexp);
    for (SEXP idxSexp : forceOrder) {
        if (TYPEOF(idxSexp) == INTSXP && LENGTH(idxSexp) == 1)
            opts.argumentForceOrder.push_back(INTEGER(idxSexp)[0]);
        else if (TYPEOF(idxSexp) == REALSXP && LENGTH(idxSexp) == 1)
            opts.argumentForceOrder.push_back((int)REAL(idxSexp)[0]);
        else
            Rf_error("can't parse force order - indices must be scalar ints or "
                     "reals");
    }
    opts.returnType = parsePirType(typeSexp);
    // TODO: Would expect opts.size() / sizeof(int), but this needs padding or
    // opts.size() is wrong.
    int optsSize = opts.size();
    SEXP res = Rf_allocVector(INTSXP, optsSize);
    memcpy(DATAPTR(res), (void*)&opts, optsSize);
    return res;
}

static pir::DebugOptions::DebugFlags getInitialDebugFlags() {
    auto verb = getenv("PIR_DEBUG");
    if (!verb)
        return pir::DebugOptions::DebugFlags();
    std::istringstream in(verb);

    pir::DebugOptions::DebugFlags flags;
    while (!in.fail()) {
        std::string opt;
        std::getline(in, opt, ',');
        if (opt.empty())
            continue;

        bool success = false;

#define V(flag)                                                                \
    if (opt.compare(#flag) == 0) {                                             \
        success = true;                                                        \
        flags = flags | pir::DebugFlag::flag;                                  \
    }
        LIST_OF_PIR_DEBUGGING_FLAGS(V)
#undef V
        if (!success) {
            std::cerr << "Unknown PIR debug flag " << opt << "\n"
                      << "Valid flags are:\n";
#define V(flag) std::cerr << "- " << #flag << "\n";
            LIST_OF_PIR_DEBUGGING_FLAGS(V)
#undef V
            exit(1);
        }
    }
    return flags;
}

static std::regex getInitialDebugPassFilter() {
    auto filter = getenv("PIR_DEBUG_PASS_FILTER");
    if (filter)
        return std::regex(filter);
    return std::regex(".*");
}

static std::regex getInitialDebugFunctionFilter() {
    auto filter = getenv("PIR_DEBUG_FUNCTION_FILTER");
    if (filter)
        return std::regex(filter);
    return std::regex(".*");
}

static pir::DebugStyle getInitialDebugStyle() {
    auto styleStr = getenv("PIR_DEBUG_STYLE");
    if (!styleStr) {
        return pir::DebugStyle::Standard;
    }
    pir::DebugStyle style;
    if (!parseDebugStyle(styleStr, style)) {
        std::cerr << "Unknown PIR debug print style " << styleStr << "\n"
                  << "Valid styles are:\n";
#define V(style) std::cerr << "- " << #style << "\n";
        LIST_OF_DEBUG_STYLES(V)
#undef V
        exit(1);
    }
    return style;
}

pir::DebugOptions PirDebug = {
    getInitialDebugFlags(), getInitialDebugPassFilter(),
    getInitialDebugFunctionFilter(), getInitialDebugStyle()};

REXPORT SEXP pir_setDebugFlags(SEXP debugFlags) {
    if (TYPEOF(debugFlags) != INTSXP || Rf_length(debugFlags) < 1)
        Rf_error(
            "pir_setDebugFlags expects an integer vector as second parameter");
    PirDebug.flags = pir::DebugOptions::DebugFlags(INTEGER(debugFlags)[0]);
    return R_NilValue;
}

static SEXP pirCompile(SEXP what, const Assumptions& assumptions,
                       const std::string& name, const pir::DebugOptions& debug,
                       const pir::ClosureProperties& aprops) {
    if (!isValidClosureSEXP(what)) {
        Rf_error("not a compiled closure");
    }
    if (!DispatchTable::check(BODY(what))) {
        Rf_error("Cannot optimize compiled expression, only closure");
    }

    PROTECT(what);

    bool dryRun = debug.includes(pir::DebugFlag::DryRun);
    // compile to pir
    pir::Module* m = new pir::Module;
    pir::StreamLogger logger(debug);
    logger.title("Compiling " + name);
    pir::Rir2PirCompiler cmp(m, logger);
    cmp.compileClosure(what, name, assumptions, aprops,
                       [&](pir::ClosureVersion* c) {
                           logger.flush();
                           cmp.optimizeModule();

                           // compile back to rir
                           pir::Pir2RirCompiler p2r(logger);
                           auto fun = p2r.compile(c, dryRun);

                           // Install
                           if (dryRun)
                               return;

                           Protect p(fun->container());
                           DispatchTable::unpack(BODY(what))->insert(fun);
                       },
                       [&]() {
                           if (debug.includes(pir::DebugFlag::ShowWarnings))
                               std::cerr << "Compilation failed\n";
                       });

    delete m;
    UNPROTECT(1);
    return what;
}

SEXP pirCompile(SEXP closure, const rir::Assumptions& assumptions,
                const std::string& name, const rir::pir::DebugOptions& debug) {
    pir::ClosureProperties aprops = pir::ClosureProperties();
    return pirCompile(closure, assumptions, name, debug, aprops);
}

// Used in test infrastructure for counting invocation of different versions
REXPORT SEXP rir_invocation_count(SEXP what) {
    if (!isValidClosureSEXP(what)) {
        Rf_error("not a compiled closure");
    }
    auto dt = DispatchTable::check(BODY(what));
    assert(dt);

    SEXP res = Rf_allocVector(INTSXP, dt->size());
    for (size_t i = 0; i < dt->size(); ++i)
        INTEGER(res)[i] = dt->get(i)->invocationCount();

    return res;
}

REXPORT SEXP pir_compile(SEXP what, SEXP name, SEXP debugFlags, SEXP debugStyle,
                         SEXP assumeProps) {
    if (debugFlags != R_NilValue &&
        (TYPEOF(debugFlags) != INTSXP || Rf_length(debugFlags) != 1))
        Rf_error("pir_compile expects an integer scalar as second parameter");
    if (debugStyle != R_NilValue && TYPEOF(debugStyle) != SYMSXP)
        Rf_error("pir_compile expects a symbol as third parameter");
    if (TYPEOF(assumeProps) != INTSXP)
        Rf_error("pir_compile expects result of pir.assumeProps as fourth "
                 "parameter");
    std::string n;
    if (TYPEOF(name) == SYMSXP)
        n = CHAR(PRINTNAME(name));
    pir::DebugOptions opts = PirDebug;

    if (debugFlags != R_NilValue) {
        opts.flags = *INTEGER(debugFlags);
    }
    if (debugStyle != R_NilValue) {
        if (!parseDebugStyle(CHAR(PRINTNAME(debugStyle)), opts.style)) {
            Rf_error("pir_compile - given unknown debug style");
        }
    }
    pir::ClosureProperties aprops =
        *(pir::ClosureProperties*)DATAPTR(assumeProps);
    return pirCompile(what, rir::pir::Rir2PirCompiler::defaultAssumptions, n,
                      opts, aprops);
}

REXPORT SEXP pir_tests() {
    PirTests::run();
    return R_NilValue;
}

static size_t oldMaxInput = 0;
static size_t oldInlinerMax = 0;

REXPORT SEXP pir_check_warmup_begin(SEXP f, SEXP checksSxp, SEXP env) {
    if (oldMaxInput == 0) {
        oldMaxInput = pir::Parameter::MAX_INPUT_SIZE;
        oldInlinerMax = pir::Parameter::INLINER_MAX_SIZE;
    }
    pir::Parameter::MAX_INPUT_SIZE = 3500;
    pir::Parameter::INLINER_MAX_SIZE = 4000;
    return R_NilValue;
}
REXPORT SEXP pir_check_warmup_end(SEXP f, SEXP checksSxp, SEXP env) {
    pir::Parameter::MAX_INPUT_SIZE = oldMaxInput;
    pir::Parameter::INLINER_MAX_SIZE = oldInlinerMax;
    return R_NilValue;
}

REXPORT SEXP pir_check(SEXP f, SEXP checksSxp, SEXP env) {
    if (TYPEOF(checksSxp) != LISTSXP)
        Rf_error("pir_check: 2nd parameter must be a pairlist (of symbols)");
    std::list<PirCheck::Type> checkTypes;
    for (SEXP c = checksSxp; c != R_NilValue; c = CDR(c)) {
        SEXP checkSxp = CAR(c);
        if (TYPEOF(checkSxp) != SYMSXP)
            Rf_error("pir_check: each item in 2nd parameter must be a symbol");
        PirCheck::Type type = PirCheck::parseType(CHAR(PRINTNAME(checkSxp)));
        if (type == PirCheck::Type::Invalid)
            Rf_error("pir_check: invalid check type. List of check types:"
#define V(Check) "\n    " #Check
                     LIST_OF_PIR_CHECKS(V)
#undef V
            );
        checkTypes.push_back(type);
    }
    // Automatically compile rir for convenience (necessary to get PIR)
    if (!isValidClosureSEXP(f))
        rir_compile(f, env);
    PirCheck check(checkTypes);
    bool res = check.run(f);
    return res ? R_TrueValue : R_FalseValue;
}

SEXP rirOptDefaultOpts(SEXP closure, const Assumptions& assumptions,
                       SEXP name) {
    std::string n = "";
    if (TYPEOF(name) == SYMSXP)
        n = CHAR(PRINTNAME(name));
    // PIR can only optimize closures, not expressions
    if (isValidClosureSEXP(closure))
        return pirCompile(closure, assumptions, n, PirDebug);
    else
        return closure;
}

SEXP rirOptDefaultOptsDryrun(SEXP closure, const Assumptions& assumptions,
                             SEXP name) {
    std::string n = "";
    if (TYPEOF(name) == SYMSXP)
        n = CHAR(PRINTNAME(name));
    // PIR can only optimize closures, not expressions
    if (isValidClosureSEXP(closure))
        return pirCompile(closure, assumptions, n,
                          PirDebug | pir::DebugFlag::DryRun);
    else
        return closure;
}

bool startup() {
    initializeRuntime();
    return true;
}

bool startup_ok = startup();
