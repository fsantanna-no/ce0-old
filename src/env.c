#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

int err_message (Tk tk, const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): %s", tk.lin, tk.col, v);
    return 0;
}

Stmt* env_find_decl (Env* env, const char* id, int* scope) {    // scope=# of times crossed "arg"
    if (scope != NULL) {
        *scope = 0;
    }
    while (env != NULL) {
        const char* cur = NULL;
        switch (env->stmt->sub) {
            case STMT_USER:
                cur = env->stmt->User.id.val.s;
                break;
            case STMT_VAR:
                cur = env->stmt->Var.id.val.s;
                break;
            case STMT_FUNC:
                cur = env->stmt->Func.id.val.s;
                break;
            default:
                assert(0 && "bug found");
        }
        if (scope!=NULL && !strcmp("arg",cur)) {
            *scope = *scope + 1;
        }
        if (cur!=NULL && !strcmp(id,cur)) {
            return env->stmt;
        }
        env = env->prev;
    }
    return NULL;
}

Stmt* env_find_super (Env* env, const char* sub) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            for (int i=0; i<env->stmt->User.size; i++) {
                if (!strcmp(sub, env->stmt->User.vec[i].id.val.s)) {
                    return env->stmt;
                }
            }
        }
        env = env->prev;
    }
    return NULL;
}

Type* env_expr_type (Expr* e) { // static returns use env=NULL b/c no ids underneath
    switch (e->sub) {
        case EXPR_UNIT: {
            static Type tp = { TYPE_UNIT, NULL };
            return &tp;
        }

        case EXPR_ARG: {
            Stmt* s = env_find_decl(e->env, "arg", NULL);
            assert(s->sub == STMT_VAR);
            return &s->Var.type;
        }

        case EXPR_NATIVE: {
            static Type tp = { TYPE_NATIVE, NULL };
            return &tp;
        }

        case EXPR_VAR: {
            Stmt* s = env_find_decl(e->env, e->tk.val.s, NULL);
            assert(s != NULL);
            return (s->sub == STMT_VAR) ? &s->Var.type : &s->Func.type;
        }

        case EXPR_CALL: {
            Type* tp = NULL;
            if (e->Call.func->sub == EXPR_VAR) {
                tp = env_expr_type(e->Call.func);
            } else {
                static Type nat = { TYPE_NATIVE, NULL };
                static Type tp_ = { TYPE_FUNC, NULL, .Func={&nat,&nat} };
                tp = &tp_;
            }
            assert(tp->sub == TYPE_FUNC);
            return tp->Func.out;
        }

        case EXPR_CONS: {
            Tk tk;
            if (e->Cons.sub.enu == TX_NIL) {
                Stmt* user = env_find_decl(e->env, e->Cons.sub.val.s, NULL);
                assert(user != NULL);
                tk = user->User.id;
            } else {
                Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
                assert(user != NULL);
                tk = user->User.id;
            }
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type){ TYPE_USER, e->env, {.tk=tk} };
            return tp;
        }

        case EXPR_TUPLE: {
            Type* vec = malloc(e->Tuple.size*sizeof(Type));
            assert(vec != NULL);
            for (int i=0; i<e->Tuple.size; i++) {
                vec[i] = *env_expr_type(&e->Tuple.vec[i]);
            }
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type) { TYPE_TUPLE, e->env, {.Tuple={e->Tuple.size,vec}} };
            return tp;
        }

        case EXPR_INDEX:    // x.1
            return &env_expr_type(e->Index.tuple)->Tuple.vec[e->Index.index];

        case EXPR_DISC: {   // x.True
            Type* tp = env_expr_type(e->Disc.val);         // Bool
            Stmt* s  = env_find_decl(e->env, tp->tk.val.s, NULL); // type Bool { ... }
            assert(s != NULL);
            for (int i=0; i<s->User.size; i++) {
                if (!strcmp(s->User.vec[i].id.val.s, e->Disc.sub.val.s)) {
                    return &s->User.vec[i].type;
                }
            }
            assert(0 && "bug found");
        }

        case EXPR_PRED: {
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type) { TYPE_USER, e->env, {} };
            strcpy(tp->tk.val.s, "Bool");
            return tp;
        }
    }
    assert(0 && "bug found");
}

