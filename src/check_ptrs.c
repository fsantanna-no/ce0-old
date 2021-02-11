#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

///////////////////////////////////////////////////////////////////////////////

#if 0
static int FE (Env env, Expr* e) {
    if (e->sub != EXPR_CONS) {
        return EXEC_CONTINUE;
    }
    ;
}
#endif

// Cannot hold pointer in recursive data:
//      set x.1 = \y    <-- if x is rec, it might be moved outside scope of y
// Only cycles are allowed:
//      set x.1 = \x.2
// Cycles only possible in `set`, but not in `var`.

static int FS1 (Stmt* s) {
    switch (s->sub) {
        case STMT_VAR: {
            int ishasrec = env_type_ishasrec(s->env, s->Var.type);
            if (ishasrec) {
                int n=0; Expr* vars[256]; int uprefs[256];
                env_held_vars(s->env, s->Var.init, &n, vars, uprefs);
                if (n > 0) {
                    char err[TK_BUF+256];
                    sprintf(err, "invalid assignment : cannot hold pointer to \"%s\" in recursive value",
                            vars[0]->tk.val.s);
                    err_message(&vars[0]->tk, err);
                    return VISIT_ERROR;
                }
            }
            break;
        }

        case STMT_SET: {
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(s->env, s->Set.dst);
puts("-=-=-=-=-");
dump_stmt(s);
dump_type(tp); puts(" <<<");
            if (!env_type_ishasptr(s->env,tp)) {
puts("BREAK");
                break;
            }

            Stmt* dst = env_expr_leftmost_decl(s->env, s->Set.dst);
            assert(dst!=NULL && dst->sub==STMT_VAR);

// achar prefixo em comum a partir de leftmost
// pegar ishasrec a partir dai

            int ishasrec = env_type_ishasrec(s->env, dst->Var.type);
// tenho que ver se alguem no caminho, que faz x\, tem ishasrec
printf("ISREC = %d\n", ishasrec);
            if (ishasrec) {
                Expr* non = env_held_vars_nonself(s->env, dst->Var.tk.val.s, s->Set.src);
printf("NON = %p\n", non);
                if (non != NULL) {
                    char err[TK_BUF+256];
                    sprintf(err, "invalid assignment : cannot hold pointer to \"%s\" in recursive value",
                            non->tk.val.s);
                    err_message(&non->tk, err);
                    return VISIT_ERROR;
                }
            } else if (s->Set.dst->sub != EXPR_VAR) { //if (!strcmp(dst->Var.tk.val.s,"arg")) {
                int n=0; Expr* vars[256]; int uprefs[256];
                env_held_vars(s->env, s->Set.src, &n, vars, uprefs);
                char err[TK_BUF+256];
                sprintf(err, "invalid assignment : cannot hold pointer to \"%s\"",
                        vars[0]->tk.val.s);
                err_message(&vars[0]->tk, err);
                return VISIT_ERROR;
            }
            break;
        }

        default:
            break;
    }
    return VISIT_CONTINUE;
}

///////////////////////////////////////////////////////////////////////////////

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

static int FS2 (Stmt* s) {
    switch (s->sub) {
        case STMT_VAR: {
            int ishasptr = env_type_ishasptr(s->env, s->Var.type);

            if (ishasptr && strcmp(s->Var.tk.val.s,"arg")) {
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
                    sprintf(err, "invalid return : cannot return local pointer to \"%s\" (ln %d)",
                            src_var->val.s, src_var->lin);
                } else {
                    sprintf(err, "invalid assignment : cannot hold pointer to \"%s\" (ln %d) in outer scope",
                            src_var->val.s, src_var->lin);
                }
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
    if (!visit_stmt(0,S,FS1,NULL,NULL)) {
        return 0;
    }

    // ptr_deepest requires EXEC
    {
        if (!exec(stmt_xmost(S,0),NULL,FS2,NULL)) {
            return 0;
        }

        int fs (Stmt* s) {
            if (s->sub==STMT_FUNC && s->Func.body!=NULL &&
                !exec(stmt_xmost(s->Func.body,0),NULL,FS2,NULL)
            ) {
                return VISIT_ERROR;
            }
            return VISIT_CONTINUE;
        }
        if (!visit_stmt(0,S,fs,NULL,NULL)) {
            return 0;
        }
    }
    return 1;
}
