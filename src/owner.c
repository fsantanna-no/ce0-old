#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

///////////////////////////////////////////////////////////////////////////////

void env_txed_vars (Env* env, Expr* e, int* vars_n, Expr** vars) {
    assert((*vars_n) < 255);
    Type* TP = env_expr_to_type(env, e);
    if (!env_type_ishasrec(env,TP,0)) {
        return;
    }

    switch (e->sub) {
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
            vars[(*vars_n)++] = e;
            break;

        case EXPR_DNREF: {
            if (e->Dnref->sub == EXPR_VAR) {
                vars[(*vars_n)++] = e;
            }
            break;
        }

        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                env_txed_vars(env, e->Tuple.vec[i], vars_n, vars);
            }
            break;

        case EXPR_DISC:
            e->Disc.tx_setnull = 1;
            break;
        case EXPR_INDEX:
            e->Index.tx_setnull = 1;
            break;

        case EXPR_CALL: // tx for args is checked elsewhere (here, only if return type is also ishasrec)
            // func may transfer its argument back
            // set x.2 = f(x) where f: T1->T2 { return arg.2 }
            env_txed_vars(env, e->Call.arg, vars_n, vars);
            break;

        case EXPR_CONS: {
            env_txed_vars(env, e->Cons.arg, vars_n, vars);
            break;
        }

        default:
            break;
    }
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

    if (!env_type_ishasrec(S->env, S->Var.type,0)) {
        return 1;               // not recursive type
    }

    // STMT_VAR: recursive user type

    // var x: Nat = ...         // starting from each declaration

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

    typedef struct { Stmt* stop; int bws_n; } Stack;
    Stack stack[256];
    int stack_n = 0;

    void pre (void) {
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
    return exec(S->seqs[1], pre, fs);

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
            stack[stack_n].stop  = s->seqs[0];
//printf("[%d] +++ bws = %d (me=%d/%d/%p) (stop=%d/%d/%p)\n", stack_n, bws_n, s->tk.lin,s->sub,s, s->seqs[0]->tk.lin, s->seqs[0]->sub, s->seqs[0]);
//dump_stmt(s->seqs[0]);
            stack[stack_n].bws_n = bws_n;
            assert(stack_n < 256);
            stack_n++;
        }
        while (s == stack[stack_n-1].stop) {   // leave block: pop state
            stack_n--;
            bws_n = stack[stack_n].bws_n;
//printf("[%d] --- bws = %d [%p]\n", stack_n, bws_n, s);
        }

        // Add y/z in bws:
        //  var y = ... \x ...
        //  set z = ... y ...
        void add_bws (Expr* e) {
            int n=0; Expr* vars[256];
            env_held_vars(s->env, e, &n, vars);
            for (int i=0; i<n; i++) {
                Stmt* dcl = env_id_to_stmt(s->env, vars[i]->Var.tk.val.s);
                assert(dcl != NULL);
                if (bws_has(dcl)) {
                    bws[bws_n++] = s;
                    break;
                }
            }
        }

        // Set Var.tx_done
        //  var y = x
        int set_txs (Expr* e, int iscycle) {
            // var y = x
            int n=0; Expr* vars[256];
            env_txed_vars(s->env, e, &n, vars);
            for (int i=0; i<n; i++) {
                Expr* e = vars[i];
                if (e->sub == EXPR_DNREF) {
                    assert(e->Dnref->sub == EXPR_VAR);
                    char err[1024];
                    sprintf(err, "invalid dnref : cannot transfer value");
                    return err_message(&e->Dnref->Var.tk, err);
                } else {
                    assert(e->sub == EXPR_VAR);
                    if (!strcmp(e->Var.tk.val.s,S->Var.tk.val.s)) {
                        e->Var.tx_done = 1;
                        if (iscycle) {
                            char err[1024];
                            sprintf(err, "invalid assignment : cannot transfer ownsership to itself");
                            return err_message(&e->Var.tk, err);
                        }
                    }
                }
            }
            return 1;
        }

        // STMT_VAR, STMT_SET, STMT_RETURN transfer their source expressions.
        // EXPR_CALL, EXPR_CONS transfeir their argument (in any STMT_*).

        switch (s->sub) {
            case STMT_VAR:
                add_bws(s->Var.init);
                if (!set_txs(s->Var.init,0)) return EXEC_ERROR;
                goto __ACCS__;

            case STMT_SET: {
                int iscycle = 0; { // Rule 5: cycles can only occur in STMT_SET
                    Expr* dst = expr_leftmost(s->Set.dst);
                    assert(dst->sub == EXPR_VAR);
                    for (int i=0; i<bws_n; i++) {
                        if (!strcmp(dst->Var.tk.val.s,bws[i]->Var.tk.val.s)) {
                            iscycle = 1;
                            break;
                        }
                    }
                }
                add_bws(s->Set.src);
                if (!set_txs(s->Set.src,iscycle)) return EXEC_ERROR;

                goto __ACCS__;
            }

            case STMT_RETURN:
                if (!set_txs(s->Return,0)) return EXEC_ERROR;
                goto __ACCS__;

            case STMT_CALL:
            case STMT_IF:
__ACCS__:
            {
                int fs (Env* env, Expr* e) {
                    switch (e->sub) {
                        case EXPR_CALL:
                            if (!set_txs(e->Call.arg,0)) return EXEC_ERROR;
                            break;
                        case EXPR_CONS:
                            if (!set_txs(e->Cons.arg,0)) return EXEC_ERROR;
                            break;
                        case EXPR_VAR:
                            if (!strcmp(S->Var.tk.val.s,e->Var.tk.val.s)) {
                                // ensure that EXPR_VAR is really same as STMT_VAR
                                Stmt* decl = env_id_to_stmt(env, e->Var.tk.val.s);
                                assert(decl!=NULL && decl==S);

                                // Rule 6
                                if (bws_n >= 2) {
                                    if (e->Var.tx_done) {
                                        assert(txed_tk != NULL);
                                        char err[TK_BUF+256];
                                        sprintf(err, "invalid transfer of \"%s\" : active pointer in scope (ln %ld)",
                                                e->Var.tk.val.s, txed_tk->lin);
                                        err_message(&e->Var.tk, err);
                                        return VISIT_ERROR;
                                    }
                                }
                                txed_tk = &e->Var.tk;
                            }
                        default:
                            break;
                    }
                    return VISIT_CONTINUE;
                }
                int ret = visit_stmt(0, s, NULL, fs, NULL);
                if (ret != EXEC_CONTINUE) {
                    return ret;
                }
                break;
            }
            default:
                break;
        }
        return EXEC_CONTINUE;
    }
}

///////////////////////////////////////////////////////////////////////////////

int owner (Stmt* s) {
    if (!visit_stmt(0,s,check_txs,NULL,NULL)) {
        return 0;
    }
    return 1;
}