void env_dump (Env* env) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            puts(env->stmt->User.id.val.s);
        } else {
            assert(env->stmt->sub == STMT_VAR);
            puts(env->stmt->Var.id.val.s);
        }
        env = env->prev;
    }
}

///////////////////////////////////////////////////////////////////////////////

void set_envs (Stmt* S) {
    // TODO: _N_=0
    // predeclare function `output`
    static Env env_;
    {
        static Type unit = {TYPE_UNIT};
        static Stmt s_out = {
            0, STMT_VAR, NULL,
            .Var={ {TX_VAR,{.s="output"},0,0},0,
                   {TYPE_FUNC,NULL,.Func={&unit,&unit}},{EXPR_UNIT} }
        };
        env_ = (Env) { &s_out, NULL };
    }

    Env* env = &env_;

    auto int set_env_type (Type* tp);
    auto int set_env_expr (Expr* e);
    auto int set_env_stmt (Stmt* t);
    visit_stmt(S, set_env_stmt, set_env_expr, set_env_type);

    int set_env_type (Type* tp) {
        tp->env = env;
        return 1;
    }

    int set_env_expr (Expr* e) {
        e->env = env;
        return 1;
    }

    int set_env_stmt (Stmt* s) {
        s->env = env;
        switch (s->sub) {
            case STMT_RETURN:
            case STMT_CALL:
            case STMT_SEQ:
                break;

            case STMT_VAR: {
                visit_type(&s->Var.type, set_env_type);
                visit_expr(&s->Var.init, set_env_expr); // visit expr before stmt below
                Env* new = malloc(sizeof(Env));         // visit stmt after expr above
                *new = (Env) { s, env };
                env = new;
                return 0;                               // do not visit expr again
            }

            case STMT_USER: {
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, env };
                env = new;
                break;
            }

            case STMT_IF: {
                Env* save = env;
                visit_expr(&s->If.cond, set_env_expr);  // visit expr before stmts below
                visit_stmt(s->If.true,  set_env_stmt, set_env_expr, set_env_type);
                env = save;
                visit_stmt(s->If.false, set_env_stmt, set_env_expr, set_env_type);
                env = save;
                return 0;                               // do not visit children, I just did that
            }

            case STMT_FUNC: {
                visit_type(&s->Func.type, set_env_type);

                // body of recursive function depends on new env
                {
                    Env* new = malloc(sizeof(Env));
                    *new = (Env) { s, env };
                    env = new;
                }

                Env* save = env;

                // body depends on arg
                {
                    Stmt* arg = malloc(sizeof(Stmt));
                    *arg = (Stmt) {
                        0, STMT_VAR, NULL,
                        .Var={ {TX_VAR,{.s="arg"},0,0},0,*s->Func.type.Func.out,{EXPR_UNIT} }
                    };
                    Env* new = malloc(sizeof(Env));
                    *new = (Env) { arg, env };
                    env = new;
                }

                visit_stmt(s->Func.body, set_env_stmt, set_env_expr, set_env_type);

                env = save;

                return 0;       // do not visit children, I just did that
            }
        }
        return 1;
    }
}

///////////////////////////////////////////////////////////////////////////////

