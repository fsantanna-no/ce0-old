#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

static int MOVES[1024];
static int MOVES_N = 0;

///////////////////////////////////////////////////////////////////////////////

void env_txed_vars (Env* env, Expr* e, int* vars_n, Expr** vars) {
    assert((*vars_n) < 255);
    Type* TP __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e);
    if (!env_type_ishasrec(env,TP)) {
        return;
    }

    Expr* e_ = e;
    if (e->sub==EXPR_CALL && e->Call.func->sub==EXPR_VAR && !strcmp(e->Call.func->tk.val.s,"move")) {
        e = e->Call.arg;
    }

    if (e->sub==EXPR_VAR || e->sub==EXPR_DNREF || e->sub==EXPR_DISC || e->sub==EXPR_INDEX) {
        Expr* var = expr_leftmost(e);
        assert(var != NULL);
        assert(var->sub == EXPR_VAR);
        vars[(*vars_n)++] = e_;
    }

    switch (e->sub) {
        case EXPR_DNREF:
        case EXPR_NULL:
            break;
        case EXPR_UNIT:
        case EXPR_UNK:
        case EXPR_NATIVE:
        case EXPR_INT:
        case EXPR_PRED:
        case EXPR_UPREF:
            assert(0);      // cannot be ishasrec
            break;

        case EXPR_VAR:
            e->Var.tx_setnull = 1;
            break;

        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                env_txed_vars(env, e->Tuple.vec[i], vars_n, vars);
            }
            break;

        case EXPR_DISC:
            e->Disc.tx_setnull = 1;
            break;

        case EXPR_INDEX: {
            e->Index.tx_setnull = 1;
            break;
        }

        case EXPR_CALL: // tx for args is checked elsewhere (here, only if return type is also ishasrec)
            // func may transfer its argument back
            // set x.2 = f(x) where f: T1->T2 { return arg.2 }
            env_txed_vars(env, e->Call.arg, vars_n, vars);
            break;

        case EXPR_CONS: {
            env_txed_vars(env, e->Cons, vars_n, vars);
            break;
        }

        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////

