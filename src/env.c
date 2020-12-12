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
            Stmt* s = env_find_decl(e->env, e->var.val.s, NULL);
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

        case EXPR_ALIAS: {
            Type* tp = env_expr_type(e->alias);
            assert(tp->sub==TYPE_USER && !tp->User.isalias);

            Type* ret = malloc(sizeof(Type));
            assert(ret != NULL);
            *ret = *tp;
            ret->User.isalias = 1;
            return ret;
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
            *tp = (Type){ TYPE_USER, e->env, .User={tk,0} };
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
            *tp = (Type) { TYPE_USER, e->env, .User={{},0} };
            strcpy(tp->User.id.val.s, "Bool");
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
        Stmt* s = env_find_decl(e->env, tp->User.id.val.s, NULL);
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
            .Var={ {TX_LOWER,{.s="output"},0,0},
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
                        .Var={ {TX_LOWER,{.s="arg"},0,0},*s->Func.type.Func.out,{EXPR_UNIT} }
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
            if (env_find_decl(tp->env, tp->User.id.val.s, NULL) == NULL) {
                char err[512];
                sprintf(err, "undeclared type \"%s\"", tp->User.id.val.s);
                OK = err_message(tp->User.id, err);
            }
        }
        return 1;
    }

    int fe (Expr* e) {
        switch (e->sub) {
            case EXPR_VAR: {
                Stmt* decl = env_find_decl(e->env, e->var.val.s, NULL);
                if (decl == NULL) {
                    char err[512];
                    sprintf(err, "undeclared variable \"%s\"", e->var.val.s);
                    OK = err_message(e->var, err);
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
assert(0 && "TODO e->tk");
                        //OK = err_message(e->tk, err);
                    }
                } else {
                    Stmt* user = env_find_super(e->env, sub->val.s);
                    if (user == NULL) {
                        char err[512];
                        sprintf(err, "undeclared subtype \"%s\"", sub->val.s);
assert(0 && "TODO e->tk");
                        //OK = err_message(e->tk, err);
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

int env (Stmt* s) {
    set_envs(s);
    if (!check_undeclareds(s)) {
        return 0;
    }
    return 1;
}
