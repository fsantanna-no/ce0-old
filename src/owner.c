#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

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
    int istx = 0;

    // Tracks borrows of x:
    //      var y: \Nat = \x
    //  - if active borrow (bws_n > 0), cannot be transferred
    //  - clean bws on block exit
    Stmt* bws[256] = {};
    int bws_n = 0;

    typedef enum { NONE, BORROWED } State;
    State state = NONE;
    Tk* tk1 = NULL;

    typedef struct { Stmt* stop; int state; int bws_n; } Stack;
    Stack stack[256];
    int stack_n = 0;

    auto int stmt_var (Stmt* s);

    void pre (void) {
        istx = 0;
        bws_n = 0;
        state = NONE;
        tk1 = NULL;
        stack_n = 0;
    }

    return exec(S->seqs[1], pre, stmt_var);

    // ... x ...                // check all accesses to it

    int stmt_var (Stmt* s)
    {
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
            stack[stack_n].state = state;
            stack[stack_n].bws_n = bws_n;
            assert(stack_n < 256);
            stack_n++;
        }

        if (s == stack[stack_n-1].stop) {   // leave block: pop state
            stack_n--;
            bws_n = stack[stack_n].bws_n;
            state = stack[stack_n].state;
        }

        // Rule 6/4: check var accesses starting from VAR/SET/CALL/IF/RETURN
        auto int var_access (Env* env, Expr* var);
        switch (s->sub) {
            case STMT_VAR:
            case STMT_SET:
            case STMT_CALL:
            case STMT_IF:
            case STMT_RETURN: {
                int ret = visit_stmt(0, s, NULL, var_access, NULL);
                if (ret != EXEC_CONTINUE) {
                    return ret;
                }
                break;
            }
            default:
                break;
        }

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

            if (istx) { // Rule 4
                // if already moved, it doesn't matter, any access is invalid
                assert(tk1 != NULL);
                char err[1024];
                sprintf(err, "invalid access to \"%s\" : ownership was transferred (ln %ld)",
                        var->Var.tk.val.s, tk1->lin);
                err_message(&var->Var.tk, err);
                return VISIT_ERROR;
            } else if (state == BORROWED) {    // Rule 6
                assert(tk1 != NULL);
                if (var->Var.txbw == TX) {
                    char err[1024];
                    sprintf(err, "invalid transfer of \"%s\" : active pointer in scope (ln %ld)",
                            var->Var.tk.val.s, tk1->lin);
                    err_message(&var->Var.tk, err);
                    return VISIT_ERROR;
                }
            }
            tk1 = &var->Var.tk;
            if (var->Var.txbw == BW) {
                assert(!istx && "bug found");
                state = BORROWED;
            } else if (var->Var.txbw == TX) {
                istx = 1;
            }
            return VISIT_CONTINUE;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

int owner (Stmt* s) {
    if (!visit_stmt(0,s,FS,NULL,NULL)) {
        return 0;
    }
    return 1;
}