// Set Var.tx_done
//  var y = x
int check_set_txs_all (Env* env, Expr* E, int iscycle) {
    int n=0; Expr* vars[256];
    env_txed_vars(env, E, &n, vars);
    for (int i=0; i<n; i++) {
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
        } else {
            Expr* e_ = expr_leftmost(e);
            assert(e_->sub == EXPR_VAR);
#if 0
            if (!strcmp(e_->tk.val.s,S->Var.tk.val.s)) {
                e_->Var.tx_done = 1;
                if (iscycle) {
                    char err[1024];
                    sprintf(err, "invalid assignment : cannot transfer ownsership to itself");
                    return err_message(&e_->tk, err);
                }
            }
#endif
        }
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

// How to detect ownership violations?
//  - Rule 4: transfer ownership and then access again:
//      var x: Nat = ...            -- owner
//      var z: Nat = add(x,...)     -- transfer
//      ... x or &x ...             -- error: access after ownership transfer
//  - Rule 6: save reference and then pass ownership:
//      var x: Nat = ...            -- owner
//      var z: Nat = &x             -- borrow
//      ... x ...                   -- error: transfer while borrow is active
//  - Rule 7: return alias pointing to local owner
//      func f: () -> &List {
//          var l: List = build()   -- `l` is the owner
//          return &l               -- error: cannot return alias to deallocated value
//      }
//
//  - Start from each STMT_VAR declaration: find all with `visit`.
//      - only STMT_VAR.isrec
//  - Call `exec` and stop when `env_id_to_stmt` fails.
//      - track EXPR_VAR acesses:
//          - transfer (istx)  // error if state=borrowed/moved // set state=moved
//          - borrow   (!istx) // error if state=moved          // set state=borrowed

int check_txs (Stmt* S) {

    // visit all var declarations

    if (S->sub != STMT_VAR) {
        return 1;               // not var declaration
    }

    if (!env_type_ishasrec(S->env, S->Var.type)) {
        return 1;               // not recursive type
    }

    // STMT_VAR: recursive user type

    // var x: Nat = ...         // starting from each declaration

    // Tracks if x has been transferred:
    //      var y: Nat = x
    //  - once trasferred, never fallback
    //  - reject further accesses
    int istxd = 0;

    // Tracks borrows of x (who I am borrowed to):
    //      var y: \Nat = \x
    //      var z: (Int,\Nat) = (1,\y)  -- x is borrowed by both y and z
    //  - if x has active borrows (bws_n > 0), then it cannot be transferred
    //  - clean bws on block exit
    Stmt* bws[256] = { S };
    int bws_n = 1;
    int bws_has (Stmt* s) {
        for (int i=0; i<bws_n; i++) {
            if (bws[i] == s) {
                return 1;
            }
        }
        return 0;
    }

    Tk* txed_tk = NULL;

    typedef struct { int depth_stop; int bws_n; } Stack;
    Stack stack[256];
    int stack_n = 0;

    void pre (void) {
        istxd = 0;
        bws_n = 1;
        bws[0] = S;
        txed_tk = NULL;
        stack_n = 0;
    }

    // S.Var.init.tx_setnull = ?
    {
        int n=0; Expr* vars[256];
        env_txed_vars(S->env, S->Var.init, &n, vars);
    }

    auto int fs (Stmt* s);
//printf(">>> %s\n", S->Var.tk.val.s);
//return 1;
    return exec(S->seq, pre, fs, NULL);

    // y = \x ...                // check all accesses to it

    int fs (Stmt* s) {
        // stop when tracked STMT_VAR is out of scope
        Stmt* decl = env_id_to_stmt(s->env, S->Var.tk.val.s);
        if (decl!=NULL && decl==S) {
            // ok: S is still in scope
        } else {
            return EXEC_HALT;           // no: S is not in scope
        }

        // push/pop stack
        if (s->sub == STMT_BLOCK) {         // enter block: push state
            stack[stack_n].depth_stop = s->env->depth;
//printf("+++ %d\n", s->env->depth);
            stack[stack_n].bws_n = bws_n;
            assert(stack_n < 256);
            stack_n++;
        } else {
            while (stack_n>0 && s->env->depth<=stack[stack_n-1].depth_stop) {   // leave block: pop state
//printf("--- %d\n", s->env->depth);
                stack_n--;
                bws_n = stack[stack_n].bws_n;
            }
        }

        auto void add_bws (Expr* e);
        auto int check_set_txs_seq (Expr* E, int iscycle);
        auto int fe (Env* env, Expr* e);

#if 0
        int expr_is_prefix (Expr* e1, Expr* e2) {
            if (e2->sub == EXPR_CALL) {
                assert(e2->Call.func->sub==EXPR_VAR && !strcmp(e2->Call.func->tk.val.s,"move"));
                e2 = e2->Call.arg;
            }
            if (e1->sub == EXPR_VAR) {
                Expr* e2_ = expr_leftmost(e2);
dump_expr(e2); puts(" <<<");
                assert(e2_->sub == EXPR_VAR);
                return !strcmp(e1->tk.val.s,e2->tk.val.s);
            }
            if (e1->sub != e2->sub) {
                return 0;
            }
            switch (e1->sub) {
                case EXPR_VAR:
                    assert(0);      // handled above
                case EXPR_DNREF:
                    return expr_is_prefix(e1->Dnref, e2->Dnref);
                case EXPR_INDEX:
                    return (e1->tk.val.n==e2->tk.val.n &&
                            expr_is_prefix(e1->Index.val,e2->Index.val));
                case EXPR_DISC:
                    return (!strcmp(e1->tk.val.s,e2->tk.val.s) &&
                            expr_is_prefix(e1->Disc.val,e2->Disc.val));
                default:
                    assert(0);      // impossible in an attribution
            }
            assert(0);
        }
#endif

        switch (s->sub) {
            case STMT_VAR:
                add_bws(s->Var.init);
                if (!check_set_txs_seq(s->Var.init,0)) return EXEC_ERROR;
                goto __ACCS__;

            case STMT_SET: {
#if 0
                int iscycle = 1;
puts("-=-=-=-");
                {
                    int n=0; Expr* vars[256];
                    env_txed_vars(s->env, s->Set.src, &n, vars);
                    for (int i=0; i<n; i++) {
dump_expr(s->Set.dst); printf(" vs "); dump_expr(vars[i]); puts(" <<<");
                        if (expr_is_prefix(s->Set.dst, vars[i])) {
                            iscycle = 0;
                            break;
                        }
                    }
                }
#else
                int iscycle = 0;
                if (s->Set.dst->sub != EXPR_VAR) { // not cycle if root transfer (x = x.Item!)
                    // Rule 5: cycles can only occur in STMT_SET
                    Expr* dst = expr_leftmost(s->Set.dst);
                    assert(dst->sub == EXPR_VAR);
//dump_stmt(s);
                    for (int i=0; i<bws_n; i++) {
                        if (!strcmp(dst->tk.val.s,bws[i]->Var.tk.val.s)) {
//printf(">>> %s\n", bws[i]->Var.tk.val.s);
                            iscycle = 1;
                            break;
                        }
                    }
                }
#endif
                if (!iscycle) {     // TODO: only in `growable´ mode
                    add_bws(s->Set.src);
                }
                if (!check_set_txs_seq(s->Set.src,iscycle)) return EXEC_ERROR;

                goto __ACCS__;
            }

            case STMT_IF: {
                int ret = visit_expr(0, s->env, s->If.tst, fe);
                if (ret != EXEC_CONTINUE) {
                    return ret;
                }
                break;
            }

            case STMT_CALL:
__ACCS__:
            {
                int ret = visit_stmt(0, s, NULL, fe, NULL);
                if (ret != EXEC_CONTINUE) {
                    return ret;
                }
                break;
            }
            default:
                break;
        }
        return EXEC_CONTINUE;

        // Add y/z in bws:
        //  var y = ... \x ...
        //  set z = ... y ...
        void add_bws (Expr* e) {
            int n=0; Expr* vars[256]; int uprefs[256];
            env_held_vars(s->env, e, &n, vars, uprefs);
            for (int i=0; i<n; i++) {
                Stmt* dcl = env_id_to_stmt(s->env, vars[i]->tk.val.s);
                assert(dcl != NULL);
                if (bws_has(dcl)) { // indirect alias
                    bws[bws_n++] = s;
                    break;
                }
            }
        }

        // Set Var.tx_done
        //  var y = x
        int check_set_txs_seq (Expr* E, int iscycle) {
            int n=0; Expr* vars[256];
            env_txed_vars(s->env, E, &n, vars);
            for (int i=0; i<n; i++) {
                Expr* e = vars[i];
                Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(s->env, e);
                int ishasptr = env_type_ishasptr(s->env, tp);
                int ismove = (e->sub==EXPR_CALL && e->Call.func->sub==EXPR_VAR && !strcmp(e->Call.func->tk.val.s,"move"));
                if (ismove) {
                    e = e->Call.arg;
                } else {
assert(0);
                }

                if (ishasptr && (e->sub==EXPR_DISC || e->sub==EXPR_INDEX)) {  // TODO: only in `growable´ mode
assert(0);
                } else if (e->sub == EXPR_DNREF) {
assert(0);
                } else {
                    Expr* e_ = expr_leftmost(e);
                    assert(e_->sub == EXPR_VAR);
                    if (!strcmp(e_->tk.val.s,S->Var.tk.val.s)) {
                        e_->Var.tx_done = 1;
                        if (iscycle) {
                            char err[1024];
                            sprintf(err, "invalid assignment : cannot transfer ownsership to itself");
                            return err_message(&e_->tk, err);
                        }
                    }
                }
            }
            return 1;
        }

        // STMT_VAR, STMT_SET, STMT_RETURN transfer their source expressions.
        // EXPR_CALL, EXPR_CONS transfeir their argument (in any STMT_*).

        int fe (Env* env, Expr* e) {
            switch (e->sub) {
                case EXPR_CALL:
                    //assert(e->Call.func->sub == EXPR_VAR);
                    if (!strcmp(e->Call.func->tk.val.s,"move")) break;
                    if (!check_set_txs_seq(e->Call.arg,0)) return EXEC_ERROR;
                    break;
                case EXPR_CONS:
                    if (!check_set_txs_seq(e->Cons,0)) return EXEC_ERROR;
                    break;
                case EXPR_VAR:
                    if (!strcmp(S->Var.tk.val.s,e->tk.val.s)) {
                        // ensure that EXPR_VAR is really same as STMT_VAR
                        Stmt* decl = env_id_to_stmt(env, e->tk.val.s);
                        assert(decl!=NULL && decl==S);

                        if (istxd) { // Rule 4 (growable)
                            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e);
                            int ishasptr = env_type_ishasptr(env, tp);
                            // if already moved, it doesn't matter, any access is invalid
                            if (ishasptr) {
                                assert(txed_tk != NULL);
                                char err[TK_BUF+256];
                                sprintf(err, "invalid access to \"%s\" : ownership was transferred (ln %d)",
                                        e->tk.val.s, txed_tk->lin);
                                err_message(&e->tk, err);
                                return VISIT_ERROR;
                            }
                            return VISIT_CONTINUE;

                        // Rule 6
                        } else if (bws_n >= 2) {
                            if (e->Var.tx_done) {
                                int lin = (txed_tk == NULL) ? e->tk.lin : txed_tk->lin;
                                char err[TK_BUF+256];
                                sprintf(err, "invalid transfer of \"%s\" : active pointer in scope (ln %d)",
                                        e->tk.val.s, lin);
// ignorar os bws em si mesmo no modo growable
                                err_message(&e->tk, err);
                                return VISIT_ERROR;
                            }
                        }
                        txed_tk = &e->tk;
                        if (e->Var.tx_done) {
                            istxd = 1;
                        }
                    }
                default:
                    break;
            }
            return VISIT_CONTINUE;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

int owner (Stmt* s) {
    MOVES_N = 0;

// TODO
int iscycle = 0;

    // SET_TXS
    {
        int fs (Stmt* s) {
            switch (s->sub) {
                case STMT_VAR:
                    if (!check_set_txs_all(s->env, s->Var.init,0)) {
                        return VISIT_ERROR;
                    }
                    break;
                case STMT_SET:
                    if (!check_set_txs_all(s->env, s->Set.src,iscycle)) {
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
                        if (!check_set_txs_all(env, e->Call.arg,0)) {
                            return VISIT_ERROR;
                        }
                    }
                    break;
                case EXPR_CONS:
                    if (!check_set_txs_all(env, e->Cons,0)) {
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

    // CHECK_TXS
    if (!visit_stmt(0,s,check_txs,NULL,NULL)) {
        return 0;
    }
    return 1;
}
