#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

typedef struct Env {
    Stmt* stmt;         // STMT_TYPE, STMT_VAR
    struct Env* prev;
} Env;


int err_message (Tk tk, const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): %s", tk.lin, tk.col, v);
    return 0;
}

Stmt* env_get (Env* env, const char* xp) {
    while (env != NULL) {
        const char* id = NULL;
        switch (env->stmt->sub) {
            case STMT_TYPE:
                id = env->stmt->Type.id.val.s;
                break;
            case STMT_VAR:
                id = env->stmt->Var.id.val.s;
                break;
            case STMT_FUNC:
                id = env->stmt->Func.id.val.s;
                break;
            default:
                assert(0 && "bug found");
        }
        if (id!=NULL && !strcmp(xp,id)) {
            return env->stmt;
        }
        env = env->prev;
    }
    return NULL;
}

Type* env_type (Expr* e) {
    static Type ret;
    switch (e->sub) {
        case EXPR_NONE:
            assert(0 && "bug found");

        case EXPR_UNIT:
            ret = (Type) { TYPE_UNIT };
            return &ret;

        case EXPR_NULL:
            assert(0 && "TODO");

        case EXPR_ARG:
            assert(0 && "TODO");

        case EXPR_NATIVE:
            ret = (Type) { TYPE_NATIVE };
            return &ret;

        case EXPR_VAR: {
            Stmt* s = env_get(e->env, e->tk.val.s);
            assert(s != NULL);
            return (s->sub == STMT_VAR) ? &s->Var.type : &s->Func.type;
        }
        case EXPR_CALL:     // f()
            return env_type(e->Call.func)->Func.out;

        case EXPR_CONS:     // Bool.True()
            ret = (Type) { TYPE_TYPE, {.tk=e->Cons.type} };
            return &ret;

        case EXPR_TUPLE: {
            Type* vec = malloc(e->Tuple.size*sizeof(Type));
            assert(vec != NULL);
            for (int i=0; i<e->Tuple.size; i++) {
                vec[i] = *env_type(&e->Tuple.vec[i]);
            }
            ret = (Type) { TYPE_TUPLE, {.Tuple={e->Tuple.size,vec}} };
            return &ret;
        }

        case EXPR_INDEX:    // x.1
            return &env_type(e->Index.tuple)->Tuple.vec[e->Index.index];

        case EXPR_DISC: {   // x.True
            Type* tp   = env_type(e->Disc.cons);        // Bool
            Stmt* stmt = env_get(e->env, tp->tk.val.s); // type Bool { ... }
            for (int i=0; i<stmt->Type.size; i++) {
                if (!strcmp(stmt->Type.vec[i].id.val.s, e->Disc.subtype.val.s)) {
                    return &stmt->Type.vec[i].type;
                }
            }
            assert(0 && "bug found");
        }

        case EXPR_PRED: {
            Type* ret = malloc(sizeof(Type));
            assert(ret != NULL);
            *ret = (Type) { TYPE_TYPE, {} };
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

int types_type (Env* env, Type* tp) {
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int types_expr (Env* env, Expr* e) {
    e->env = env;
    switch (e->sub) {
        case EXPR_NONE:
            assert(0 && "bug found");
        case EXPR_UNIT:
        case EXPR_NULL:
        case EXPR_ARG:
        case EXPR_NATIVE:
            break;
        case EXPR_VAR:
            if (env_get(env, e->tk.val.s) == NULL) {
                char err[512];
                sprintf(err, "undeclared variable \"%s\"", e->tk.val.s);
                return err_message(e->tk, err);
            }
            break;
        case EXPR_CALL:
            if (!types_expr(env, e->Call.func)) {
                return 0;
            }
            if (!types_expr(env, e->Call.arg)) {
                return 0;
            }
            break;
        case EXPR_CONS:
            if (!types_expr(env, e->Cons.arg)) {
                return 0;
            }
            break;
        case EXPR_TUPLE: {
            for (int i=0; i<e->Tuple.size; i++) {
                if (!types_expr(env, &e->Tuple.vec[i])) {
                    return 0;
                }
            }
            break;
        }
        case EXPR_INDEX:
            if (!types_expr(env, e->Index.tuple)) {
                return 0;
            }
            break;
        case EXPR_DISC:
            if (!types_expr(env, e->Disc.cons)) {
                return 0;
            }
            break;
        case EXPR_PRED:
            if (!types_expr(env, e->Disc.cons)) {
                return 0;
            }
            break;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int types_stmt (Env** env, Stmt* s) {
    switch (s->sub) {
        case STMT_NONE:
            assert(0 && "bug found");
            break;

        case STMT_VAR:
            if (!types_expr(*env, &s->Var.init)) {
                return 0;
            }
            {
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, *env };
                *env = new;
            }
            break;

        case STMT_TYPE:
            for (int i=0; i<s->Type.size; i++) {
                if (!types_type(*env, &s->Type.vec[i].type)) {
                    return 0;
                }
            }
            {
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, *env };
                *env = new;
            }
            break;

        case STMT_CALL:
            if (!types_expr(*env, &s->call)) {
                return 0;
            }
            break;

        case STMT_SEQ:
            for (int i=0; i<s->Seq.size; i++) {
                if (!types_stmt(env, &s->Seq.vec[i])) {
                    return 0;
                }
            }
            break;

        case STMT_IF: {
            if (!types_expr(*env, &s->If.cond)) {
                return 0;
            }
            Env* trash1 = *env;
            if (!types_stmt(&trash1, s->If.true)) {
                return 0;
            }
            Env* trash2 = *env;
            if (!types_stmt(&trash2, s->If.false)) {
                return 0;
            }
            break;
        }

        case STMT_FUNC: {
            if (!types_type(*env, s->Func.type.Func.out)) {
                return 0;
            }
            if (!types_type(*env, s->Func.type.Func.inp)) {
                return 0;
            }
            Env* trash = *env;
            if (!types_stmt(&trash, s->Func.body)) {
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
            if (!types_expr(*env, &s->ret)) {
                return 0;
            }
            break;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int types (Stmt* s) {
    Env* env = NULL;
    return types_stmt(&env, s);
}
