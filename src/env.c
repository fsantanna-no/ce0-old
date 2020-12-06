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
        case EXPR_NONE:
            assert(0 && "bug found");

        case EXPR_UNIT:
            ret = (Type) { TYPE_UNIT, e->env };
            return &ret;

        case EXPR_NIL:
            assert(0 && "TODO");

        case EXPR_ARG:
            assert(0 && "TODO");

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
            Type* tp = env_expr_type(e->Disc.cons);        // Bool
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

#if 0
void env_dump (Env* env) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_TYPE) {
            puts(env->stmt->Type.id.val.s);
        } else {
            assert(env->stmt->sub == STMT_VAR);
            puts(env->stmt->Var.id.val.s);
        }
        env = env->prev;
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////

int env_type (Env* env, Type* tp) {
    tp->env = env;
    switch (tp->sub) {
        case TYPE_NONE:
            assert(0 && "bug found");
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_USER:
            break;
        case TYPE_TUPLE:
            for (int i=0; i<tp->Tuple.size; i++) {
                env_type(env, &tp->Tuple.vec[i]);
            }
            break;
        default:
            assert(0 && "TODO");
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int env_expr (Env* env, Expr* e) {
    e->env = env;
    switch (e->sub) {
        case EXPR_NONE:
            assert(0 && "bug found");
        case EXPR_UNIT:
        case EXPR_NIL:
        case EXPR_ARG:
        case EXPR_NATIVE:
            break;
        case EXPR_VAR:
            if (env_find_decl(env, e->tk.val.s) == NULL) {
                char err[512];
                sprintf(err, "undeclared variable \"%s\"", e->tk.val.s);
                return err_message(e->tk, err);
            }
            break;
        case EXPR_CALL:
            if (!env_expr(env, e->Call.func)) {
                return 0;
            }
            if (!env_expr(env, e->Call.arg)) {
                return 0;
            }
            break;
        case EXPR_CONS:
            if (!env_expr(env, e->Cons.arg)) {
                return 0;
            }
            break;
        case EXPR_TUPLE: {
            for (int i=0; i<e->Tuple.size; i++) {
                if (!env_expr(env, &e->Tuple.vec[i])) {
                    return 0;
                }
            }
            break;
        }
        case EXPR_INDEX:
            if (!env_expr(env, e->Index.tuple)) {
                return 0;
            }
            break;
        case EXPR_DISC:
            if (!env_expr(env, e->Disc.cons)) {
                return 0;
            }
            break;
        case EXPR_PRED:
            if (!env_expr(env, e->Disc.cons)) {
                return 0;
            }
            break;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int env_stmt (Env** env, Stmt* s) {
    switch (s->sub) {
        case STMT_NONE:
            assert(0 && "bug found");
            break;

        case STMT_VAR:
            if (!env_expr(*env, &s->Var.init)) {
                return 0;
            }
            if (!env_type(*env, &s->Var.type)) {
                return 0;
            }
            {
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, *env };
                *env = new;
            }
            break;

        case STMT_USER:
            {   // change env fisrt b/c of rec
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, *env };
                *env = new;
            }
            for (int i=0; i<s->User.size; i++) {
                if (!env_type(*env, &s->User.vec[i].type)) {
                    return 0;
                }
            }
            break;

        case STMT_CALL:
            if (!env_expr(*env, &s->call)) {
                return 0;
            }
            break;

        case STMT_SEQ:
            for (int i=0; i<s->Seq.size; i++) {
                if (!env_stmt(env, &s->Seq.vec[i])) {
                    return 0;
                }
            }
            break;

        case STMT_IF: {
            if (!env_expr(*env, &s->If.cond)) {
                return 0;
            }
            Env* trash1 = *env;
            if (!env_stmt(&trash1, s->If.true)) {
                return 0;
            }
            Env* trash2 = *env;
            if (!env_stmt(&trash2, s->If.false)) {
                return 0;
            }
            break;
        }

        case STMT_FUNC: {
            if (!env_type(*env, s->Func.type.Func.out)) {
                return 0;
            }
            if (!env_type(*env, s->Func.type.Func.inp)) {
                return 0;
            }
            Env* trash = *env;
            if (!env_stmt(&trash, s->Func.body)) {
                return 0;
            }
            {   // TODO: rec function must change env before body
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, *env };
                *env = new;
            }
            break;
        }

        case STMT_RETURN:
            if (!env_expr(*env, &s->ret)) {
                return 0;
            }
            break;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

static Stmt out_stmt = { STMT_VAR, .Var={ {TX_VAR,{.s="output"},0,0},{TYPE_FUNC},{EXPR_NONE} } };
static Env  out_env  = { &out_stmt, NULL };

int env (Stmt* s) {
    Env* env = &out_env;
    return env_stmt(&env, s);
}
