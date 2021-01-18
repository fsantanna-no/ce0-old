#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

///////////////////////////////////////////////////////////////////////////////

void set_tx (Env* env, Expr* E, int cantx) {
    Type* tp = env_expr_to_type(env, (E->sub==EXPR_UPREF ? E->Upref : E));
    assert(tp != NULL);
    if (env_type_ishasrec(env,tp,0)) {
        if (cantx) {
            E->istx = 1;
        }

        Expr* left = expr_leftmost(E);
        assert(left != NULL);
        if (left->sub == EXPR_VAR) {
            if (E->sub!=EXPR_UPREF && E==left && cantx) { // TX only for root vars (E==left)
                left->Var.istx = 1;
            }
        } else {
            // ok
        }
    }
}

int set_istx_expr (Env* env, Expr* e, int cantx) {
    set_tx(env, e, cantx);

    switch (e->sub) {
        case EXPR_UPREF:
            set_istx_expr(env, e->Upref, 0);
            break;

        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                set_istx_expr(env, e->Tuple.vec[i], cantx);
            }
            break;

        case EXPR_INDEX:
            set_istx_expr(env, e->Index.val, 0);
            break;

        case EXPR_CALL:
            set_istx_expr(env, e->Call.func, 0);
            set_istx_expr(env, e->Call.arg, 1);
            break;

        case EXPR_CONS:
            set_istx_expr(env, e->Cons.arg, 1);
            break;

        case EXPR_DISC:
            set_istx_expr(env, e->Disc.val, 0);
            break;

        case EXPR_PRED:
            set_istx_expr(env, e->Pred.val, 0);
            break;

        default:
            // istx = 0
            break;
    }
    return VISIT_CONTINUE;
}

int set_istx_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_VAR:
            set_istx_expr(s->env, s->Var.init, 1);
            break;
        case STMT_SET:
            set_istx_expr(s->env, s->Set.src, 1);
            break;
        case STMT_CALL:
            set_istx_expr(s->env, s->Call, 0);
            break;
        case STMT_IF:
            set_istx_expr(s->env, s->If.tst, 0);
            break;
        case STMT_RETURN:
            set_istx_expr(s->env, s->Return, 1);
            break;
        default:
            // istx = 0
            break;
    }
    return VISIT_CONTINUE;
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

int FS (Stmt* S) {

    // visit all var declarations

    if (S->sub != STMT_VAR) {
        return 1;               // not var declaration
    }

    if (!env_type_ishasrec(S->env, S->Var.type,0)) {
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

    Tk* tk1 = NULL;

    typedef struct { Stmt* stop; int bws_n; } Stack;
    Stack stack[256];
    int stack_n = 0;

    void pre (void) {
        istxd = 0;
        bws_n = 1;
        bws[0] = S;
        tk1 = NULL;
        stack_n = 0;
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
            stack[stack_n].bws_n = bws_n;
            assert(stack_n < 256);
            stack_n++;
        }
        if (s == stack[stack_n-1].stop) {   // leave block: pop state
            stack_n--;
            bws_n = stack[stack_n].bws_n;
        }

        // Rules 4/5/6
        auto int var_access (Env* env, Expr* var);

        // Add y/z in bws:
        //  var y = ... \x ...
        //  set z = ... y ...

        switch (s->sub) {
            case STMT_VAR: {
                int n=0; Expr* vars[256];
                env_held_vars(s->env, s->Var.init, &n, vars);
                for (int i=0; i<n; i++) {
                    Stmt* dcl = env_id_to_stmt(s->env, vars[i]->Var.tk.val.s);
                    assert(dcl != NULL);
                    if (bws_has(dcl)) {
                        bws[bws_n++] = s;   // var y = ... \x ...
                        break;
                    }
                }
                goto __VAR_ACCESS__;
                break;
            }
            case STMT_SET: {
                Type* tp = env_expr_to_type(s->env, s->Set.dst);
                if (!env_type_ishasptr(s->env,tp)) {
                    goto __VAR_ACCESS__;
                    break;
                }

                Stmt* dst = env_expr_leftmost_decl(s->env, s->Set.dst);
                assert(dst != NULL);
                assert(dst->sub == STMT_VAR);

                int n=0; Expr* vars[256];
                env_held_vars(s->env, s->Set.src, &n, vars);
                for (int i=0; i<n; i++) {
                    Stmt* dcl = env_id_to_stmt(s->env, vars[i]->Var.tk.val.s);
                    assert(dcl != NULL);
                    if (bws_has(dcl)) {
                        bws[bws_n++] = dst;   // set z = ... \x ...
                        break;
                    }
                }

                goto __VAR_ACCESS__;
                break;
            }

            //case STMT_VAR:
            //case STMT_SET:
            case STMT_CALL:
            case STMT_IF:
            case STMT_RETURN:
__VAR_ACCESS__: {
                int ret = visit_stmt(0, s, NULL, var_access, NULL);
                if (ret != EXEC_CONTINUE) {
                    return ret;
                }
                break;
            }
            default:
                break;
        }
        return EXEC_CONTINUE;

        int var_access (Env* env, Expr* var) {
            if (var->sub != EXPR_VAR) {
                return VISIT_CONTINUE;
            }
            if (strcmp(S->Var.tk.val.s,var->Var.tk.val.s)) {
                return VISIT_CONTINUE;
            }
            // ensure that EXPR_VAR is really same as STMT_VAR
            Stmt* decl = env_id_to_stmt(env, var->Var.tk.val.s);
            assert(decl!=NULL && decl==S);

            if (istxd) { // Rule 4
                // if already moved, it doesn't matter, any access is invalid
                assert(tk1 != NULL);
                char err[1024];
                sprintf(err, "invalid access to \"%s\" : ownership was transferred (ln %ld)",
                        var->Var.tk.val.s, tk1->lin);
                err_message(&var->Var.tk, err);
                return VISIT_ERROR;
            } else if (bws_n >= 2) {    // Rule 6
#if 0
                int isbw = 0;
                for (int i=0; i<bws_n; i++) {
                    if (bws[i] == var) {
                        isbw = 1;
                    }
                }
                if (!isbw) {
#endif
                if (var->Var.istx) {
                    assert(tk1 != NULL);
                    char err[1024];
                    sprintf(err, "invalid transfer of \"%s\" : active pointer in scope (ln %ld)",
                            var->Var.tk.val.s, tk1->lin);
                    err_message(&var->Var.tk, err);
                    return VISIT_ERROR;
                }
            }
            tk1 = &var->Var.tk;
            if (var->Var.istx) {
                istxd = 1;
            }
            return VISIT_CONTINUE;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

int owner (Stmt* s) {
    assert(visit_stmt(0,s,set_istx_stmt,NULL,NULL));
    if (!visit_stmt(0,s,FS,NULL,NULL)) {
        return 0;
    }
    return 1;
}
