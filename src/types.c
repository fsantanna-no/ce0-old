#include <assert.h>
#include <string.h>

#include "all.h"

Stmt* env_get (Env* env, const char* xp) {
    while (env != NULL) {
        const char* id = NULL;
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

int types (Stmt s) {
    return 1;
#if 0
        case EXPR_VAR:
            if (env_get(e.env,e.tk.val.s) == NULL) {
                char err[512];
                sprintf(err, "undeclared variable \"%s\"", e.tk.val.s);
                return err_message(e.tk, err);
            }
            out(e.tk.val.s);
            break;
#endif
}
