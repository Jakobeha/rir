#pragma once

#include "R/Symbols.h"
#include "R/r.h"
#include "interpreter/LazyEnvironment.h"
#include <queue>
#include <stack>
#include <unordered_map>

namespace rir {

#define ENV_LEXICAL_MASK 1
#define DEBUG_ENV_TRACKER

class EnvTracker {
  private:
    // Helpers
    inline static bool isEnv(SEXP env) {
        return TYPEOF(env) == ENVSXP || LazyEnvironment::check(env);
    }

    inline static std::stack<SEXP> lexicalEnvChain(SEXP env) {
        std::queue<SEXP> rev;
        SLOWASSERT(isEnv(env));
        do {
            rev.push(env);
            env = ENCLOS(env);
        } while (env != R_BaseEnv);
        std::stack<SEXP> res;
        while (!rev.empty()) {
            res.push(rev.front());
            rev.pop();
        }
        return res;
    }

    // Stack passed by value on purpose
    template <typename T>
    inline bool stackContains(std::stack<T> stack, T elem) {
        while (!stack.empty()) {
            if (stack.top() == elem)
                return true;
            stack.pop();
        }
        return false;
    }

    std::stack<std::stack<std::stack<SEXP>>> envs;

    EnvTracker() : envs() {}

    inline std::stack<std::stack<std::stack<SEXP>>>& promiseEnvStack() {
        return envs;
    }

    inline std::stack<std::stack<SEXP>>& callEnvStack() {
        SLOWASSERT(!promiseEnvStack().empty());
        return promiseEnvStack().top();
    }

    inline std::stack<SEXP>& lexicalEnvStack() {
        SLOWASSERT(!callEnvStack().empty());
        return callEnvStack().top();
    }

    inline SEXP localEnv() {
        SLOWASSERT(!lexicalEnvStack().empty());
        return lexicalEnvStack().top();
    }

    inline void
    changeLexicalContext(SEXP newTop,
                         std::function<void(const std::stack<SEXP>&)> action) {
        SLOWASSERT(newTop == NULL || isEnv(newTop));
        if (!promiseEnvStack().empty() && !callEnvStack().empty()) {
            // Copy stack (by value)
            for (std::stack<SEXP> oldChain = callEnvStack().top();
                 !oldChain.empty(); oldChain.pop()) {
                SEXP env = oldChain.top();
                env->sxpinfo.extra &= ~ENV_LEXICAL_MASK;
            }
        }
        std::stack<SEXP> newChain;
        if (newTop == NULL) {
            action(newChain);
            if (!promiseEnvStack().empty()) {
                newChain = lexicalEnvChain(localEnv());
            }
        } else {
            newChain = lexicalEnvChain(newTop);
            action(newChain);
        }
        for (; !newChain.empty(); newChain.pop()) {
            SEXP env = newChain.top();
            env->sxpinfo.extra |= ENV_LEXICAL_MASK;
        }
    }

  public:
    static EnvTracker* current() {
        return (EnvTracker*)R_GlobalContext->external_env_tracker;
    }

    static void createEnvTracker(RCNTXT* ctx) {
        ctx->external_env_tracker = new EnvTracker;
        if (EnvTracker* cur = current())
            ((EnvTracker*)ctx->external_env_tracker)->envs = cur->envs; // Copy
    }

    static void destroyEnvTracker(RCNTXT* ctx) {
        assert(ctx->external_env_tracker != NULL);
        delete (EnvTracker*)ctx->external_env_tracker;
    }

    inline bool isEnvLexical(SEXP env) {
        bool res = env->sxpinfo.extra & ENV_LEXICAL_MASK ? true : false;
        SLOWASSERT(isEnv(env) && res == stackContains(lexicalEnvStack(), env));
        return res;
    }

    /*inline void pushLexicalEnv(SEXP env) {
#ifdef DEBUG_ENV_TRACKER
        std::cout << " # push lexical env: ";
        Rf_PrintValue(env);
#endif
        SLOWASSERT(isEnv(env) && !stackContains(lexicalEnvStack(), env));
        lexicalEnvStack().push(env);
        env->sxpinfo.extra |= ENV_LEXICAL_MASK;
    }

    inline void popLexicalEnv(SEXP old) {
#ifdef DEBUG_ENV_TRACKER
        std::cout << " # pop lexical env: ";
        Rf_PrintValue(old);
#endif
        SLOWASSERT(isEnv(old) && lexicalEnvStack().top() == old);
        lexicalEnvStack().pop();
        old->sxpinfo.extra &= ~ENV_LEXICAL_MASK;
    }*/

    inline void pushCallEnv(SEXP env) {
#ifdef DEBUG_ENV_TRACKER
        for (int i = 0; i < (int)callEnvStack().size(); i++)
            std::cout << "-";
        std::cout << " # push call env: ";
        Rf_PrintValue(env);
#endif
        changeLexicalContext(env, [&](const std::stack<SEXP>& newChain) {
            callEnvStack().push(newChain);
        });
    }

    inline void popCallEnv(SEXP cur, SEXP old) {
#ifdef DEBUG_ENV_TRACKER
        for (int i = 0; i < (int)callEnvStack().size() - 1; i++)
            std::cout << "-";
        std::cout << " # pop call env: ";
        Rf_PrintValue(cur);
#endif
        SLOWASSERT(stackContains(lexicalEnvStack(), cur));
        changeLexicalContext(old, [&](const std::stack<SEXP>& newChain) {
            callEnvStack().pop();
        });
    }

    inline void pushPromiseEnv(SEXP env) {
#ifdef DEBUG_ENV_TRACKER
        for (int i = 0; i < (int)promiseEnvStack().size(); i++)
            std::cout << "-";
        std::cout << " # push promise env: ";
        Rf_PrintValue(env);
#endif
        changeLexicalContext(env, [&](const std::stack<SEXP>& newChain) {
            std::stack<std::stack<SEXP>> newCallChain;
            newCallChain.push(newChain);
            promiseEnvStack().push(newCallChain);
        });
    }

    inline void popPromiseEnv(SEXP cur) {
#ifdef DEBUG_ENV_TRACKER
        for (int i = 0; i < (int)promiseEnvStack().size() - 1; i++)
            std::cout << "-";
        std::cout << " # pop promise env: ";
        Rf_PrintValue(cur);
#endif
        SLOWASSERT(stackContains(lexicalEnvStack(), cur));
        changeLexicalContext(NULL, [&](const std::stack<SEXP>& newChain) {
            promiseEnvStack().pop();
        });
    }

    inline void start(SEXP env) {
#ifdef DEBUG_ENV_TRACKER
        std::cout << "## start tracking envs";
#endif
        assert(!envs.empty() || env == R_GlobalEnv);
#ifdef DEBUG_ENV_TRACKER
        if (!envs.empty()) {
            std::cout << " (lost track)";
        }
        std::cout << "\n";
#endif

        pushPromiseEnv(env);
    }

    inline void stop(SEXP cur) {
        popPromiseEnv(cur);
        assert(!envs.empty() || cur == R_GlobalEnv);
#ifdef DEBUG_ENV_TRACKER
        std::cout << "## stop tracking envs";
        if (envs.empty()) {
            std::cout << " (lost track)";
        }
        std::cout << "\n";
#endif
    }
};

} // namespace rir