int check_undeclareds (Stmt* s) {
    int OK = 1;

    auto int ft (Type* tp);
    auto int fe (Expr* e);
    visit_stmt(s, NULL, fe, ft);

    int ft (Type* tp) {
        if (tp->sub == TYPE_USER) {
            if (env_find_decl(tp->env, tp->tk.val.s, NULL) == NULL) {
                char err[512];
                sprintf(err, "undeclared type \"%s\"", tp->tk.val.s);
                OK = err_message(tp->tk, err);
            }
        }
        return 1;
    }

    int fe (Expr* e) {
        switch (e->sub) {
            case EXPR_VAR: {
                Stmt* decl = env_find_decl(e->env, e->tk.val.s, NULL);
                if (decl == NULL) {
                    char err[512];
                    sprintf(err, "undeclared variable \"%s\"", e->tk.val.s);
                    OK = err_message(e->tk, err);
                }
                break;
            }
            case EXPR_DISC:
            case EXPR_PRED:
            case EXPR_CONS: {
                Tk* sub = (e->sub==EXPR_DISC ? &e->Disc.sub : (e->sub==EXPR_PRED ? &e->Pred.sub : &e->Cons.sub));
                if (sub->enu == TX_NIL) {
                    Stmt* decl = env_find_decl(e->env, sub->val.s, NULL);
                    if (decl == NULL) {
                        char err[512];
                        sprintf(err, "undeclared type \"%s\"", sub->val.s);
                        OK = err_message(e->tk, err);
                    }
                } else {
                    Stmt* user = env_find_super(e->env, sub->val.s);
                    if (user == NULL) {
                        char err[512];
                        sprintf(err, "undeclared subtype \"%s\"", sub->val.s);
                        OK = err_message(e->tk, err);
                    }
                }
                break;
            }
            default:
                break;
        }
        return 1;
    }

    return OK;
}

///////////////////////////////////////////////////////////////////////////////

// Identify all STMT_CALL with EXPR_CALLs with recursive returns.
// Error because they have no explicit pool to hold the returns:
//      func f: () -> Nat {}    -- "f" returns Nat
//      call f()                -- ERR: missing pool for return of "f"

int check_calls_without_return_pool (Stmt* S) {
    int OK = 1;

    auto int fs (Stmt* s);
    visit_stmt(S, fs, NULL, NULL);

    // find all STMT_CALL

    int fs (Stmt* s) {
        if (s->sub != STMT_CALL) {
            return 1;
        }

        auto int fe (Expr* e);
        visit_expr(&s->call, fe);

        // find all EXPR_CALL inside STMT_CALL w/ output type TYPE_USER.isrec
        int fe (Expr* e) {
            Type* tp = env_expr_type(e);
            assert(tp != NULL);
            if (e->sub!=EXPR_CALL || (tp->sub!=TYPE_USER)) {
                return 1;
            }
            Stmt* decl = env_find_decl(e->env, tp->tk.val.s, NULL);
            assert(decl != NULL);
            if (decl->User.isrec) {    // recursive type created inside function
                char err[512];
                assert(e->Call.func->sub == EXPR_VAR);
                sprintf(err, "missing pool for return of \"%s\"", e->Call.func->tk.val.s);
                OK = err_message(e->Call.func->tk, err);
            }
            return 1;
        }

        return 1;
    }
    return OK;
}

///////////////////////////////////////////////////////////////////////////////

// Check if initialization of STMT_VAR with pool really allocates memory:
//      x[] = Succ(...)     -- ok: EXPR_CONS
//      x[] = f_nat()       -- ok: EXPR_CALL (only if return isrec)
//      x[] = ...           -- no: does not allocate anything

int check_pools_init (Stmt* S) {
    int OK = 1;

    auto int fs (Stmt* S);
    visit_stmt(S, fs, NULL, NULL);

    // find all STMT_VAR with pool

    int fs (Stmt* s) {
        if (s->sub!=STMT_VAR || !s->Var.pool) {
                return 1;
        }

        // check if it's an EXPR_CONS or EXPR_CALL to isrec

        if (s->Var.init.sub == EXPR_CONS) {
            // ok: x[] = Succ(...)
        } else if (s->Var.init.sub == EXPR_CALL) {
            // maybe
            Type* tp = env_expr_type(s->Var.init.Call.func);
            if (tp->sub == TYPE_USER) {
                Stmt* decl = env_find_decl(s->Var.init.env, tp->tk.val.s, NULL);
                assert(decl != NULL);
                if (decl->User.isrec) {
                    // ok: x[] = f_nat()
                } else {
                    // no: x[] = f_bool()
                    OK = 0;
                }
            }
        } else {
            // no: x[] = ...
            OK = 0;
        }
        if (!OK) {
            err_message(s->Var.id, "invalid pool : no data allocation");
        }
        return 1;
    }

    return OK;
}

///////////////////////////////////////////////////////////////////////////////

