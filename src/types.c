#include <assert.h>
#include <string.h>

#include "all.h"

int err_message (Tk tk, const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): %s", tk.lin, tk.col, v);
    return 0;
}

Stmt* env_get (Env* env, const char* xp) {
    while (env != NULL) {
        const char* id = NULL;
printf("%p %p %d\n", &id, env->stmt, env->stmt->sub);
        if (env->stmt->sub == STMT_TYPE) {
            id = env->stmt->Type.id.val.s;
        } else {
            assert(env->stmt->sub == STMT_VAR);
            id = env->stmt->Var.id.val.s;
        }
        if (id!=NULL && !strcmp(xp,id)) {
            return env->stmt;
        }
        env = env->prev;
    }
    return NULL;
}

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

///////////////////////////////////////////////////////////////////////////////

int types_type (Type tp) {
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int types_expr (Expr e) {
    switch (e.sub) {
        case EXPR_NONE:
            assert(0 && "bug found");
        case EXPR_UNIT:
        case EXPR_NULL:
        case EXPR_ARG:
        case EXPR_NATIVE:
            break;
        case EXPR_VAR:
            if (env_get(e.env,e.tk.val.s) == NULL) {
                char err[512];
                sprintf(err, "undeclared variable \"%s\"", e.tk.val.s);
                return err_message(e.tk, err);
            }
            break;
        case EXPR_CALL:
            if (!types_expr(*e.Call.func)) {
                return 0;
            }
            if (!types_expr(*e.Call.arg)) {
                return 0;
            }
            break;
        case EXPR_CONS:
            if (!types_expr(*e.Cons.arg)) {
                return 0;
            }
            break;
        case EXPR_TUPLE: {
            for (int i=0; i<e.Tuple.size; i++) {
                if (!types_expr(e.Tuple.vec[i])) {
                    return 0;
                }
            }
            break;
        }
        case EXPR_INDEX:
            if (!types_expr(*e.Index.tuple)) {
                return 0;
            }
            break;
        case EXPR_DISC:
            if (!types_expr(*e.Disc.cons)) {
                return 0;
            }
            break;
        case EXPR_PRED:
            if (!types_expr(*e.Disc.cons)) {
                return 0;
            }
            break;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int types_stmt (Stmt s) {
    switch (s.sub) {
        case STMT_NONE:
            assert(0 && "bug found");
            break;

        case STMT_VAR:
            if (!types_expr(s.Var.init)) {
                return 0;
            }
            break;

        case STMT_TYPE:
            for (int i=0; i<s.Type.size; i++) {
                if (!types_type(s.Type.vec[i].type)) {
                    return 0;
                }
            }
            break;

        case STMT_CALL:
            if (!types_expr(s.call)) {
                return 0;
            }
            break;

        case STMT_SEQ:
            for (int i=0; i<s.Seq.size; i++) {
                if (!types_stmt(s.Seq.vec[i])) {
                    return 0;
                }
            }
            break;

        case STMT_IF:
            if (!types_expr(s.If.cond)) {
                return 0;
            }
            if (!types_stmt(*s.If.true)) {
                return 0;
            }
            if (!types_stmt(*s.If.false)) {
                return 0;
            }
            break;

        case STMT_FUNC:
            if (!types_type(*s.Func.type.Func.out)) {
                return 0;
            }
            if (!types_type(*s.Func.type.Func.inp)) {
                return 0;
            }
            if (!types_stmt(*s.Func.body)) {
                return 0;
            }
            break;

        case STMT_RETURN:
            if (!types_expr(s.ret)) {
                return 0;
            }
            break;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int types (Stmt s) {
    return types_stmt(s);
}
