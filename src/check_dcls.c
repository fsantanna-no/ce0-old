#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

///////////////////////////////////////////////////////////////////////////////

int ftk (Env* env, Tk* tk, char* var_type) {
    switch (tk->enu) {
        case TK_UNIT:
        case TX_NATIVE:
        case TX_NUM:
            return 1;

        case TX_NULL:
            var_type = "type";
        case TX_USER:
        case TX_VAR: {
            Stmt* decl = env_id_to_stmt(env, tk->val.s);
            if (decl == NULL) {
                if (!strcmp(tk->val.s,"_ret_")) {
                    return err_message(tk, "invalid return : no enclosing function");
                } else {
                    char err[TK_BUF+256];
                    sprintf(err, "undeclared %s \"%s\"", var_type, tk->val.s);
                    return err_message(tk, err);
                }
            }
            return 1;
        }

        default:
            assert(0 && "TODO");
    }
}

int ft (Env* env, Type* tp) {
    if (tp->sub == TYPE_USER) {
        return ftk(env, &tp->User, "type");
    }
    return VISIT_CONTINUE;
}

int fe (Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_NULL:
            return ftk(env, &e->tk, "type");
        case EXPR_VAR:
            return ftk(env, &e->tk, "variable");

        case EXPR_CALL:
            if (e->Call.func->sub != EXPR_VAR) {
                return VISIT_CONTINUE;
            }
            if (!ftk(env, &e->Call.func->tk, "function")) {
                return 0;
            }
            if (!visit_expr(1,env,e->Call.arg,fe)) {
                return 0;
            }
            return VISIT_BREAK;

        case EXPR_CONS: {
            Stmt* user = env_sub_id_to_user_stmt(env, e->tk.val.s);
            if (user == NULL) {
                char err[TK_BUF+256];
                sprintf(err, "undeclared subtype \"%s\"", e->tk.val.s);
                return err_message(&e->tk, err);
            }
            return VISIT_CONTINUE;
        }

        case EXPR_DISC:
        case EXPR_PRED: {
            if (e->tk.enu == TX_NULL) {
                if (!ftk(env, &e->tk, "type")) {
                    return 0;
                }
            } else {
                Stmt* user = env_sub_id_to_user_stmt(env, e->tk.val.s);
                if (user == NULL) {
                    char err[TK_BUF+256];
                    sprintf(err, "undeclared subtype \"%s\"", e->tk.val.s);
                    return err_message(&e->tk, err);
                }
            }
            return VISIT_CONTINUE;
        }

        default:
            return VISIT_CONTINUE;
    }
}

///////////////////////////////////////////////////////////////////////////////

int check_dcls (Stmt* s) {
    if (!visit_stmt(0,s,NULL,fe,ft)) {
        return 0;
    }
    return 1;
}
