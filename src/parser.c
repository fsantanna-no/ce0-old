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
                assert(vec != NULL);
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

    // TYPE_TYPE
    } else if (accept(TX_TYPE)) {
        *ret = (Type) { TYPE_TYPE, .tk=ALL.tk0 };

    } else {
        return err_expected("type");
    }

    // TYPE_FUNC
    if (accept(TK_ARROW)) {
        Type tp;
        if (!parser_type(&tp)) {
            return 0;
        }
        Type* inp = malloc(sizeof(Type));
        Type* out = malloc(sizeof(Type));
        assert(inp != NULL);
        assert(out != NULL);
        *inp = *ret;
        *out = tp;
        *ret = (Type) { TYPE_FUNC, .Func={inp,out} };
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

static int parser_expr_one (Env* env, Expr* ret) {
    // EXPR_UNIT
    if (accept('(')) {
        if (accept(')')) {
            *ret = (Expr) { EXPR_UNIT, env };
        } else {
            if (!parser_expr(env, ret)) {
                return 0;
            }

    // EXPR_TUPLE
            if (check(',')) {
                int n = 1;
                Expr* vec = malloc(n*sizeof(Expr));
                assert(vec != NULL);
                vec[n-1] = *ret;
                while (accept(',')) {
                    Expr e;
                    if (!parser_expr(env, &e)) {
                        return 0;
                    }
                    n++;
                    vec = realloc(vec, n*sizeof(Expr));
                    vec[n-1] = e;
                }
                if (!accept_err(')')) {
                    return 0;
                }
                *ret = (Expr) { EXPR_TUPLE, env, .Tuple={n,vec} };

    // EXPR_PARENS
            } else if (!accept_err(')')) {
                return 0;
            }
        }

    // EXPR_NULL
    } else if (accept('$')) {
        *ret = (Expr) { EXPR_NULL, env };

    // EXPR_ARG
    } else if (accept(TK_ARG)) {
        *ret = (Expr) { EXPR_ARG, env };

    // EXPR_NATIVE
    } else if (accept(TX_NATIVE)) {
        *ret = (Expr) { EXPR_NATIVE, env, .tk=ALL.tk0 };

    // EXPR_VAR
    } else if (accept(TX_VAR)) {
        *ret = (Expr) { EXPR_VAR, env, .tk=ALL.tk0 };

    // EXPR_CONS
    } else if (accept(TX_TYPE)) {       // Bool
        Tk type = ALL.tk0;

        if (!accept_err('.')) {         // .
            return 0;
        }

        if (!accept_err(TX_TYPE)) {     // True
            return 0;
        }
        Tk subtype = ALL.tk0;

        if (!check('(')) { // only checks, arg will accept
            return err_expected("`(´");
        }

        Expr* arg = malloc(sizeof(Expr));
        assert(arg != NULL);
        if (!parser_expr_one(env, arg)) {   // ()
            return 0;
        }

        *ret = (Expr) { EXPR_CONS, env, { .Cons={type,subtype,arg} } };

    } else {
        return err_expected("expression");
    }
    return 1;
}

int parser_expr (Env* env, Expr* ret) {
    Expr e;
    if (!parser_expr_one(env, &e)) {
        return 0;
    }
    *ret = e;

    while (1) {
    // EXPR_CALL
        if (check('(')) {                // only checks, arg will accept
            Expr* arg = malloc(sizeof(Expr));
            assert(arg != NULL);
            if (!parser_expr_one(env, arg)) {   // f().() and not f.()()
                return 0;
            }

            Expr* func = malloc(sizeof(Expr));
            assert(func != NULL);
            *func = *ret;
            *ret  = (Expr) { EXPR_CALL, env, .Call={func,arg} };
        } else if (accept('.')) {
    // EXPR_INDEX
            if (accept(TX_INDEX)) {
                Expr* tup = malloc(sizeof(Expr));
                assert(tup != NULL);
                *tup = *ret;
                *ret = (Expr) { EXPR_INDEX, env, .Index={tup,ALL.tk0.val.n} };
    // EXPR_DISC
            } else if (accept(TX_TYPE)) {
                Expr* cons = malloc(sizeof(Expr));
                assert(cons != NULL);
                *cons = *ret;
                if (accept('?')) {
                    *ret = (Expr) { EXPR_PRED, env, .Pred={cons,ALL.tk0} };
                } else if (accept('!')) {
                    *ret = (Expr) { EXPR_DISC, env, .Disc={cons,ALL.tk0} };
                } else {
                    return err_expected("`?´ or `!´");
                }
            } else {
                return err_expected("index or subtype");
            }
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

int parser_stmt (Env** env, Stmt* ret) {
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
        if (!parser_expr(*env, &e)) {
            return 0;
        }
        *ret = (Stmt) { STMT_VAR, *env, .Var={id,tp,e} };

        Env* new = malloc(sizeof(Env));
        *new = (Env) { ret, *env };
        *env = new;

    // STMT_TYPE
    } else if (accept(TK_TYPE)) {       // type
        int isrec = 0;
        if (accept(TK_REC)) {           // rec
            isrec = 1;
        }
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
        assert(vec != NULL);
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

        *ret = (Stmt) { STMT_TYPE, *env, .Type={isrec,id,n,vec} };

        Env* new = malloc(sizeof(Env));
        *new = (Env) { ret, *env };
        *env = new;

    // STMT_CALL
    } else if (accept(TK_CALL)) {
        Expr e;
        if (!parser_expr(*env, &e)) {
            return 0;
        }
        *ret = (Stmt) { STMT_CALL, *env, .call=e };

    // STMT_IF
    } else if (accept(TK_IF)) {         // if
        Expr e;
        if (!parser_expr(*env, &e)) {         // x
            return 0;
        }

        if (!accept_err('{')) { return 0; }

        Stmt* t = malloc(sizeof(Stmt));
        assert(t != NULL);
        Env* trash1 = *env;
        if (!parser_stmts(&trash1,t)) {         // true()
            return 0;
        }

        if (!accept_err('}')) { return 0; }

        Stmt* f = malloc(sizeof(Stmt));
        assert(f != NULL);
        if (accept(TK_ELSE)) {
            if (!accept_err('{')) { return 0; }
            Env* trash2 = *env;
            if (!parser_stmts(&trash2,f)) {     // false()
                return 0;
            }
            if (!accept_err('}')) { return 0; }
        } else {
            *f = (Stmt) { STMT_SEQ, *env, .Seq={0,NULL} };
        }

        *ret = (Stmt) { STMT_IF, *env, .If={e,t,f} };

    // STMT_FUNC
    } else if (accept(TK_FUNC)) {   // func
        if (!accept(TX_VAR)) {      // f
            return 0;
        }
        Tk id = ALL.tk0;
        if (!accept(':')) {         // :
            return 0;
        }
        Type tp;
        if (!parser_type(&tp)) {    // () -> ()
            return 0;
        }
        if (!accept('{')) { return 0; }

        Stmt* s = malloc(sizeof(Stmt)); // return ()
        assert(s != NULL);
        Env* trash = *env;
        if (!parser_stmts(&trash, s)) {
            return 0;
        }
        *ret = (Stmt) { STMT_FUNC, *env, .Func={tp,id,s} };

        if (!accept('}')) { return 0; }

    // STMT_RETURN
    } else if (accept(TK_RETURN)) {
        Expr e;
        if (!parser_expr(*env, &e)) {
            return 0;
        }
        *ret = (Stmt) { STMT_RETURN, *env, .ret=e };

    } else {
        return 0;
    }

    return 1;
}

int parser_stmts (Env** env, Stmt* ret) {
    int n = 0;
    Stmt* vec = NULL;

    while (1) {
        accept(';');    // optional
        Stmt q;
        if (!parser_stmt(env,&q)) {
            break;
        }
        n++;
        vec = realloc(vec, n*sizeof(Stmt));
        vec[n-1] = q;
        accept(';');    // optional
    }

    *ret = (Stmt) { STMT_SEQ, *env, { .Seq={n,vec} } };
    return 1;
}

int parser (Stmt* ret) {
    Env* env = NULL;
    if (!parser_stmts(&env,ret)) {
        return 0;
    }
    if (!accept_err(TK_EOF)) {
        return 0;
    }
    return 1;
}
