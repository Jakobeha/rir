#ifndef BC_NOARG_LIST_H
#define BC_NOARG_LIST_H

#include "simple_instruction_list.h"

#define V_SIMPLE_INSTRUCTION_IN_BC_NOARGS(V, name, Name) V(_, name, name)
#define BC_NOARGS_NATIVE(V, NESTED, op)                                        \
    V(NESTED, op, op)                                                          \
    V(NESTED, op##_int, op##_int)                                              \
    V(NESTED, op##_real, op##_real)                                            \
    V(NESTED, op##_lgl, op##_lgl)
#define BC_NOARGS_NATIVE_NUM(V, NESTED, op)                                    \
    V(NESTED, op, op)                                                          \
    V(NESTED, op##_int, op##_int)                                              \
    V(NESTED, op##_real, op##_real)

#define BC_NOARGS(V, NESTED)                                                   \
    SIMPLE_INSTRUCTIONS(V_SIMPLE_INSTRUCTION_IN_BC_NOARGS, V)                  \
    V(NESTED, popContext, pop_context)                                         \
    V(NESTED, nop, nop)                                                        \
    V(NESTED, parentEnv, parent_env)                                           \
    V(NESTED, getEnv, get_env)                                                 \
    V(NESTED, setEnv, set_env)                                                 \
    V(NESTED, ret, ret)                                                        \
    V(NESTED, pop, pop)                                                        \
    V(NESTED, force, force)                                                    \
    V(NESTED, asast, asast)                                                    \
    V(NESTED, checkMissing, check_missing)                                     \
    V(NESTED, subassign1_1, subassign1_1)                                      \
    V(NESTED, subassign2_1, subassign2_1)                                      \
    V(NESTED, subassign1_2, subassign1_2)                                      \
    V(NESTED, subassign2_2, subassign2_2)                                      \
    V(NESTED, length, length)                                                  \
    V(NESTED, names, names)                                                    \
    V(NESTED, setNames, set_names)                                             \
    V(NESTED, asbool, asbool)                                                  \
    V(NESTED, endloop, endloop)                                                \
    V(NESTED, dup, dup)                                                        \
    V(NESTED, dup2, dup2)                                                      \
    V(NESTED, forSeqSize, for_seq_size)                                        \
    V(NESTED, inc, inc)                                                        \
    V(NESTED, inc_int, inc_int)                                                \
    V(NESTED, close, close)                                                    \
    BC_NOARGS_NATIVE_NUM(V, NESTED, add)                                       \
    BC_NOARGS_NATIVE_NUM(V, NESTED, mul)                                       \
    BC_NOARGS_NATIVE_NUM(V, NESTED, div)                                       \
    BC_NOARGS_NATIVE_NUM(V, NESTED, idiv)                                      \
    BC_NOARGS_NATIVE_NUM(V, NESTED, mod)                                       \
    BC_NOARGS_NATIVE_NUM(V, NESTED, sub)                                       \
    BC_NOARGS_NATIVE_NUM(V, NESTED, uplus)                                     \
    BC_NOARGS_NATIVE_NUM(V, NESTED, uminus)                                    \
    BC_NOARGS_NATIVE(V, NESTED, lt)                                            \
    BC_NOARGS_NATIVE(V, NESTED, gt)                                            \
    BC_NOARGS_NATIVE(V, NESTED, le)                                            \
    BC_NOARGS_NATIVE(V, NESTED, ge)                                            \
    BC_NOARGS_NATIVE(V, NESTED, eq)                                            \
    BC_NOARGS_NATIVE(V, NESTED, ne)                                            \
    V(NESTED, not_, not)                                                       \
    V(NESTED, not_int, not_int)                                                \
    V(NESTED, not_real, not_real)                                              \
    V(NESTED, not_lgl, not_lgl)                                                \
    V(NESTED, pow, pow)                                                        \
    V(NESTED, identicalNoforce, identical_noforce)                             \
    V(NESTED, seq, seq)                                                        \
    V(NESTED, colon, colon)                                                    \
    V(NESTED, makeUnique, make_unique)                                         \
    V(NESTED, setShared, set_shared)                                           \
    V(NESTED, ensureNamed, ensure_named)                                       \
    V(NESTED, asLogical, aslogical)                                            \
    V(NESTED, lglOr, lgl_or)                                                   \
    V(NESTED, lglAnd, lgl_and)                                                 \
    V(NESTED, isfun, isfun)                                                    \
    V(NESTED, invisible, invisible)                                            \
    V(NESTED, visible, visible)                                                \
    V(NESTED, extract1_1, extract1_1)                                          \
    V(NESTED, extract1_2, extract1_2)                                          \
    V(NESTED, extract2_1, extract2_1)                                          \
    V(NESTED, extract2_2, extract2_2)                                          \
    V(NESTED, swap, swap)                                                      \
    V(NESTED, isobj, isobj)                                                    \
    V(NESTED, isstubenv, isstubenv)                                            \
    V(NESTED, return_, return )                                                \
    V(NESTED, unbox, unbox)

#undef V_SIMPLE_INSTRUCTION

#endif
