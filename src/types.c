#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

Sub* env_find_sub (Env* env, const char* sub) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            for (int i=0; i<env->stmt->User.size; i++) {
                if (!strcmp(sub, env->stmt->User.vec[i].tk.val.s)) {
                    return &env->stmt->User.vec[i];
                }
            }
        }
        env = env->prev;
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

int types_expr (Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_UNK:
        case EXPR_NATIVE:
        case EXPR_VAR:
        case EXPR_TUPLE:
        case EXPR_NULL:
        case EXPR_INT:
            break;

        case EXPR_UPREF: {
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e->Dnref);
            if (tp->isptr) {
                return err_message(&e->tk, "invalid `\\´ : unexpected pointer type");
            }
            break;
        }

        case EXPR_DNREF: {
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e->Dnref);
            if (!tp->isptr) {
                return err_message(&e->tk, "invalid `\\´ : expected pointer type");
            }
            break;
        }

        case EXPR_INDEX: {
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e->Index.val);
            if (tp->sub!=TYPE_TUPLE || tp->isptr) {
                return err_message(&e->tk, "invalid `.´ : expected tuple type");
            }
            if (0>e->tk.val.n || e->tk.val.n>tp->Tuple.size) {
                return err_message(&e->tk, "invalid `.´ : index is out of range");
            }
            break;
        }

        case EXPR_DISC: {
            Type* __ENV_EXPR_TO_TYPE_FREE__ tp = env_expr_to_type(env, e->Disc.val);
            if (tp->sub!=TYPE_USER || tp->isptr) {
                return err_message(&e->tk, "invalid `.´ : expected user type");
            }
// TODO: check if user type has subtype
            break;
        }

        case EXPR_PRED: {
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e->Pred.val);
            if (tp->sub!=TYPE_USER || tp->isptr) {
                return err_message(&e->tk, "invalid `.´ : expected user type");
            }
// TODO: check if user type has subtype
            break;
        }

        case EXPR_CALL: {
            Type* func __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e->Call.func);
            Type* arg  __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e->Call.arg);

            if (e->Call.func->sub == EXPR_NATIVE) {
                TODO("TODO [check_types]: _f(...)\n");
            } else {
                assert(e->Call.func->sub == EXPR_VAR);
                if (!strcmp(e->Call.func->tk.val.s,"clone")) {
                    TODO("TODO [check_types]: clone(...)\n");
                } else if (!strcmp(e->Call.func->tk.val.s,"std")) {
                    TODO("TODO [check_types]: std(...)\n");
                } else if (!type_is_sup_sub(func->Func.inp, arg, 0)) {
                    char err[TK_BUF+256];
                    sprintf(err, "invalid call to \"%s\" : type mismatch", e->Call.func->tk.val.s);
                    return err_message(&e->Call.func->tk, err);
                }
            }
            break;
        }

        case EXPR_CONS: {
            Sub* sub = env_find_sub(env, e->tk.val.s);
            assert(sub != NULL);
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env,e->Cons);
            if (!type_is_sup_sub(sub->type,tp,0)) {
                char err[TK_BUF+256];
                sprintf(err, "invalid constructor \"%s\" : type mismatch", e->tk.val.s);
                return err_message(&e->tk, err);
            }
        }
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int types_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_NONE:
        case STMT_USER:
        case STMT_FUNC:
        case STMT_SEQ:
        case STMT_LOOP:
        case STMT_BREAK:
        case STMT_BLOCK:
        case STMT_NATIVE:
        case STMT_CALL:
        case STMT_RETURN:   // (STMT_SET of _ret_)
            return 1;

        case STMT_VAR: {
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(s->env,s->Var.init);
            if (!type_is_sup_sub(s->Var.type,tp,0)) {
                char err[TK_BUF+256];
                sprintf(err, "invalid assignment to \"%s\" : type mismatch", s->Var.tk.val.s);
                return err_message(&s->Var.tk, err);
            }
            return 1;
        }

        case STMT_SET: {
            Type* tp_dst __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(s->env,s->Set.dst);
            Type* tp_src __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(s->env,s->Set.src);
            if (!type_is_sup_sub(tp_dst,tp_src,1)) {
                char err[TK_BUF+256];
                if (s->Set.dst->sub == EXPR_VAR) {
                    if (!strcmp(s->Set.dst->tk.val.s,"_ret_")) {
                        return err_message(&s->tk, "invalid return : type mismatch");
                    } else {
                        sprintf(err, "invalid assignment to \"%s\" : type mismatch", s->Set.dst->tk.val.s);
                        return err_message(&s->Set.dst->tk, err);
                    }
                } else {
                    strcpy(err, "invalid assignment : type mismatch");
                    return err_message(&s->tk, err);
                }
            }
            return 1;
        }

        case STMT_IF: {
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(s->env,s->If.tst);
            static Type Type_Bool = { TYPE_USER, 0, .User={TX_USER,{.s="Bool"},0,0} };
            if (!type_is_sup_sub(&Type_Bool, tp, 0)) {
                return err_message(&s->tk, "invalid condition : type mismatch");
            }
            return 1;
        }
    }
    assert(0);
}

///////////////////////////////////////////////////////////////////////////////

int types (Stmt* s) {
    if (!visit_stmt(1,s,types_stmt,types_expr,NULL)) {
        return 0;
    }
    return 1;
}
