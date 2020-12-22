#include <assert.h>
#include <stdlib.h>

#include "all.h"

int _N_ = 0;

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
            *ret = (Type) { TYPE_UNIT, 0 };
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
                *ret = (Type) { TYPE_TUPLE, 0, .Tuple={n,vec} };

    // TYPE_PARENS
            } else if (!accept_err(')')) {
                return 0;
            }
        }

    // TYPE_NATIVE
    } else if (accept(TX_NATIVE)) {
        *ret = (Type) { TYPE_NATIVE, 0, .Native=ALL.tk0 };

    // TYPE_USER
    } else if (accept('&') || accept(TX_USER)) {
        int isalias = (ALL.tk0.enu == '&');
        if (isalias && !accept_err(TX_USER)) {
            return 0;
        }
        *ret = (Type) { TYPE_USER, isalias, .User=ALL.tk0 };

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
        *ret = (Type) { TYPE_FUNC, 0, .Func={inp,out} };
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_expr_one (Exp1* ret) {
    // EXPR_UNIT
    if (accept('(')) {
        if (accept(')')) {
            *ret = (Exp1) { _N_++, EXPR_UNIT, 0 };
        } else {
            if (!parser_exp1(ret)) {
                return 0;
            }

    // EXPR_TUPLE
            if (check(',')) {
                int n = 1;
                Exp1* vec = malloc(n*sizeof(Exp1));
                assert(vec != NULL);
                vec[n-1] = *ret;
                while (accept(',')) {
                    Exp1 e;
                    if (!parser_exp1(&e)) {
                        return 0;
                    }
                    n++;
                    vec = realloc(vec, n*sizeof(Exp1));
                    vec[n-1] = e;
                }
                if (!accept_err(')')) {
                    return 0;
                }
                *ret = (Exp1) { _N_++, EXPR_TUPLE, 0, .Tuple={n,vec} };

    // EXPR_PARENS
            } else if (!accept_err(')')) {
                return 0;
            }
        }

    // EXPR_NATIVE
    } else if (accept(TX_NATIVE)) {
        *ret = (Exp1) { _N_++, EXPR_NATIVE, 0, .Native=ALL.tk0 };

    // EXPR_VAR
    } else if (accept('&') || accept(TK_ARG) || accept(TK_OUTPUT) || accept(TX_VAR)) {
        int isalias = (ALL.tk0.enu == '&');
        if (isalias) {
            if (!accept(TK_ARG) && !accept(TK_OUTPUT) && !accept_err(TX_VAR)) {
                return 0;
            }
            Exp1* e = malloc(sizeof(Exp1));
            assert(e != NULL);
            *e = (Exp1) { _N_++, EXPR_VAR, 0, .Var={ALL.tk0,0} };
            *ret = (Exp1) { _N_++, EXPR_ALIAS, .Alias=e };
        } else {
            *ret = (Exp1) { _N_++, EXPR_VAR, 0, .Var={ALL.tk0,0} };
        }

    // EXPR_CONS
    } else if (accept(TX_USER) || accept('$')) {  // True, $Nat
        if (ALL.tk0.enu == '$') {
            if (!accept_err(TX_USER)) {
                return 0;
            }
            ALL.tk0.enu = TX_NIL;   // TODO: move to lexer
        }
        Tk sub = ALL.tk0;

        Exp1* arg = malloc(sizeof(Exp1));
        assert(arg != NULL);
        if (sub.enu==TX_NIL || !parser_expr_one(arg)) {   // ()
            *arg = (Exp1) { _N_++, EXPR_UNIT, 0 };
        }

        *ret = (Exp1) { _N_++, EXPR_CONS, 0, { .Cons={sub,arg} } };

    } else {
        return err_expected("expression");
    }
    return 1;
}

int parser_exp1 (Exp1* ret) {
    if (!parser_expr_one(ret)) {
        return 0;
    }

    while (1) {
    // EXPR_CALL
        if (check('(')) {                // only checks, arg will accept
            Exp1* arg = malloc(sizeof(Exp1));
            assert(arg != NULL);
            if (!parser_expr_one(arg)) {   // f().() and not f.()()
                return 0;
            }

            Exp1* func = malloc(sizeof(Exp1));
            assert(func != NULL);
            *func = *ret;
            *ret  = (Exp1) { _N_++, EXPR_CALL, 0, .Call={func,arg} };
        } else if (accept('.')) {
    // EXPR_INDEX
            if (accept(TX_NUM)) {
                Exp1* tup = malloc(sizeof(Exp1));
                assert(tup != NULL);
                *tup = *ret;
                *ret = (Exp1) { _N_++, EXPR_INDEX, 0, .Index={tup,ALL.tk0.val.n} };
    // EXPR_DISC / EXPR_PRED
            } else if (accept(TX_USER) || accept('$')) {
                if (ALL.tk0.enu == '$') {
                    if (!accept_err(TX_USER)) {
                        return 0;
                    }
                    ALL.tk0.enu = TX_NIL;   // TODO: move to lexer
                }
                Tk tk = ALL.tk0;

                Exp1* val = malloc(sizeof(Exp1));
                assert(val != NULL);
                *val = *ret;
                if (accept('?')) {
                    *ret = (Exp1) { _N_++, EXPR_PRED, 0, .Pred={val,tk} };
                } else if (accept('!')) {
                    *ret = (Exp1) { _N_++, EXPR_DISC, 0, .Disc={val,tk} };
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
    if (!accept_err(TX_USER)) {
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

int parser_stmts (TK opt, Stmt* ret);

int parser_stmt (Stmt* ret) {
    int parser_block (Stmt* ret) {
        if (!accept('{')) {
            return 0;
        }
        Tk tk = ALL.tk0;

        Stmt* blk = malloc(sizeof(Stmt));
        assert(blk != NULL);
        if (!parser_stmts('}',blk)) {
            return 0;
        }

        if (!accept_err('}')) { return 0; }
        *ret = (Stmt) { _N_++, STMT_BLOCK, NULL, NULL, tk, .Block=blk };
        return 1;
    }

    // STMT_VAR
    if (accept(TK_VAR)) {
        Tk tk = ALL.tk0;
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
        Exp1 e;
        if (!parser_exp1(&e)) {
            return 0;
        }
        *ret = (Stmt) { _N_++, STMT_VAR, NULL, NULL, tk, .Var={id,tp,e} };

    // STMT_USER
    } else if (accept(TK_TYPE)) {       // type
        Tk tk = ALL.tk0;
        int isrec = 0;
        if (accept(TK_REC)) {           // rec
            isrec = 1;
        }
        if (!accept_err(TX_USER)) {
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

        *ret = (Stmt) { _N_++, STMT_USER, NULL, NULL, tk, .User={isrec,id,n,vec} };

    // STMT_CALL
    } else if (accept(TK_CALL)) {
        Tk tk = ALL.tk0;
        Exp1 e;
        if (!parser_exp1(&e)) {
            return 0;
        }
        *ret = (Stmt) { _N_++, STMT_CALL, NULL, NULL, tk, .Call=e };

    // STMT_IF
    } else if (accept(TK_IF)) {         // if
        Tk tk = ALL.tk0;
        Exp1 e;
        if (!parser_exp1(&e)) {         // x
            return 0;
        }

        Stmt* t = malloc(sizeof(Stmt));
        assert(t != NULL);
        err_expected("{");
        if (!parser_block(t)) {         // true()
            return 0;
        }

        Stmt* f = malloc(sizeof(Stmt));
        assert(f != NULL);
        if (accept(TK_ELSE)) {
            err_expected("{");
            if (!parser_block(f)) {     // false()
                return 0;
            }
        } else {
            Stmt* seq = malloc(sizeof(Stmt));
            assert(seq != NULL);
            *seq = (Stmt) { _N_++, STMT_SEQ,   NULL, NULL, ALL.tk0, .Seq={0,NULL} };
            *f   = (Stmt) { _N_++, STMT_BLOCK, NULL, NULL, ALL.tk0, .Block=seq };
        }

        *ret = (Stmt) { _N_++, STMT_IF, NULL, NULL, tk, .If={e,t,f} };

    // STMT_FUNC
    } else if (accept(TK_FUNC)) {   // func
        Tk tk = ALL.tk0;
        if (!accept(TX_VAR)) {    // f
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

        Stmt* s = malloc(sizeof(Stmt)); // return ()
        assert(s != NULL);
        err_expected("{");
        if (!parser_block(s)) {
            return 0;
        }
        *ret = (Stmt) { _N_++, STMT_FUNC, NULL, NULL, tk, .Func={id,tp,s} };

    // STMT_RETURN
    } else if (accept(TK_RETURN)) {
        Tk tk = ALL.tk0;
        Exp1 e;
        if (!parser_exp1(&e)) {
            return 0;
        }
        *ret = (Stmt) { _N_++, STMT_RETURN, NULL, NULL, tk, .Return=e };

    // STMT_BLOCK
    } else if (check('{')) {
        return parser_block(ret);

    } else {
        return err_expected("statement (maybe `call´?)");
    }

    return 1;
}

int parser_stmts (TK opt, Stmt* ret) {
    int n = 0;
    Stmt* vec = NULL;

    while (1) {
        accept(';');    // optional
        Stmt q;
        if (!parser_stmt(&q)) {
            if (check(opt)) {
                break;
            } else {
                return 0;
            }
        }
        n++;
        vec = realloc(vec, n*sizeof(Stmt));
        vec[n-1] = q;
        accept(';');    // optional
    }

    *ret = (Stmt) { _N_++, STMT_SEQ, NULL, NULL, ALL.tk0, { .Seq={n,vec} } };

    // STMT_SEQ.seq of its children

    void set_seq (Stmt* prv, Stmt* nxt) {
        switch (prv->sub) {
            case STMT_IF:
                set_seq(prv->If.true, nxt);
                set_seq(prv->If.false, nxt);
                prv->seq = NULL; // undetermined: user code has to determine
                break;
            case STMT_SEQ:
                if (prv->Seq.size == 0) {
                    prv->seq = nxt;                 // Stmt -> nxt
                } else {
                    prv->seq = &prv->Seq.vec[0];    // Stmt -> Stmt[0]
                }
                break;
            case STMT_BLOCK:
                set_seq(prv->Block, nxt);
                prv->seq = prv->Block;
                break;
            default:
                prv->seq = nxt;
        }
    }

    for (int i=0; i<ret->Seq.size-1; i++) {
        Stmt* prv = &ret->Seq.vec[i];
        Stmt* nxt = &ret->Seq.vec[i+1];
        set_seq(prv, nxt);                          // Stmt[i] -> Stmt[i+1]
    }

    return 1;
}

int parser (Stmt** ret) {
    static Type Type_Unit  = { TYPE_UNIT, 0 };

    Stmt* vec = malloc(3*sizeof(Stmt));
        assert(vec != NULL);

    Stmt* ss = malloc(sizeof(Stmt));
        assert(ss != NULL);
        *ss = (Stmt) { _N_++, STMT_SEQ, NULL, {NULL,NULL}, {}, { .Seq={3,vec} } };
    *ret = ss;

    // clone, output
    {
        static Type any   = { TYPE_NATIVE, 0, {.Native={TX_NATIVE,{.s="any"},0,0}} };
        static Type alias = { TYPE_NATIVE, 1, {.Native={TX_NATIVE,{.s="any"},0,0}} };
        vec[0] = (Stmt) {   // clone ()
            0, STMT_FUNC, NULL, {NULL,NULL},
            .Func = {
                { TX_VAR,{.s="clone"},0,0 },
                { TYPE_FUNC,.Func={&alias,&any} },
                NULL
            }
        };
        vec[1] = (Stmt) {   // output ()
            0, STMT_FUNC, NULL, {NULL,NULL},
            .Func = {
                { TX_VAR,{.s="output"},0,0 },
                { TYPE_FUNC,.Func={&alias,&Type_Unit} },
                NULL
            }
        };
    }

    if (!parser_stmts(TK_EOF,&vec[2])) {
        return 0;
    }
    if (!accept_err(TK_EOF)) {
        return 0;
    }
    return 1;
}
