#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

static int MOVES[1024];
static int MOVES_N = 0;

///////////////////////////////////////////////////////////////////////////////

typedef void (*F_txed_vars) (Expr* e_, Expr* e);

void env_txed_vars (Env* env, Expr* e, F_txed_vars f) {
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
                env_txed_vars(env, e->Tuple.vec[i], f);
            }
            break;

        case EXPR_CALL: // tx for args is checked elsewhere (here, only if return type is also ishasrec)
            // func may transfer its argument back
            // set x.2 = f(x) where f: T1->T2 { return arg.2 }
            env_txed_vars(env, e->Call.arg, f);
            break;

        case EXPR_CONS: {
            env_txed_vars(env, e->Cons, f);
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
int check_txs_exprs (Env* env, Expr* E, int iscycle) {

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
        env_txed_vars(env, E, f);
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

int check_exec_vars (Stmt* S) {

    // visit all var declarations

    if (S->sub != STMT_VAR) {
        return 1;               // not var declaration
    }

    if (!env_type_ishasrec(S->env, S->Var.type)) {
        return 1;               // not recursive type
    }

    // STMT_VAR (S.x): recursive user type

    int txed = 0;              // Tracks if S.x has been transferred:
    Tk* txed_tk = NULL;

    Stmt* bws[256] = { S };     // Tracks borrows of S.x (who I am borrowed to)
    int bws_n = 1;              //  - if S.x has active borrows (bws_n > 1), then it cannot be transferred
    int bws_has (Stmt* s) {     //  - clean bws on block exit
        for (int i=0; i<bws_n; i++) {
            if (bws[i] == s) {  //      var y: \Nat = \x
                return 1;       //      var z: (Int,\Nat) = (1,\y)  -- x is borrowed by both y and z
            }
        }
        return 0;
    }

    typedef struct { int depth_stop; int bws_n; } Stack;
    Stack stack[256];
    int stack_n = 0;

    void pre (void) {
        txed = 0;
        txed_tk = NULL;
        bws_n = 1;
        bws[0] = S;
        stack_n = 0;
    }

    auto int fs (Stmt* s);
    return exec(S->seq, pre, fs, NULL); // start from S.x and exec until out of scope

    ///////////////////////////////////////////////////////////////////////////

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
            stack[stack_n].bws_n = bws_n;
            assert(stack_n < 256);
            stack_n++;
        } else {
            while (stack_n>0 && s->env->depth<=stack[stack_n-1].depth_stop) {   // leave block: pop state
                stack_n--;
                bws_n = stack[stack_n].bws_n;
            }
        }

        auto void add_bws (Expr* e);
        auto void set_txed (Expr* E);
        auto int fe (Env* env, Expr* e);

        int accs (Stmt* s) {
            int ret = visit_stmt(0, s, NULL, fe, NULL);
            if (ret != EXEC_CONTINUE) {
                return ret;
            }
            return EXEC_CONTINUE;
        }

        int fe (Env* env, Expr* e) {
            switch (e->sub) {
                case EXPR_CALL:
                    //assert(e->Call.func->sub == EXPR_VAR);
                    if (!strcmp(e->Call.func->tk.val.s,"move")) break;
                    set_txed(e->Call.arg);
                    break;
                case EXPR_CONS:
                    set_txed(e->Cons);
                    break;
                case EXPR_VAR:
                    if (strcmp(S->Var.tk.val.s,e->tk.val.s)) {
                        break;
                    }
                    Stmt* decl = env_id_to_stmt(env, e->tk.val.s);
                    assert(decl!=NULL && decl==S);

                    // access to S.x

                    if (txed == 0) {
                        txed_tk = &e->tk;
                        break;
                    }
                    if (txed == 2) { // Rule 4 (growable)
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

                    } else {
                        assert(txed == 1);
                        txed = 2;
                        if (bws_n >= 2) {       // Rule 6
                            int lin = (txed_tk == NULL) ? e->tk.lin : txed_tk->lin;
                            char err[TK_BUF+256];
                            sprintf(err, "invalid transfer of \"%s\" : active pointer in scope (ln %d)",
                                    e->tk.val.s, lin);
// ignorar os bws em si mesmo no modo growable
                            err_message(&e->tk, err);
                            return VISIT_ERROR;
                        }
                        txed_tk = &e->tk;
                    }
                default:
                    break;
            }
            return VISIT_CONTINUE;
        }

        // FS

        switch (s->sub) {
            case STMT_VAR:
                add_bws(s->Var.init);
                set_txed(s->Var.init);
                return accs(s);

            case STMT_SET: {
#if 0
                int iscycle = 0;
                if (s->Set.dst->sub != EXPR_VAR) { // not cycle if root transfer (x = x.Item!)
                    // Rule 5: cycles can only occur in STMT_SET
                    Expr* dst = expr_leftmost(s->Set.dst);
                    assert(dst->sub == EXPR_VAR);
                    for (int i=0; i<bws_n; i++) {
                        if (!strcmp(dst->tk.val.s,bws[i]->Var.tk.val.s)) {
                            iscycle = 1;
                            break;
                        }
                    }
                }
                if (!iscycle)
#endif
                {     // TODO: only in `growable´ mode
                    add_bws(s->Set.src);
                }
                set_txed(s->Set.src);
                return accs(s);
            }

            case STMT_IF: {     // body is handle in next step of "exec"
                int ret = visit_expr(0, s->env, s->If.tst, fe);
                if (ret != EXEC_CONTINUE) {
                    return ret;
                }
                return EXEC_CONTINUE;
            }

            case STMT_CALL:
                return accs(s);

            default:
                return EXEC_CONTINUE;
        }
        assert(0);

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
        //  var y = S.x
        void set_txed (Expr* E) {
            int vars_n=0; Expr* vars[256];
            {
                void f_get_txs (Expr* e_, Expr* e) {
                    assert(vars_n < 255);
                    if (e->sub==EXPR_VAR || e->sub==EXPR_DNREF || e->sub==EXPR_DISC || e->sub==EXPR_INDEX) {
                        Expr* var = expr_leftmost(e);
                        assert(var != NULL);
                        assert(var->sub == EXPR_VAR);
                        vars[vars_n++] = e_;
                    }
                }
                env_txed_vars(s->env, E, f_get_txs);
            }
            for (int i=0; i<vars_n; i++) {
                Expr* e = vars[i];
                if (e->sub==EXPR_CALL && e->Call.func->sub==EXPR_VAR && !strcmp(e->Call.func->tk.val.s,"move")) {
                    e = e->Call.arg;
                }
                Expr* e_ = expr_leftmost(e);
                assert(e_->sub == EXPR_VAR);
                if (!strcmp(e_->tk.val.s,S->Var.tk.val.s)) {
                    txed = 1;
#if 0
                    if (iscycle) {
                        char err[1024];
                        sprintf(err, "invalid assignment : cannot transfer ownsership to itself");
                        return err_message(&e_->tk, err);
                    }
#endif
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

int owner (Stmt* s) {
    MOVES_N = 0;

// TODO
int iscycle = 0;

    // SET_TXS_ALL
    {
        int fs (Stmt* s) {
            switch (s->sub) {
                case STMT_VAR:
                    if (!check_txs_exprs(s->env, s->Var.init,0)) {
                        return VISIT_ERROR;
                    }
                    break;
                case STMT_SET:
                    if (!check_txs_exprs(s->env, s->Set.src,iscycle)) {
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
                        if (!check_txs_exprs(env, e->Call.arg,0)) {
                            return VISIT_ERROR;
                        }
                    }
                    break;
                case EXPR_CONS:
                    if (!check_txs_exprs(env, e->Cons,0)) {
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
    if (!visit_stmt(0,s,check_exec_vars,NULL,NULL)) {
        return 0;
    }
    return 1;
}
