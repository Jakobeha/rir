#ifndef SYMBOLS_LIST_H_
#define SYMBOLS_LIST_H_

#include "simple_instruction_list.h"
#define SYMBOLS_SIMPLE_INSTRUCTION_V(V, name, _) V(name, "." #name)

#define SYMBOLS(V)                                                             \
    V(SuperAssignBracket, "[<<-")                                              \
    V(SuperAssignDoubleBracket, "[[<<-")                                       \
    V(AssignBracket, "[<-")                                                    \
    V(AssignDoubleBracket, "[[<-")                                             \
    V(SuperAssign2Bracket, "[,<<-")                                            \
    V(SuperAssign2DoubleBracket, "[[,<<-")                                     \
    V(Assign2Bracket, "[,<-")                                                  \
    V(Assign2DoubleBracket, "[[,<-")                                           \
    V(DoubleBracket, "[[")                                                     \
    V(Bracket, "[")                                                            \
    V(Block, "{")                                                              \
    V(Parenthesis, "(")                                                        \
    V(Assign, "<-")                                                            \
    V(Assign2, "=")                                                            \
    V(SuperAssign, "<<-")                                                      \
    V(If, "if")                                                                \
    V(Function, "function")                                                    \
    V(Return, "return")                                                        \
    V(For, "for")                                                              \
    V(While, "while")                                                          \
    V(Repeat, "repeat")                                                        \
    V(Break, "break")                                                          \
    V(Next, "next")                                                            \
    V(Switch, "switch")                                                        \
    V(Add, "+")                                                                \
    V(Sub, "-")                                                                \
    V(Mul, "*")                                                                \
    V(Div, "/")                                                                \
    V(Pow, "^")                                                                \
    V(Idiv, "%/%")                                                             \
    V(Mod, "%%")                                                               \
    V(Sqrt, "sqrt")                                                            \
    V(Exp, "exp")                                                              \
    V(Eq, "==")                                                                \
    V(Ne, "!=")                                                                \
    V(Lt, "<")                                                                 \
    V(Le, "<=")                                                                \
    V(Ge, ">=")                                                                \
    V(Gt, ">")                                                                 \
    V(BitAnd, "&")                                                             \
    V(BitOr, "|")                                                              \
    V(Not, "!")                                                                \
    V(Ellipsis, "...")                                                         \
    V(Colon, ":")                                                              \
    V(Internal, ".Internal")                                                   \
    V(tmp, "*tmp*")                                                            \
    V(vtmp, "*vtmp*")                                                          \
    V(value, "value")                                                          \
    V(isnull, "is.null")                                                       \
    V(islist, "is.list")                                                       \
    V(ispairlist, "is.pairlist")                                               \
    V(quote, "quote")                                                          \
    V(And, "&&")                                                               \
    V(Or, "||")                                                                \
    V(Missing, "missing")                                                      \
    V(seq, "seq")                                                              \
    V(lapply, "lapply")                                                        \
    V(aslist, "as.list")                                                       \
    V(isvector, "is.vector")                                                   \
    V(substr, "substr")                                                        \
    V(Class, "class")                                                          \
    V(OldClass, "oldClass")                                                    \
    V(at, "@")                                                                 \
    V(names, "names")                                                          \
    V(attr, "attr")                                                            \
    V(body, "body")                                                            \
    V(slot, "slot")                                                            \
    V(as, "as")                                                                \
    V(packageSlot, "packageSlot")                                              \
    V(attributes, "attributes")                                                \
    V(c, "c")                                                                  \
    V(standardGeneric, "standardGeneric")                                      \
    SIMPLE_INSTRUCTIONS(SYMBOLS_SIMPLE_INSTRUCTION_V, V)                       \
    V(UseMethod, "UseMethod")                                                  \
    V(sysframe, "sys.frame")                                                   \
    V(syscall, "sys.call")                                                     \
    V(srcref, "srcref")

#endif // SYMBOLS_LIST_H_