// Check if accesses of EXPR_VAR with pool hits another pool or returns.
//      val x[]: Nat = ...  -- x is a pool
//      val y[]: Nat = x    -- no: hits pool
//      return x            -- no: returns
//      call output(y)      -- ok

int check_pools_escapes (Stmt* S) {
    int OK = 1;

    auto int fs (Stmt* S);
    visit_stmt(S, fs, NULL, NULL);

    // find all STMT_VAR and STMT_RETURN

    int fs (Stmt* s) {
        if (s->sub!=STMT_VAR && s->sub!=STMT_RETURN) {
            return 1;
        }

        // check if pool initialization or return expression uses other pools
        //  - needs to recurse on every other EXPR_VAR found

        Expr* e = (s->sub == STMT_VAR ? &s->Var.init : &s->ret);

        auto int fe (Expr* e);
        visit_expr(e, fe);

        // find all EXPR_VAR uses

        int fe (Expr* e) {
            if (e->sub != EXPR_VAR) {
                return 1;
            }

            // check if EXPR_VAR is a declared pool

            Stmt* decl = env_find_decl(e->env, e->tk.val.s, NULL);
            assert(decl != NULL);
            if (decl->sub == STMT_VAR) {
                if (decl->Var.pool) {
                    char err[512];
                    sprintf(err, "invalid access to \"%s\" : pool escapes", e->tk.val.s);
                    OK = err_message(e->tk, err);
                }

                // recursively check all EXPR_VAR subexpressions found on STMT_VAR init
                visit_expr(&decl->Var.init, fe);
            } else {
                assert(decl->sub == STMT_FUNC);
            }

            return 1;
        }

        return 1;
    }

    return OK;
}

///////////////////////////////////////////////////////////////////////////////

// Mark all EXPR_CONS that need to be allocated in the context pool.
//      val x[]: Nat = Succ(...)    -- _pool_x
//      val y: Nat = Succ(...)      -- static
//      return Succ(...)            -- _pool_outer
//      return x where x=Succ(...)  -- _pool_outer

int set_conss_dynamic (Stmt* S) {
    int OK = 1;

    auto int fs (Stmt* s);
    visit_stmt(S, fs, NULL, NULL);

    // find all STMT_VAR and STMT_RETURN

    int fs (Stmt* s) {
        if (s->sub!=STMT_VAR && s->sub!=STMT_RETURN) {
            return 1;
        }
        if (s->sub==STMT_VAR && !s->Var.pool) {
            return 1;   // skip non-pool declarations
        }

        auto int fe (Expr* e);
        visit_expr(&s->ret, fe);

        // find all EXPR_CONS and EXPR_VAR (to recurse for other EXPR_CONS inside respective STMT_VAR)

        int fe (Expr* e) {
            // check EXPR_CONS
            if (e->sub == EXPR_CONS) {
                if (e->Cons.sub.enu == TX_NIL) {
                    // no allocation
                } else {
                    // set EXPR_CONS to "ispool"
                    Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
                    assert(user != NULL);
                    if (user->User.isrec) {
                        // TODO: check if type matches that of STMT_RETURN or STMT_VAR
                        e->Cons.ispool = 1;
                    }
                }

            // recurse into EXPR_VAR
            } else if (e->sub == EXPR_VAR) {
                // find respective STMT_VAR
                int scope;
                Stmt* decl = env_find_decl(e->env, e->tk.val.s, &scope);
                assert(decl != NULL);
                if (decl->sub == STMT_VAR) {
                    if (scope == 0) {
                        visit_expr(&decl->Var.init, fe);    // do not visit STMT_VAR in outer scopes
                    }
                } else {
                    assert(decl->sub == STMT_FUNC);
                    // do nothing: funcs are always static/global
                }
            }
            return 1;
        }

        return 1;
    }
    return OK;
}

///////////////////////////////////////////////////////////////////////////////

int env (Stmt* s) {
    set_envs(s);
    if (
        ! check_undeclareds(s)                  ||
        ! check_calls_without_return_pool(s)    ||
        ! check_pools_init(s)                   ||
        ! check_pools_escapes(s)
    ) {
        return 0;
    }
    set_conss_dynamic(s);
    return 1;
}
