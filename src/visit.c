#include <assert.h>

#include "all.h"

int visit_type (Env* env, Type* tp, F_Type ft) {
    switch (tp->sub) {
        case TYPE_ANY:
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_USER:
            break;
        case TYPE_TUPLE:
            for (int i=0; i<tp->Tuple.size; i++) {
                int ret = visit_type(env,tp->Tuple.vec[i],ft);
                if (ret != VISIT_CONTINUE) {
                    return ret;
                }
            }
            break;
        case TYPE_FUNC: {
            int ret = (visit_type(env,tp->Func.inp,ft) && visit_type(env,tp->Func.out,ft));
            if (ret != VISIT_CONTINUE) {
                return ret;
            }
        }
    }
    //assert(0 && "bug found");

    if (ft != NULL) {
        switch (ft(env,tp)) {
            case VISIT_ERROR:
                return 0;
            case VISIT_CONTINUE:
                return 1;
            case VISIT_BREAK:
                assert(0);
        }
    }
    return 1;
}

// 0=error, 1=success

int visit_stmt_ (int ord, Stmt* s, F_Stmt fs, F_Expr fe, F_Type ft) {
    switch (s->sub) {
        case STMT_NONE:
        case STMT_NATIVE:
        case STMT_BREAK:
        case STMT_RETURN:
            return 1;

        case STMT_CALL:
            return visit_expr(ord, s->env, s->Call, fe);

        case STMT_VAR:
            return visit_type(s->env,s->Var.type,ft) && visit_expr(ord,s->env,s->Var.init,fe);

        case STMT_SET:
            return visit_expr(ord,s->env,s->Set.dst,fe) && visit_expr(ord,s->env,s->Set.src,fe);

        case STMT_USER:
            for (int i=0; i<s->User.size; i++) {
                if (!visit_type(s->env,s->User.vec[i].type,ft)) {
                    return 0;
                }
            }
            return 1;

        case STMT_SEQ:
            return visit_stmt(ord,s->Seq.s1,fs,fe,ft) && visit_stmt(ord,s->Seq.s2,fs,fe,ft);

        case STMT_IF:
            return visit_expr(ord,s->env,s->If.tst,fe) && visit_stmt(ord,s->If.true,fs,fe,ft) && visit_stmt(ord,s->If.false,fs,fe,ft);

        case STMT_LOOP:
            return visit_stmt(ord,s->Loop,fs,fe,ft);

        case STMT_FUNC:
            if (s->Func.body == NULL) {
                return visit_type(s->env,s->Func.type,ft);
            } else {
                return visit_type(s->env,s->Func.type,ft) && visit_stmt(ord,s->Func.body,fs,fe,ft);
            }

        case STMT_BLOCK:
            return visit_stmt(ord,s->Block, fs,fe,ft);
    }
    assert(0 && "bug found");
}

int visit_stmt (int ord, Stmt* s, F_Stmt fs, F_Expr fe, F_Type ft) {
    if (ord == 0) {
        if (fs != NULL) {
            switch (fs(s)) {
                case VISIT_ERROR:
                    return 0;
                case VISIT_CONTINUE:
                    break;
                case VISIT_BREAK:
                    return 1;
            }
        }
        return visit_stmt_(ord, s, fs, fe, ft);
    } else {
        int ret = visit_stmt_(ord, s, fs, fe, ft);
        if (ret != VISIT_CONTINUE) {
            return ret;
        }
        if (fs!=NULL && fs(s)==VISIT_ERROR) {
            return 0;
        } else {
            return 1;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

int visit_expr_ (int ord, Env* env, Expr* e, F_Expr fe) {
    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_UNK:
        case EXPR_NATIVE:
        case EXPR_NULL:
        case EXPR_INT:
        case EXPR_VAR:
            return 1;
        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                if (!visit_expr(ord, env, e->Tuple.vec[i], fe)) {
                    return 0;
                }
            }
            return 1;
        case EXPR_INDEX:
            return visit_expr(ord, env, e->Index.val, fe);
        case EXPR_CALL:
            return visit_expr(ord,env,e->Call.func,fe) && visit_expr(ord,env,e->Call.arg,fe);
        case EXPR_UPREF:
            return visit_expr(ord, env, e->Upref, fe);
        case EXPR_DNREF:
            return visit_expr(ord, env, e->Dnref, fe);
        case EXPR_CONS:
            return visit_expr(ord, env, e->Cons, fe);
        case EXPR_DISC:
            return visit_expr(ord, env, e->Disc.val, fe);
        case EXPR_PRED:
            return visit_expr(ord, env, e->Disc.val, fe);
    }
    assert(0 && "bug found");
}

int visit_expr (int ord, Env* env, Expr* e, F_Expr fe) {
    if (ord == 0) {
        if (fe != NULL) {
            switch (fe(env,e)) {
                case VISIT_ERROR:
                    return 0;
                case VISIT_CONTINUE:
                    break;
                case VISIT_BREAK:
                    return 1;
            }
        }
        return visit_expr_(ord, env, e, fe);
    } else {
        int ret = visit_expr_(ord, env, e, fe);
        if (ret != VISIT_CONTINUE) {
            return ret;
        }
        if (fe!=NULL && fe(env,e)==VISIT_ERROR) {
            return 0;
        } else {
            return 1;
        }
    }
}
