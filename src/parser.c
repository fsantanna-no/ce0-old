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

    // EXPR_CALL
    while (check('(')) {                // only checks, arg will accept
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
    }

    return 1;
}
