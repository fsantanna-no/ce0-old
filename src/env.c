#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

Type Type_Unit  = { TYPE_UNIT, NULL, 0 };

int err_message (Tk tk, const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): %s", tk.lin, tk.col, v);
    return 0;
}

Stmt* env_id_to_stmt (Env* env, const char* id) {
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


Stmt* env_sub_id_to_user_stmt (Env* env, const char* sub) {
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

Sub* env_find_sub (Env* env, const char* sub) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            for (int i=0; i<env->stmt->User.size; i++) {
                if (!strcmp(sub, env->stmt->User.vec[i].id.val.s)) {
                    return &env->stmt->User.vec[i];
                }
            }
        }
        env = env->prev;
    }
    return NULL;
}

Type* env_expr_to_type (Expr* e) { // static returns use env=NULL b/c no ids underneath
    switch (e->sub) {
        case EXPR_UNIT: {
            static Type tp = { TYPE_UNIT, NULL, 0 };
            return &tp;
        }

        case EXPR_ARG: {
            Stmt* s = env_id_to_stmt(e->env, "arg");
            assert(s->sub == STMT_VAR);
            return &s->Var.type;
        }

        case EXPR_NATIVE: {
            static Type tp = { TYPE_NATIVE, NULL, 0 };
            return &tp;
        }

        case EXPR_VAR: {
            Stmt* s = env_id_to_stmt(e->env, e->Var.id.val.s);
            assert(s != NULL);
            return (s->sub == STMT_VAR) ? &s->Var.type : &s->Func.type;
        }

        case EXPR_CALL: {
            Type* tp = NULL;
            if (e->Call.func->sub == EXPR_VAR) {
                tp = env_expr_to_type(e->Call.func);
            } else {
                static Type nat = { TYPE_NATIVE, NULL, 0 };
                static Type tp_ = { TYPE_FUNC, NULL, 0, .Func={&nat,&nat} };
                tp = &tp_;
            }
            assert(tp->sub == TYPE_FUNC);
            return tp->Func.out;
        }

        case EXPR_ALIAS: {
            Type* tp = env_expr_to_type(e->Alias);
            assert(tp->sub==TYPE_USER && !tp->isalias);

            Type* ret = malloc(sizeof(Type));
            assert(ret != NULL);
            *ret = *tp;
            ret->isalias = 1;
            return ret;
        }

        case EXPR_CONS: {
            Tk tk;
            if (e->Cons.subtype.enu == TX_NIL) {
                Stmt* user = env_id_to_stmt(e->env, e->Cons.subtype.val.s);
                assert(user != NULL);
                tk = user->User.id;
            } else {
                Stmt* user = env_sub_id_to_user_stmt(e->env, e->Cons.subtype.val.s);
                assert(user != NULL);
                tk = user->User.id;
            }
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type){ TYPE_USER, e->env, 0, .User=tk };
            return tp;
        }

        case EXPR_TUPLE: {
            Type* vec = malloc(e->Tuple.size*sizeof(Type));
            assert(vec != NULL);
            for (int i=0; i<e->Tuple.size; i++) {
                vec[i] = *env_expr_to_type(&e->Tuple.vec[i]);
            }
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type) { TYPE_TUPLE, e->env, 0, {.Tuple={e->Tuple.size,vec}} };
            return tp;
        }

        case EXPR_INDEX:    // x.1
            return &env_expr_to_type(e->Index.tuple)->Tuple.vec[e->Index.index];

        case EXPR_DISC: {   // x.True
            Type* val = env_expr_to_type(e->Disc.val);
            assert(val->sub == TYPE_USER);
            Stmt* decl = env_id_to_stmt(e->env, val->User.val.s);
            assert(decl!=NULL && decl->sub==STMT_USER);
            for (int i=0; i<decl->User.size; i++) {
                if (!strcmp(decl->User.vec[i].id.val.s, e->Disc.subtype.val.s)) {
                    Type* tp = &decl->User.vec[i].type;
                    assert(!tp->isalias && "bug found : `&´ inside subtype");
                    if (!val->isalias) {
                        return tp;
                    } else {
                        Type* ret = malloc(sizeof(Type));
                        assert(ret != NULL);
                        *ret = *tp;
                        ret->isalias = 1;
                        return ret;
                    }
                }
            }
            assert(0 && "bug found");
        }

        case EXPR_PRED: {
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type) { TYPE_USER, e->env, 0, .User={} };
            strcpy(tp->User.val.s, "Bool");
            return tp;
        }
    }
    assert(0 && "bug found");
}

