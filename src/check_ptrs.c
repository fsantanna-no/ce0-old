#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

// Rule 7:
// set DST = \SRC
// return \SRC
//  - check if scope of DST<=S

void set_dst_ptr_deepest (Stmt* dst, Env* env, Expr* src) {
    int n=0; Expr* vars[256]; int uprefs[256];
    env_held_vars(env, src, &n, vars, uprefs);
    for (int i=0; i<n; i++) {
        Type* stp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, vars[i]);
            // set cur = \cur\... (cur is a pointer, so not upref)
        Stmt* ssrc = env_id_to_stmt(env, vars[i]->tk.val.s);
        assert(ssrc!=NULL && ssrc->sub==STMT_VAR);
        if (!stp->isptr && uprefs[i]) {
            // acquired w/ =\x: points exactly to x which is in the deepest possible scope
            //assert(dst->Var.ptr_deepest==NULL || src->Var.ptr_deepest->env->depth>dst->Var.ptr_deepest->env->depth);
            dst->Var.ptr_deepest = ssrc;
        } else {
            // acquired w/ =x: points to something that x points, take that scope, not the scope of x
            if (dst->Var.ptr_deepest==NULL || ssrc->Var.ptr_deepest->env->depth>dst->Var.ptr_deepest->env->depth) {
                dst->Var.ptr_deepest = ssrc->Var.ptr_deepest;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

static int FS (Stmt* s) {
    switch (s->sub) {
        case STMT_VAR: {
            if (env_type_ishasptr(s->env,s->Var.type) && strcmp(s->Var.tk.val.s,"arg")) {
                s->Var.ptr_deepest = NULL;
            } else {
                s->Var.ptr_deepest = s;
            }

            set_dst_ptr_deepest(s, s->env, s->Var.init);

            // var x: \Int = ?
            if (s->Var.ptr_deepest == NULL) {
                s->Var.ptr_deepest = s; // assumes ? has the same scope of x
            }
            break;
        }

        case STMT_SET: {
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(s->env, s->Set.dst);
            if (!env_type_ishasptr(s->env,tp)) {
                break;
            }

            Stmt* dst = env_expr_leftmost_decl(s->env, s->Set.dst);
            assert(dst->sub == STMT_VAR);

            set_dst_ptr_deepest(dst, s->env, s->Set.src);
            Tk* src_var = &dst->Var.ptr_deepest->Var.tk;

            // my blk depth vs assign depth
            char err[TK_BUF+256];
            if (dst->env->depth < dst->Var.ptr_deepest->env->depth) {
                if (s->Set.dst->sub==EXPR_VAR && !strcmp(s->Set.dst->tk.val.s,"_ret_")) {
                    sprintf(err, "invalid return : cannot return local pointer \"%s\" (ln %d)",
                            src_var->val.s, src_var->lin);
                } else {
                    sprintf(err, "invalid assignment : cannot hold pointer \"%s\" (ln %d) in outer scope",
                            src_var->val.s, src_var->lin);
                }
                err_message(&s->tk, err);
                return EXEC_ERROR;
            } else if (!strcmp(dst->Var.tk.val.s,"arg")) {
                sprintf(err, "invalid assignment : cannot hold pointer in \"arg\" : unkown scope");
                err_message(&s->tk, err);
                return EXEC_ERROR;
            }
            break;
        }

        default:
            break;
    }
    return EXEC_CONTINUE;
}

///////////////////////////////////////////////////////////////////////////////

int check_ptrs (Stmt* S) {
    if (!exec(stmt_xmost(S,0),NULL,FS,NULL)) {
        return 0;
    }

    int fs (Stmt* s) {
        if (s->sub==STMT_FUNC && s->Func.body!=NULL &&
            !exec(stmt_xmost(s->Func.body,0),NULL,FS,NULL)
        ) {
            return VISIT_ERROR;
        }
        return VISIT_CONTINUE;
    }
    if (!visit_stmt(0,S,fs,NULL,NULL)) {
        return 0;
    }
    return 1;
}
