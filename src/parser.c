#include <assert.h>
#include <stdlib.h>

#include "all.h"

int err_expected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): expected %s : have %s",
        ALL.tk1.lin, ALL.tk1.col, v, lexer_tk2str(&ALL.tk1));
    return 0;
}

int accept (TK enu) {
    if (ALL.tk1.enu == enu) {
        lexer();
        return 1;
    } else {
        return 0;
    }
}

int accept_err (TK enu) {
    int ret = accept(enu);
    if (ret == 0) {
        err_expected(lexer_tk2err(enu));
        //puts(ALL.err);
    }
    return ret;
}

int check (TK enu) {
    return (ALL.tk1.enu == enu);
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
                if (!accept_err(')')) {
                    return 0;
                }
                *ret = (Type) { TYPE_TUPLE, .Tuple={n,vec} };

    // TYPE_PARENS
            } else if (!accept_err(')')) {
                return 0;
            }
        }

    // TYPE_NATIVE
    } else if (accept(TX_NATIVE)) {
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
                if (!accept_err(')')) {
                    return 0;
                }
                *ret = (Expr) { EXPR_TUPLE, .Tuple={n,vec} };

    // EXPR_PARENS
            } else if (!accept_err(')')) {
                return 0;
            }
        }

    // EXPR_NATIVE
    } else if (accept(TX_NATIVE)) {
        *ret = (Expr) { EXPR_NATIVE, .tk=ALL.tk0 };

    // EXPR_VAR
    } else if (accept(TX_VAR)) {
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
        } else if (accept('.')) {
            if (!accept_err(TX_INDEX)) {
                return 0;
            }
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


int parser_stmt_sub (Sub* ret) {
    if (!accept_err(TX_TYPE)) {
        return 0;
    }
    Tk id = ALL.tk0;                // True

    if (!accept_err(':')) {         // :
        return 0;
    }

    Type tp;
    if (!parser_type(&tp)) {        // ()
        return 0;
    }

    *ret = (Sub) { id, tp };
    return 1;
}

int parser_stmt_one (Stmt* ret) {
    // STMT_VAR
    if (accept(TK_VAL)) {
        if (!accept_err(TX_VAR)) {
            return 0;
        }
        Tk id = ALL.tk0;
        if (!accept_err(':')) {
            return 0;
        }
        Type tp;
        if (!parser_type(&tp)) {
            return 0;
        }
        if (!accept_err('=')) {
            return 0;
        }
        Expr e;
        if (!parser_expr(&e)) {
            return 0;
        }
        *ret = (Stmt) { STMT_VAR, .Var={id,tp,e} };

    // STMT_TYPE
    } else if (accept(TK_TYPE)) {       // type
        if (!accept_err(TX_TYPE)) {
            return 0;
        }
        Tk id = ALL.tk0;                // Bool

        if (!accept_err('{')) {
            return 0;
        }

        Sub s;
        if (!parser_stmt_sub(&s)) {     // False ()
            return 0;
        }
        int n = 1;
        Sub* vec = malloc(n*sizeof(Sub));
        vec[n-1] = s;

        while (1) {
            accept(';');    // optional
            Sub q;
            if (!parser_stmt_sub(&q)) { // True ()
                break;
            }
            n++;
            vec = realloc(vec, n*sizeof(Sub));
            vec[n-1] = q;
        }

        if (!accept_err('}')) {
            return 0;
        }

        *ret = (Stmt) { STMT_TYPE, .Type={id,n,vec} };

    // STMT_CALL
    } else if (accept(TK_CALL)) {
        Expr e;
        if (!parser_expr(&e)) {
            return 0;
        }
        *ret = (Stmt) { STMT_CALL, .call=e };
    } else {
        return 0;
    }

    return 1;
}

int parser_stmt (Stmt* ret) {
    Stmt s;
    if (!parser_stmt_one(&s)) {
        return 0;
    }

    int n = 1;
    Stmt* vec = malloc(n*sizeof(Stmt));
    vec[n-1] = s;

    while (1) {
        accept(';');    // optional
        Stmt q;
        if (!parser_stmt_one(&q)) {
            break;
        }
        n++;
        vec = realloc(vec, n*sizeof(Stmt));
        vec[n-1] = q;
    }

    if (n == 1) {
        *ret = s;
    } else {
        *ret = (Stmt) { STMT_SEQ, { .Seq={n,vec} } };
    }
    return 1;
}
