#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

static int MOVES[1024];
static int MOVES_N = 0;

///////////////////////////////////////////////////////////////////////////////

void txs_txed_vars (Env* env, Expr* e, F_txed_vars f) {
    Type* TP __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e);
    if (!env_type_ishasrec(env,TP)) {
        return;
    }

    Expr* e_ = e;
    if (e->sub==EXPR_CALL && e->Call.func->sub==EXPR_VAR && !strcmp(e->Call.func->tk.val.s,"move")) {
        e = e->Call.arg;
    }

    f(e_, e);

    switch (e->sub) {
        case EXPR_VAR:
        case EXPR_NULL:
        case EXPR_DNREF:
        case EXPR_DISC:
        case EXPR_INDEX:
            break;
        case EXPR_UNIT:
        case EXPR_UNK:
        case EXPR_NATIVE:
        case EXPR_INT:
        case EXPR_PRED:
        case EXPR_UPREF:
            assert(0);      // cannot be ishasrec
            break;

        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                txs_txed_vars(env, e->Tuple.vec[i], f);
            }
            break;

        case EXPR_CALL: // tx for args is checked elsewhere (here, only if return type is also ishasrec)
            // func may transfer its argument back
            // set x.2 = f(x) where f: T1->T2 { return arg.2 }
            txs_txed_vars(env, e->Call.arg, f);
            break;

        case EXPR_CONS: {
            txs_txed_vars(env, e->Cons, f);
            break;
        }

        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////

// Check transfers:
//  - missing `move` before transfer    (set x = move y)
//  - partial transfer in `growable`    (set x = move y.Item!.2)
//  - dnref transfer                    (set x = move y\)
int check_txs_exprs (Env* env, Expr* E) {

    int vars_n=0; Expr* vars[256];
    {
        void f_set_tx_setnull (Expr* e_, Expr* e) {
            switch (e->sub) {
                case EXPR_VAR:
                    e->Var.tx_setnull = 1;
                    break;
                case EXPR_DNREF:
                    //e->Dnref.tx_setnull = 1; // will fail in "invalid dnref" below
                    break;
                case EXPR_DISC:
                    e->Disc.tx_setnull = 1;
                    break;
                case EXPR_INDEX:
                    e->Index.tx_setnull = 1;
                    break;
                default:
                    break;
            }
        }
        void f_get_txs (Expr* e_, Expr* e) {
            assert(vars_n < 255);
            if (e->sub==EXPR_VAR || e->sub==EXPR_DNREF || e->sub==EXPR_DISC || e->sub==EXPR_INDEX) {
                Expr* var = expr_leftmost(e);
                assert(var != NULL);
                assert(var->sub == EXPR_VAR);
                vars[vars_n++] = e_;
            }
        }
        void f(Expr* e_, Expr* e) {
            f_set_tx_setnull(e_,e);
            f_get_txs(e_,e);
        }
        txs_txed_vars(env, E, f);
    }

    for (int i=0; i<vars_n; i++) {
        Expr* e = vars[i];
        Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e);
        int ishasptr = env_type_ishasptr(env, tp);
        int ismove = (e->sub==EXPR_CALL && e->Call.func->sub==EXPR_VAR && !strcmp(e->Call.func->tk.val.s,"move"));
        if (ismove) {
            int contains = 0;
            for (int i=0; i<MOVES_N; i++) {
                if (MOVES[i] == e->N) {
                    contains = 1;
                    break;
                }
            }
            if (!contains) {
                assert(MOVES_N < 1024);
                MOVES[MOVES_N++] = e->N;
            }
            e = e->Call.arg;
        } else {
            char err[1024];
            sprintf(err, "invalid ownership transfer : expected `move´ before expression");
            return err_message(&e->tk, err);
        }

        if (ishasptr && (e->sub==EXPR_DISC || e->sub==EXPR_INDEX)) {  // TODO: only in `growable´ mode
            char err[1024];
            sprintf(err, "invalid ownership transfer : mode `growable´ only allows root transfers");
            return err_message(&e->tk, err);
        } else if (e->sub == EXPR_DNREF) {
            assert(e->Dnref->sub == EXPR_VAR);
            char err[1024];
            sprintf(err, "invalid dnref : cannot transfer value");
            return err_message(&e->Dnref->tk, err);
        }
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int check_txs (Stmt* s) {
    MOVES_N = 0;

    // SET_TXS_ALL
    {
        int fs (Stmt* s) {
            switch (s->sub) {
                case STMT_VAR:
                    if (!check_txs_exprs(s->env, s->Var.init)) {
                        return VISIT_ERROR;
                    }
                    break;
                case STMT_SET:
                    if (!check_txs_exprs(s->env, s->Set.src)) {
                        return VISIT_ERROR;
                    }
                    break;
                default:
                    break;
            }
            return VISIT_CONTINUE;
        }
        int fe (Env* env, Expr* e) {
            switch (e->sub) {
                case EXPR_CALL:
                    if (strcmp(e->Call.func->tk.val.s,"move")) {
                        if (!check_txs_exprs(env, e->Call.arg)) {
                            return VISIT_ERROR;
                        }
                    }
                    break;
                case EXPR_CONS:
                    if (!check_txs_exprs(env, e->Cons)) {
                        return VISIT_ERROR;
                    }
                    break;
                default:
                    break;
            }
            return VISIT_CONTINUE;
        }
        if (!visit_stmt(0,s,fs,fe,NULL)) {
            return 0;
        }
    }

    // CHECK_MOVES
    {
        int fe (Env* env, Expr* e) {
            if (e->sub == EXPR_CALL && e->Call.func->sub == EXPR_VAR) {
                if (!strcmp(e->Call.func->tk.val.s,"move")) {
                    for (int i=0; i<MOVES_N; i++) {
                        if (MOVES[i] == e->N) {
                            return VISIT_CONTINUE;
                        }
                    }
                    char err[1024];
                    sprintf(err, "unexpected `move´ call : no ownership transfer");
                    return err_message(&e->tk, err);
                }
            }
            return VISIT_CONTINUE;
        }
        if (!visit_stmt(0,s,NULL,fe,NULL)) {
            return 0;
        }
    }

    return 1;
}
