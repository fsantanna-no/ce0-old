#include <assert.h>
#include <stdlib.h>

#include "all.h"

int _N_ = 2;    // TODO: see env.c _N_

int err_expected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): expected %s : have %s",
        ALL.tk1.lin, ALL.tk1.col, v, lexer_tk2str(&ALL.tk1));
    return 0;
}

int err_unexpected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): unexpected %s : %s",
        ALL.tk1.lin, ALL.tk1.col, lexer_tk2str(&ALL.tk1), v);
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

int check_err (TK enu) {
    int ret = check(enu);
    if (ret == 0) {
        err_expected(lexer_tk2err(enu));
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

int parser_type (Type* ret) {
    int isalias = accept('&');

// TYPE_UNIT
    if (accept(TK_UNIT)) {
        *ret = (Type) { TYPE_UNIT, isalias };

// TYPE_TUPLE
    } else if (accept('(')) {
        if (!parser_type(ret)) {
            return 0;
        }
        if (!accept_err(',')) {
            return 0;
        }

        int n = 1;
        Type* vec = malloc(n*sizeof(Type));
        assert(vec != NULL);
        vec[n-1] = *ret;

        do {
            Type tp;
            if (!parser_type(&tp)) {
                return 0;
            }
            n++;
            vec = realloc(vec, n*sizeof(Type));
            vec[n-1] = tp;
        } while (accept(','));

        if (!accept_err(')')) {
            return 0;
        }
        *ret = (Type) { TYPE_TUPLE, isalias, .Tuple={n,vec} };

    // TYPE_NATIVE
    } else if (accept(TX_NATIVE)) {
        *ret = (Type) { TYPE_NATIVE, isalias, .Native=ALL.tk0 };

    // TYPE_USER
    } else if (accept(TX_USER)) {
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

int parser_expr_0 (void) {
    return accept(TK_UNIT) || accept(TX_NATIVE) || accept(TX_VAR) || accept(TX_NULL)
            || err_expected("simple expression");
}

EXPR tk_to_expr (Tk* tk) {
    switch (tk->enu) {
        case TK_UNIT:   return EXPR_UNIT;
        case TX_NATIVE: return EXPR_NATIVE;
        case TX_VAR:    return EXPR_VAR;
        case TX_NULL:   return EXPR_NULL;
        default: assert(0);
    }
}

int parser_expr_1 (Expr* ret) {
// EXPR_ALIAS
    if (accept('&')) {
        if (!check_err(TX_VAR) || !parser_expr_1(ret)) {
            return 0;
        }
        assert(ret->sub==EXPR_VAR || ret->sub==EXPR_INDEX || ret->sub==EXPR_DISC);
        ret->isalias = 1;
        return 1;

// EXPR_TUPLE
    } else if (accept('(')) {
        if (!parser_expr_0()) {
            return 0;
        }
        Tk tk = ALL.tk0;
        if (!accept_err(',')) {
            return 0;
        }

        int n = 1;
        Tk* vec = malloc(n*sizeof(Expr));
        assert(vec != NULL);
        vec[n-1] = tk;

        do {
            if (!parser_expr_0()) {
                return 0;
            }
            n++;
            vec = realloc(vec, n*sizeof(Expr));
            vec[n-1] = ALL.tk0;
        } while (accept(','));

        if (!accept_err(')')) {
            return 0;
        }
        *ret = (Expr) { _N_++, EXPR_TUPLE, 0, .Tuple={n,vec} };
        return 1;

// EXPR_CONS
    } else if (accept(TX_USER)) {  // True
        Tk subtype = ALL.tk0;
        if (!parser_expr_0()) { // ()
            return 0;
        }
        *ret = (Expr) { _N_++, EXPR_CONS, 0, { .Cons={subtype,ALL.tk0} } };
        return 1;

// EXPR_INDEX / EXPR_PRED / EXPR_DISC / EXPR_CALL
    } else if (check(TX_VAR) || check(TX_NATIVE)) {
        assert(parser_expr_0());
        Tk var = ALL.tk0;

// EXPR_INDEX / EXPR_PRED / EXPR_DISC
        if (accept('.')) {

// EXPR_INDEX
            if (accept(TX_NUM)) {
                *ret = (Expr) { _N_++, EXPR_INDEX, 0, .Index={var,ALL.tk0} };
                return 1;

// EXPR_PRED / EXPR_DISC
            } else if (accept(TX_USER) || accept(TX_NULL)) {
                Tk sub = ALL.tk0;
                if (accept('?')) {
                    *ret = (Expr) { _N_++, EXPR_PRED, 0, .Disc={var,sub} };
                } else if (accept('!')) {
                    *ret = (Expr) { _N_++, EXPR_DISC, 0, .Disc={var,sub} };
                } else {
                    return err_expected("`?´ or `!´");
                }
                return 1;
            } else {
                return err_expected("index or subtype");
            }

// EXPR_CALL
        } else {
            if (parser_expr_0()) {
                *ret = (Expr) { _N_++, EXPR_CALL, 0, .Call={var,ALL.tk0} };
                return 1;

// EXP0
            } else {
                assert(var.enu==TX_VAR || var.enu==TX_NATIVE);
                int sub = (var.enu == TX_VAR ? EXPR_VAR : EXPR_NATIVE);
                *ret = (Expr) { _N_++, sub, 0, .tk=var };
                return 1;
            }
        }

    } else {
        if (!parser_expr_0()) {
            return 0;
        }
        *ret = (Expr) { _N_++, tk_to_expr(&ALL.tk0), 0, .tk=ALL.tk0 };
        return 1;
    }
    assert(0);
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

        Stmt* ss = malloc(sizeof(Stmt));
        assert(ss != NULL);
        if (!parser_stmts('}',ss)) {
            return 0;
        }

        if (!accept_err('}')) { return 0; }
        *ret = (Stmt) { _N_++, STMT_BLOCK, NULL, {NULL,NULL}, tk, .Block=ss };
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
        Expr e;
        if (!parser_expr_1(&e)) {
            return 0;
        }
        *ret = (Stmt) { _N_++, STMT_VAR, NULL, {NULL,NULL}, tk, .Var={id,tp,e} };

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

        *ret = (Stmt) { _N_++, STMT_USER, NULL, {NULL,NULL}, tk, .User={isrec,id,n,vec} };

    // STMT_CALL
    } else if (accept(TK_CALL)) {
        Tk tk = ALL.tk1;
        Expr e;
        if (!parser_expr_1(&e)) {
            return 0;
        }
        if (e.sub != EXPR_CALL) {
//assert(0);
            ALL.tk1.enu = TK_ERR;  // workaround to fail enclosing '}'/EOF
            sprintf(ALL.err, "(ln %ld, col %ld): expected call expression : have %s",
                tk.lin, tk.col, lexer_tk2str(&tk));
            return 0;
        }
        *ret = (Stmt) { _N_++, STMT_CALL, NULL, {NULL,NULL}, tk, .Call=e };

    // STMT_IF
    } else if (accept(TK_IF)) {         // if
        Tk tk = ALL.tk0;

        if (!parser_expr_0()) {         // x
            return 0;
        }
        Tk cond = ALL.tk0;

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
            *seq = (Stmt) { _N_++, STMT_SEQ,   NULL, {NULL,NULL}, ALL.tk0, .Seq={0,NULL} };
            *f   = (Stmt) { _N_++, STMT_BLOCK, NULL, {NULL,NULL}, ALL.tk0, .Block=seq };
        }

        *ret = (Stmt) { _N_++, STMT_IF, NULL, {NULL,NULL}, tk, .If={cond,t,f} };

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

        Stmt* vec = malloc(2*sizeof(Stmt));
            assert(vec != NULL);

        vec[0] = (Stmt) {
            _N_++, STMT_VAR, NULL, {NULL,NULL},
            .Var = {
                { TX_VAR, {.s="arg"}, id.lin, id.col },
                *tp.Func.inp,
                { _N_++, EXPR_NATIVE, 0, {.tk={TX_NATIVE,{.s="_arg_"},id.lin,id.col}} }
            }
        };

        err_expected("{");
        if (!parser_block(&vec[1])) {
            return 0;
        }

        Stmt* ss = malloc(sizeof(Stmt));
            assert(ss != NULL);
            *ss = (Stmt) { _N_++, STMT_SEQ, NULL, {NULL,NULL}, tk, { .Seq={2,vec} } };

        Stmt* blk1 = malloc(sizeof(Stmt)); // (arg)
            assert(blk1 != NULL);
            *blk1 = (Stmt) { _N_++, STMT_BLOCK, NULL, {NULL,NULL}, tk, .Block=ss };

        *ret = (Stmt) { _N_++, STMT_FUNC, NULL, {NULL,NULL}, tk, .Func={id,tp,blk1} };

    // STMT_RETURN
    } else if (accept(TK_RETURN)) {
        Tk tk = ALL.tk0;
        if (!parser_expr_0()) {
            return 0;
        }
        *ret = (Stmt) { _N_++, STMT_RETURN, NULL, {NULL,NULL}, tk, .Return=ALL.tk0 };

    // STMT_BLOCK
    } else if (check('{')) {
        return parser_block(ret);

    } else {
        return err_expected("statement");
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

    *ret = (Stmt) { _N_++, STMT_SEQ, NULL, {NULL,NULL}, ALL.tk0, { .Seq={n,vec} } };

    // STMT_SEQ.seqs of its children

    void set_seq (Stmt* cur, Stmt* nxt) {
        cur->seqs[0] = nxt;
        switch (cur->sub) {
            case STMT_IF:
                set_seq(cur->If.true, nxt);
                set_seq(cur->If.false, nxt);
                cur->seqs[1] = NULL; // undetermined: user code has to determine
                break;
            case STMT_SEQ:
                if (cur->Seq.size == 0) {
                    cur->seqs[1] = nxt;                             // Stmt -> nxt
                } else {
                    cur->seqs[1] = &cur->Seq.vec[0];                // Stmt    -> Stmt[0]
                    cur->Seq.vec[cur->Seq.size-1].seqs[1] = nxt;    // Stmt[n] -> nxt
                }
                break;
            case STMT_BLOCK:
                set_seq(cur->Block, nxt);
                cur->seqs[1] = cur->Block;
                break;
            default:
                cur->seqs[1] = nxt;
        }
    }

    for (int i=0; i<ret->Seq.size-1; i++) {
        Stmt* cur = &ret->Seq.vec[i];
        Stmt* nxt = &ret->Seq.vec[i+1];
        set_seq(cur, nxt);                          // Stmt[i] -> Stmt[i+1]
    }

    return 1;
}

int parser (Stmt** ret) {
    *ret = malloc(sizeof(Stmt));
    assert(*ret != NULL);

    if (!parser_stmts(TK_EOF,*ret)) {
        return 0;
    }
    if (!accept_err(TK_EOF)) {
        return 0;
    }
    return 1;
}
