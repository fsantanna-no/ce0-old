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

int ishasrec (Stmt* user) {
    assert(user->sub == STMT_USER);

    auto int aux1 (Env* env1, Stmt* user1);
    auto int aux2 (Env* env2, Type* tp);

    return aux1(user->env, user);

    int aux1 (Env* env1, Stmt* user1) {
        for (int i=0; i<user1->User.size; i++) {
            Sub sub = user1->User.vec[i];
            if (aux2(env1, sub.type)) {
                return 1;
            }
        }
        return 0;
    }

    int aux2 (Env* env2, Type* tp2) {
        if (tp2->isptr) {
            return 0;
        }
        switch (tp2->sub) {
            case TYPE_ANY:
            case TYPE_UNIT:
            case TYPE_NATIVE:
            case TYPE_FUNC:
                return 0;
            case TYPE_TUPLE:
                for (int i=0; i<tp2->Tuple.size; i++) {
                    if (aux2(env2, tp2->Tuple.vec[i])) {
                        return 1;
                    }
                }
                return 0;
            case TYPE_USER: {
                Stmt* user2 = env_id_to_stmt(env2, tp2->User.val.s);
                assert(user2!=NULL && user2->sub==STMT_USER);
                if (user2->User.isrec) {
                    return 1;
                }
                if (aux1(env2,user2)) {
                    return 1;
                }
                return 0;
            }
        }
        assert(0);
    }
}

int fs (Stmt* s) {
    if (s->sub != STMT_USER) {
        return VISIT_CONTINUE;      // only check STMT_USER
    }
    if (s->User.size == 0) {
        return VISIT_CONTINUE;      // ignore pre declarations
    }

    // find `pre` and check isrec
    Stmt* pre = env_id_to_stmt(s->env->prev, s->User.tk.val.s);
    if (pre != NULL) {
        assert(pre->sub==STMT_USER && pre->User.size==0);
        if (pre->User.isrec != s->User.isrec) {
            char err[TK_BUF+256];
            sprintf(err, "invalid type declaration : unmatching predeclaration (ln %d)", pre->tk.lin);
            return err_message(&s->tk, err);
        }
    }

    // @rec must match contents
    if (s->User.isrec != ishasrec(s)) {
        char err[TK_BUF+256];
        if (s->User.isrec) {
            sprintf(err, "invalid type declaration : unexpected `@rec´");
        } else {
            sprintf(err, "invalid declaration : expected `@rec´");
        }
        return err_message(&s->tk, err);
    }
    return VISIT_CONTINUE;
}

///////////////////////////////////////////////////////////////////////////////

int check_dcls (Stmt* s) {
    if (!visit_stmt(0,s,fs,fe,ft)) {
        return 0;
    }
    return 1;
}
