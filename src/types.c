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
