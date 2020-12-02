#include <assert.h>
#include <stdlib.h>

#include "all.h"

int accept (TK enu) {
    if (ALL.tk1.enu == enu) {
        lexer();
        return 1;
    } else {
        return 0;
    }
}

int check (TK enu) {
    return (ALL.tk1.enu == enu);
}

int err_expected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): expected %s : have %s",
        ALL.tk1.lin, ALL.tk1.col, v, lexer_tk2str(&ALL.tk1));
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int parser_type (Type* ret) {
    // TYPE_UNIT
    if (accept('(')) {
        if (accept(')')) {
            *ret = (Type) { TYPE_UNIT };
        } else {
            if (!parser_type(ret)) {
                return 0;
            }

    // TYPE_TUPLE
            if (check(',')) {
                int n = 1;
                Type* vec = malloc(n*sizeof(Type));
                vec[n-1] = *ret;
                while (accept(',')) {
                    Type tp;
                    if (!parser_type(&tp)) {
                        return 0;
                    }
                    n++;
                    vec = realloc(vec, n*sizeof(Type));
                    vec[n-1] = tp;
                }
                if (!accept(')')) {
                    return err_expected("`)´");
                }
                *ret = (Type) { TYPE_TUPLE, .Tuple={n,vec} };

    // TYPE_PARENS
            } else if (!accept(')')) {
                return err_expected("`)`");
            }
        }

    // TYPE_NATIVE
    } else if (accept(TK_NATIVE)) {
        *ret = (Type) { TYPE_NATIVE, .tk=ALL.tk0 };

    } else {
        return err_expected("type");
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

static int parser_expr_one (Expr* ret) {
    // EXPR_UNIT
    if (accept('(')) {
        if (accept(')')) {
            *ret = (Expr) { EXPR_UNIT };
        } else {
            if (!parser_expr(ret)) {
                return 0;
            }

    // EXPR_TUPLE
            if (check(',')) {
                int n = 1;
                Expr* vec = malloc(n*sizeof(Expr));
                vec[n-1] = *ret;
                while (accept(',')) {
                    Expr e;
                    if (!parser_expr(&e)) {
                        return 0;
                    }
                    n++;
                    vec = realloc(vec, n*sizeof(Expr));
                    vec[n-1] = e;
                }
                if (!accept(')')) {
                    return err_expected("`)´");
                }
                *ret = (Expr) { EXPR_TUPLE, .Tuple={n,vec} };

    // EXPR_PARENS
            } else if (!accept(')')) {
                return err_expected("`)´");
            }
        }

    // EXPR_NATIVE
    } else if (accept(TK_NATIVE)) {
        *ret = (Expr) { EXPR_NATIVE, .tk=ALL.tk0 };

    // EXPR_VAR
    } else if (accept(TK_VAR)) {
        *ret = (Expr) { EXPR_VAR, .tk=ALL.tk0 };

    } else {
        return err_expected("expression");
    }
    return 1;
}

int parser_expr (Expr* ret) {
    Expr e;
    if (!parser_expr_one(&e)) {
        return 0;
    }
    *ret = e;

    while (1) {
    // EXPR_CALL
        if (check('(')) {                // only checks, arg will accept
            Expr arg;
            if (!parser_expr_one(&arg)) {   // f().() and not f.()()
                return 0;
            }
            Expr* parg = malloc(sizeof(Expr));
            *parg = arg;
            assert(parg != NULL);

            Expr* func = malloc(sizeof(Expr));
            assert(func != NULL);
            *func = *ret;
            *ret  = (Expr) { EXPR_CALL, .Call={func,parg} };
        } else if (accept(TK_INDEX)) {
            Expr* tup = malloc(sizeof(Expr));
            assert(tup != NULL);
            *tup = *ret;
            *ret = (Expr) { EXPR_INDEX, .Index={tup,ALL.tk0.val.n} };
        } else {
            break;
        }
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////


int parser_stmt (Stmt* ret) {
    // STMT_DECL
    if (accept(TK_VAL)) {
        if (!accept(TK_VAR)) {
            return err_expected("variable identifier");
        }
        Tk var = ALL.tk0;
        if (!accept(':')) {
            return err_expected("`:´");
        }
        Type tp;
        if (!parser_type(&tp)) {
            return 0;
        }
        *ret = (Stmt) { STMT_DECL, .Decl={var,tp} };

    // STMT_CALL
    } else if (accept(TK_CALL)) {
        Expr e;
        if (!parser_expr(&e)) {
            return 0;
        }
        *ret = (Stmt) { STMT_CALL, .call=e };
    }

    return 1;
}