//  type Bool { ... }
//  var x: Bool
//  ... x ...           -- give "x" -> "var x" -> type Bool
Stmt* env_expr_to_type_to_user_stmt (Expr* e) {
    Type* tp = env_expr_to_type(e);
    assert(tp != NULL);
    if (tp->sub == TYPE_USER) {
        Stmt* s = env_id_to_stmt(e->env, tp->User.val.s);
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
        static Type alias = { TYPE_USER, NULL, 0, .User={TK_ERR,{},0,0} };
        static Stmt s_out = {
            0, STMT_VAR, NULL,
            .Var={ {TX_LOWER,{.s="output"},0,0},
                   {TYPE_FUNC,NULL,.Func={&alias,&Type_Unit}},{EXPR_UNIT} }
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
            if (env_id_to_stmt(tp->env, tp->User.val.s) == NULL) {
                char err[512];
                sprintf(err, "undeclared type \"%s\"", tp->User.val.s);
                OK = err_message(tp->User, err);
            }
        }
        return 1;
    }

    int fe (Expr* e) {
        switch (e->sub) {
            case EXPR_VAR: {
                Stmt* decl = env_id_to_stmt(e->env, e->Var.id.val.s);
                if (decl == NULL) {
                    char err[512];
                    sprintf(err, "undeclared variable \"%s\"", e->Var.id.val.s);
                    OK = err_message(e->Var.id, err);
                }
                break;
            }
            case EXPR_DISC:
            case EXPR_PRED:
            case EXPR_CONS: {
                Tk* sub = (e->sub==EXPR_DISC ? &e->Disc.subtype : (e->sub==EXPR_PRED ? &e->Pred.subtype : &e->Cons.subtype));
                if (sub->enu == TX_NIL) {
                    Stmt* decl = env_id_to_stmt(e->env, sub->val.s);
                    if (decl == NULL) {
                        char err[512];
                        sprintf(err, "undeclared type \"%s\"", sub->val.s);
                        OK = err_message(*sub, err);
                    }
                } else {
                    Stmt* user = env_sub_id_to_user_stmt(e->env, sub->val.s);
                    if (user == NULL) {
                        char err[512];
                        sprintf(err, "undeclared subtype \"%s\"", sub->val.s);
                        OK = err_message(*sub, err);
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

int check_types (Stmt* S) {
    int OK = 1;

    auto int fs (Stmt* s);
    auto int fe (Expr* e);
    visit_stmt(S, fs, fe, NULL);

    int type_is_sup_sub (Type* sup, Type* sub) {
        if (sup->sub!=sub->sub || sup->isalias!=sub->isalias) {
            return 0;
        }
        switch (sup->sub) {
            case TYPE_USER:
                return (!strcmp(sup->User.val.s,sub->User.val.s) &&
                        sup->isalias == sub->isalias);

            case TYPE_TUPLE:
                if (sup->Tuple.size != sub->Tuple.size) {
                    return 0;
                }
                for (int i=0; i<sup->Tuple.size; i++) {
                    if (!type_is_sup_sub(&sup->Tuple.vec[i], &sub->Tuple.vec[i])) {
                        return 0;
                    }
                }
                break;

            default:
                break;
        }
        return 1;
    }

    int fe (Expr* e) {
        switch (e->sub) {
            case EXPR_INDEX:
                // TODO: check if e->tuple is really a tuple and that e->index is in range
                break;

            case EXPR_CALL: {
                Type* func = env_expr_to_type(e->Call.func);
                Type* arg  = env_expr_to_type(e->Call.arg);

                if (e->Call.func->sub == EXPR_NATIVE) {
                    // TODO
                } else if (!strcmp(e->Call.func->Var.id.val.s,"output")) {
                    // TODO
                } else if (!type_is_sup_sub(func->Func.inp, arg)) {
                    assert(e->Call.func->sub == EXPR_VAR);
                    char err[512];
                    sprintf(err, "invalid call to \"%s\" : type mismatch", e->Call.func->Var.id.val.s);
                    OK = err_message(e->Call.func->Var.id, err);
                }
                break;
            }

            case EXPR_CONS: {
                Type* subtype;
                if (e->Cons.subtype.enu == TX_NIL) {
                    subtype = &Type_Unit;
                } else {
                    Sub* sub = env_find_sub(e->env, e->Cons.subtype.val.s);
                    assert(sub != NULL);
                    subtype = &sub->type;
                }
                if (!type_is_sup_sub(subtype, env_expr_to_type(e->Cons.arg))) {
                    char err[512];
                    sprintf(err, "invalid constructor \"%s\" : type mismatch", e->Cons.subtype.val.s);
                    OK = err_message(e->Cons.subtype, err);
                }
            }
        }
        return 1;
    }

    int fs (Stmt* s) {
        switch (s->sub) {
            case STMT_RETURN:
                break;
        }
    }

    return OK;
}

///////////////////////////////////////////////////////////////////////////////

// Set all EXPR_VAR that are recursive and not alias to istx=1.
//      f(nat)          -- istx=1
//      return nat      -- istx=1
//      output(&nat)    -- istx=0
//      f(alias_nat)    -- istx=0

void set_vars_istx (Stmt* s) {
    auto int fe (Expr* e);
    visit_stmt(s, NULL, fe, NULL);

    int fe (Expr* e) {
        switch (e->sub) {
            case EXPR_ALIAS:
                // keep istx=0
                return 0;
            case EXPR_VAR: {
                Type* tp = env_expr_to_type(e);
                if (tp->sub==TYPE_USER && !tp->isalias) {
                    Stmt* user = env_id_to_stmt(e->env, tp->User.val.s);
                    if (user->User.isrec) {
                        e->Var.istx = 1;
                    }
                }
                break;
            }
            default:
                break;
        }
        return 1;
    }
}

int env (Stmt* s) {
    set_envs(s);
    if (!check_undeclareds(s)) {
        return 0;
    }
    if (!check_types(s)) {
        return 0;
    }
    set_vars_istx(s);
    return 1;
}
