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
            case STMT_POOL:
                cur = env->stmt->Pool.id.val.s;
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
            return (s->sub == STMT_VAR) ? &s->Var.type : ((s->sub == STMT_POOL) ? &s->Pool.type : &s->Func.type);
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
            if (e->Cons.subtype.enu == TX_NIL) {
                Stmt* user = env_find_decl(e->env, e->Cons.subtype.val.s, NULL);
                assert(user != NULL);
                tk = user->User.id;
            } else {
                Stmt* user = env_find_super(e->env, e->Cons.subtype.val.s);
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
            Stmt* s = env_expr_type_find_user(e->Disc.val); // type Bool { ... }
            assert(s != NULL);
            for (int i=0; i<s->User.size; i++) {
                if (!strcmp(s->User.vec[i].id.val.s, e->Disc.subtype.val.s)) {
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

//  type Bool { ... }
//  var x: Bool
//  ... x ...           -- give "x" -> "var x" -> type Bool
Stmt* env_expr_type_find_user (Expr* e) {
    Type* tp = env_expr_type(e);
    assert(tp != NULL);
    if (tp->sub == TYPE_USER) {
        Stmt* s = env_find_decl(e->env, tp->tk.val.s, NULL);
        assert(s != NULL);
        return s;
    } else {
        return NULL;
    }
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
            .Var={ {TX_VAR,{.s="output"},0,0},
                   {TYPE_FUNC,NULL,.Func={&unit,&unit}},{EXPR_UNIT} }
        };
        env_ = (Env) { &s_out, NULL };
    }

    Env* env = &env_;

    auto int ft (Type* tp);
    auto int fe (Expr* e);
    auto int fs (Stmt* t);
    visit_stmt(S, fs, fe, ft);

    int ft (Type* tp) {
        tp->env = env;
        return 1;
    }

    int fe (Expr* e) {
        e->env = env;
        return 1;
    }

    int fs (Stmt* s) {
        s->env = env;
        switch (s->sub) {
            case STMT_RETURN:
            case STMT_CALL:
            case STMT_SEQ:
                break;

            case STMT_VAR: {
                visit_type(&s->Var.type, ft);
                visit_expr(&s->Var.init, fe); // visit expr before stmt below
                Env* new = malloc(sizeof(Env));         // visit stmt after expr above
                *new = (Env) { s, env };
                env = new;
                return 0;                               // do not visit expr again
            }

            case STMT_POOL: {
                visit_type(&s->Pool.type, ft);
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, env };
                env = new;
            }

            case STMT_USER: {
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, env };
                env = new;
                break;
            }

            case STMT_IF: {
                Env* save = env;
                visit_expr(&s->If.cond, fe);  // visit expr before stmts below
                visit_stmt(s->If.true,  fs, fe, ft);
                env = save;
                visit_stmt(s->If.false, fs, fe, ft);
                env = save;
                return 0;                               // do not visit children, I just did that
            }

            case STMT_FUNC: {
                visit_type(&s->Func.type, ft);

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
                        .Var={ {TX_VAR,{.s="arg"},0,0},*s->Func.type.Func.out,{EXPR_UNIT} }
                    };
                    Env* new = malloc(sizeof(Env));
                    *new = (Env) { arg, env };
                    env = new;
                }

                visit_stmt(s->Func.body, fs, fe, ft);

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
                } else {
                    // check "in pool"
                    if (decl->sub==STMT_VAR && decl->Var.in.enu==TX_VAR) {
                        Stmt* pool = env_find_decl(decl->env, decl->Var.in.val.s, NULL);
                        if (pool==NULL || pool->sub!=STMT_POOL) {
                            char err[512];
                            sprintf(err, "undeclared pool \"%s\"", decl->Var.in.val.s);
                            OK = err_message(e->tk, err);
                        }
                    }
                }
                break;
            }
            case EXPR_DISC:
            case EXPR_PRED:
            case EXPR_CONS: {
                Tk* sub = (e->sub==EXPR_DISC ? &e->Disc.subtype : (e->sub==EXPR_PRED ? &e->Pred.subtype : &e->Cons.subtype));
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

// Check if EXPR_CONS or EXPR_CALL of recursive yields have an enclosing pool.
//      val x[]: Nat = Succ(...)   -- ok
//      val x:   Nat = Succ(...)   -- no: x is not a pool
//      val x:   Nat = f_nat(...)  -- no: x is not a pool
//      return Succ(...)           -- ok

int check_conss_pool (Stmt* S) {
    int OK = 1;

    auto int fs (Stmt* t);
    visit_stmt(S, fs, NULL, NULL);

    int fe (Expr* e) {
        if (e->sub == EXPR_CONS) {
            if (e->Cons.subtype.enu != TX_NIL) {
                Stmt* user = env_find_super(e->env, e->Cons.subtype.val.s);
                assert(user != NULL);
                if (user->User.isrec) {
                    OK = err_message(e->Cons.subtype, "missing pool for constructor");
                }
            }
        } else if (e->sub == EXPR_CALL) {
            Stmt* s = env_expr_type_find_user(e);
            if (s!=NULL && s->User.isrec) {
                OK = err_message(e->Call.func->tk, "missing pool for call");
            }
        }
        return 1;
    }

    int fs (Stmt* s) {
        switch (s->sub) {
            case STMT_CALL:
                visit_expr(s->call.Call.arg, fe);   // must not have any cons
                break;
            case STMT_IF:
                visit_expr(&s->If.cond, fe);        // must not have any cons
                break;
            case STMT_VAR: {
                if (s->Var.type.sub == TYPE_USER) {
                    Stmt* user = env_find_decl(s->env, s->Var.type.tk.val.s, NULL);
                    assert(user != NULL);
                    if (user->sub==STMT_USER && user->User.isrec && s->Var.in.enu==TK_ERR) {
                        visit_expr(&s->Var.init, fe);   // must not have any cons
                    }
                }
                break;
            }
            case STMT_RETURN:
                // TODO: check if return type is rec
                break;
            default:
                break;
        }
        return 1;
    }

    return OK;
}

///////////////////////////////////////////////////////////////////////////////

// Check if interacting EXPR_VAR all share the same pool.
//      val x[]: Nat = ...  -- x is a pool
//      val y[]: Nat = x    -- no: x vs y
//      return x            -- no: x vs outer
//      call output(y)      -- ok
// TODO: require `copy`?

int check_same_pools (Stmt* S) {
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int env (Stmt* s) {
    set_envs(s);
    if (
        ! check_undeclareds(s) ||
        ! check_conss_pool(s)  ||
        ! check_same_pools(s)
    ) {
        return 0;
    }
    return 1;
}
