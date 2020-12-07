#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

int err_message (Tk tk, const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): %s", tk.lin, tk.col, v);
    return 0;
}

Stmt* env_find_decl (Env* env, const char* id) {
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

Type* env_expr_type (Expr* e) {
    static Type ret;
    switch (e->sub) {
        case EXPR_UNIT:
            ret = (Type) { TYPE_UNIT, e->env };
            return &ret;

        case EXPR_NIL:
            assert(0 && "TODO");

        case EXPR_ARG: {
            Stmt* s = env_find_decl(e->env, "arg");
            assert(s->sub == STMT_VAR);
            return &s->Var.type;
        }

        case EXPR_NATIVE:
            ret = (Type) { TYPE_NATIVE, e->env };
            return &ret;

        case EXPR_VAR: {
            Stmt* s = env_find_decl(e->env, e->tk.val.s);
            assert(s != NULL);
            return (s->sub == STMT_VAR) ? &s->Var.type : &s->Func.type;
        }
        case EXPR_CALL: {   // f()
            assert(e->Call.func->sub == EXPR_VAR);
            Type* tp = env_expr_type(e->Call.func);
            assert(tp->sub == TYPE_FUNC);
            return tp->Func.out;
        }

        case EXPR_CONS: {   // Bool.True()
            Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
            assert(user != NULL);
            ret = (Type) { TYPE_USER, e->env, {.tk=user->User.id} };
            return &ret;
        }

        case EXPR_TUPLE: {
            Type* vec = malloc(e->Tuple.size*sizeof(Type));
            assert(vec != NULL);
            for (int i=0; i<e->Tuple.size; i++) {
                vec[i] = *env_expr_type(&e->Tuple.vec[i]);
            }
            ret = (Type) { TYPE_TUPLE, e->env, {.Tuple={e->Tuple.size,vec}} };
            return &ret;
        }

        case EXPR_INDEX:    // x.1
            return &env_expr_type(e->Index.tuple)->Tuple.vec[e->Index.index];

        case EXPR_DISC: {   // x.True
            Type* tp = env_expr_type(e->Disc.val);         // Bool
            Stmt* s  = env_find_decl(e->env, tp->tk.val.s); // type Bool { ... }
            assert(s != NULL);
            for (int i=0; i<s->User.size; i++) {
                if (!strcmp(s->User.vec[i].id.val.s, e->Disc.sub.val.s)) {
                    return &s->User.vec[i].type;
                }
            }
            assert(0 && "bug found");
        }

        case EXPR_PRED: {
            Type* ret = malloc(sizeof(Type));
            assert(ret != NULL);
            *ret = (Type) { TYPE_USER, e->env, {} };
            strcpy(ret->tk.val.s, "Bool");
            return ret;
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

// Identify all EXPR_CONS that need to be allocated in the surrounding pool.
//  - identify all STMT_RETURN
//      - mark all EXPR_CONS in it
//      - recurse into EXPR_VAR in it

void set_pool_cons (Stmt* s) {
    // find all STMT_RETURN
    int f_rets (Stmt* x) {
        if (x->sub == STMT_RETURN) {
            int fe (Expr* e) {
                // find all EXPR_CONS inside STMT_RETURN expr/STMT_VAR init
                if (e->sub == EXPR_CONS) {
                    // set EXPR_CONS to "ispool"
                    Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
                    assert(user != NULL);
                    e->Cons.ispool = user->User.isrec;  // ispool only if is also rec

                // find all EXPR_VAR inside STMT_RETURN expr/STMT_VAR init
                } else if (e->sub == EXPR_VAR) {
                    // find respective STMT_VAR
                    Stmt* y = env_find_decl(e->env, e->tk.val.s);
                    assert(y != NULL);
                    if (y->sub == STMT_VAR) {
                        // find all EXPR_CONS/EXPR_VAR inside STMT_VAR init
                        visit_expr(&y->Var.init, fe);
                    } else {
                        assert(y->sub == STMT_FUNC);
                    }
                }
                return 1;
            }

            // find all EXPR_CONS/EXPR_VAR inside STMT_RETURN expr
            visit_expr(&x->ret, fe);
        }
        return 1;
    }
    visit_stmt(s, f_rets, NULL, NULL);
}

///////////////////////////////////////////////////////////////////////////////

int check_undeclared (Stmt* s) {
    int OK = 1;

    int check_type (Type* tp) {
        if (tp->sub == TYPE_USER) {
            if (env_find_decl(tp->env, tp->tk.val.s) == NULL) {
                char err[512];
                sprintf(err, "undeclared type \"%s\"", tp->tk.val.s);
                OK = err_message(tp->tk, err);
            }
        }
        return 1;
    }

    int check_expr (Expr* e) {
        if (e->sub == EXPR_VAR) {
            if (env_find_decl(e->env, e->tk.val.s) == NULL) {
                char err[512];
                sprintf(err, "undeclared variable \"%s\"", e->tk.val.s);
                OK = err_message(e->tk, err);
            }
        }
        return 1;
    }

    visit_stmt(s, NULL, check_expr, check_type);
    return OK;
}

///////////////////////////////////////////////////////////////////////////////

// TODO: _N_=0
// predeclare function `output`
Stmt s_ = { 0, STMT_VAR, NULL,
           .Var={ {TX_VAR,{.s="output"},0,0},0,{TYPE_FUNC},{EXPR_UNIT} } };
Env env_ = { &s_, NULL };

void set_env_stmt_expr_type (Stmt* s) {
    Env* env = &env_;

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

                // set all EXPR_CONS that should be allocated in the pool
                if (s->Func.type.Func.out->sub == TYPE_USER) {
                    Stmt* decl = env_find_decl(s->env, s->Func.type.Func.out->tk.val.s);
                    assert(decl!=NULL && decl->sub==STMT_USER);
                    if (decl->User.isrec) {
                        //assert(0 && "ok");
                        //env_pool_cons(s->Func.body);
                    }
                }

                return 0;       // do not visit children, I just did that
            }
        }
        return 1;
    }
    visit_stmt(s, set_env_stmt, set_env_expr, set_env_type);
}

///////////////////////////////////////////////////////////////////////////////

int env (Stmt* s) {
    set_env_stmt_expr_type(s);
    set_pool_cons(s);
    return check_undeclared(s);
}
