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

// Identify all STMT_CALL with EXPR_CALLs for with recursive returns.
// These are problematic because they have no explicit pool to hold the returns.

int check_calls_with_call_rec (Stmt* s) {
    int OK = 1;

    // find all STMT_CALL
    int f_calls (Stmt* x) {
        if (x->sub != STMT_CALL) {
            return 1;
        }

        // find all EXPR_CALL inside STMT_CALL w/ output type TYPE_USER.isrec
        int fe (Expr* e) {
            Type* tp = env_expr_type(e);
            assert(tp != NULL);
            if (e->sub!=EXPR_CALL || (tp->sub!=TYPE_USER)) {
                return 1;
            }
            Stmt* s = env_find_decl(e->env, tp->tk.val.s, NULL);
            assert(s != NULL);
            if (s->User.isrec) {    // recursive type created inside function
                char err[512];
                assert(e->Call.func->sub == EXPR_VAR);
                sprintf(err, "missing pool for return of \"%s\"", e->Call.func->tk.val.s);
                OK = err_message(e->Call.func->tk, err);
            }
            return 1;
        }

        visit_expr(&x->call, fe);
        return 1;
    }
    visit_stmt(s, f_calls, NULL, NULL);
    return OK;
}

///////////////////////////////////////////////////////////////////////////////

// Mark all EXPR_CONS that need to be allocated in the surrounding pool.
//  - return Succ(...)              -- allocate Succ on received pool
//  - return x where x=Succ(...)    -- allocate Succ on received pool
// Also check if none of these allocations have a unnecessary local pool.
//  - return x where x[] = Succ(...) -- x is not pool b/c Succ will be allocd on recvd pool
// Also mark all REC_CONS references, which are those with EXPR_CONS.
//  - REC_CONS  escape and are root of some constructor
//  - REC_ALIAS escape but are not root of some constructor, instead they point to REC_CONS
//
// - identify all STMT_RETURN
//      - mark all EXPR_CONS in it
//      - recurse into EXPR_VAR in it
//          - check if EXPR_VAR is *not* a pool b/c allocation must be outside

int mark_cons__check_decl__mark_rec_cons (Stmt* s) {
    int OK = 1;

    // find all STMT_RETURN
    int f_rets (Stmt* x) {
        if (x->sub != STMT_RETURN) {
            return 1;
        }
        int fe (Expr* e) {
            // find all EXPR_CONS inside STMT_RETURN.expr/STMT_VAR.init
            if (e->sub==EXPR_CONS && e->Cons.sub.enu!=TX_NIL) {
                // set EXPR_CONS to "ispool"
                Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
                assert(user != NULL);
                e->Cons.ispool = user->User.isrec;  // ispool only if is also rec

            // find all EXPR_VAR inside STMT_RETURN.expr/STMT_VAR.init
            } else if (e->sub == EXPR_VAR) {
                // find respective STMT_VAR
                int scope;
                Stmt* y = env_find_decl(e->env, e->tk.val.s, &scope);
                assert(y != NULL);
                if (y->sub == STMT_VAR) {
                    if (y->Var.ref.sub == REC_POOL) {
                        OK = err_message(y->Var.id, "invalid pool : data returns");
                    } else {
                        y->Var.ref.sub = REC_CONS; // var declaration of recursive type w/ constructor that appears on return
//aqui realmente tem construtor?
//arg tb é REC_CONS se for retornado
//strong
                    }

                    // find all EXPR_CONS/EXPR_VAR inside STMT_VAR init
                    if (scope == 0) {
                        visit_expr(&y->Var.init, fe);
                    } else {
                        // ignore "return CONST/GLOBAL/SURVIVOR"
                    }
                } else {
                    assert(y->sub == STMT_FUNC);
                    // do nothing: funcs are always static/global
                }
            }
            return 1;
        }

        // find all EXPR_CONS/EXPR_VAR inside STMT_RETURN expr
        visit_expr(&x->ret, fe);
        return 1;
    }
    visit_stmt(s, f_rets, NULL, NULL);
    return OK;
}

///////////////////////////////////////////////////////////////////////////////

int check_undeclared (Stmt* s) {
    int OK = 1;

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

    visit_stmt(s, NULL, fe, ft);
    return OK;
}

///////////////////////////////////////////////////////////////////////////////

void set_ref (Stmt* s) {
    int fe (Expr* e) {
        if (e->sub == EXPR_VAR) {
            Stmt* decl = env_find_decl(e->env, e->tk.val.s, NULL);
            assert(decl != NULL);
            if (decl->sub==STMT_USER && s->User.isrec) {
                decl->Var.ref.sub = REC_ALIAS;
            }
        }
        return 1;
    }
    visit_stmt(s, NULL, fe, NULL);
}

///////////////////////////////////////////////////////////////////////////////

void set_env (Stmt* s) {
    // TODO: _N_=0
    // predeclare function `output`
    static Env env_;
    {
        static Type unit = {TYPE_UNIT};
        static Stmt s = {
            0, STMT_VAR, NULL,
            .Var={ {TX_VAR,{.s="output"},0,0},{REC_NONE},
                   {TYPE_FUNC,NULL,.Func={&unit,&unit}},{EXPR_UNIT} }
        };
        env_ = (Env) { &s, NULL };
    }

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
                        .Var={ {TX_VAR,{.s="arg"},0,0},{REC_NONE},*s->Func.type.Func.out,{EXPR_UNIT} }
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
    visit_stmt(s, set_env_stmt, set_env_expr, set_env_type);
}

///////////////////////////////////////////////////////////////////////////////

int env (Stmt* s) {
    set_env(s);
    if (!check_undeclared(s)) {
        return 0;
    }
    set_ref(s);
    if (!check_calls_with_call_rec(s)) {
        return 0;
    }
    if (!mark_cons__check_decl__mark_rec_cons(s)) {
        return 0;
    }
    return 1;
}
