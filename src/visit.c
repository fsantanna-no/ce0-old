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

///////////////////////////////////////////////////////////////////////////////

enum {
    IF_LEFT,
    IF_RIGHT
};

void exec_init (Exec_State* est) {
    est->size = 0;
}

// 0=error, 1=success, EXEC_HALT=aborted

int exec_stmt (Exec_State* est, Stmt* s, F_Stmt fs, F_Expr fe) {
    while (s != NULL) {
        if (fs != NULL) {
            switch (fs(s)) {
                case EXEC_ERROR:            // error stop all
                    return 0;
                case EXEC_CONTINUE:         // continue to children
                    break;
                case EXEC_BREAK:            // continue skip children
                    return EXEC_CONTINUE;
                case EXEC_HALT:             // no error stop all
                    return EXEC_HALT;
            }
        }
        if (fe != NULL) {
            int ret = EXEC_CONTINUE;
            switch (s->sub) {
                case STMT_NONE:
                case STMT_NATIVE:
                case STMT_BREAK:
                case STMT_USER:
                case STMT_SEQ:
                case STMT_LOOP:
                case STMT_FUNC:
                case STMT_BLOCK:
                case STMT_RETURN:
                    break;
                case STMT_CALL:
                    ret = visit_expr(0, s->env, s->Call, fe);
                    break;
                case STMT_VAR:
                    ret = visit_expr(0, s->env, s->Var.init, fe);
                    break;
                case STMT_SET:
                    ret = visit_expr(0,s->env,s->Set.dst,fe) && visit_expr(0,s->env,s->Set.src,fe);
                    break;
                case STMT_IF:
                    ret = visit_expr(0, s->env, s->If.tst, fe);
                    break;
            }
            if (ret != EXEC_CONTINUE) {
                return ret;
            }
        }

        // TODO: CALL
#if 0
        int ret = EXEC_CONTINUE;
        switch (s->sub) {
            case STMT_IF:   // handle in separate b/c of if/else
                break;
            case STMT_CALL:
                break;      // TODO: exec_stmt do body
            default:
                break;
        }
        if (ret != EXEC_CONTINUE) {
            return ret;
        }
#endif

        // PATH

        if (s->sub != STMT_IF) {
            s = s->seqs[1];
        } else {   // STMT_IF
            // ainda nao explorei esse galho
            if (est->cur == est->size) {
                est->cur++;
                est->vec[est->size++] = IF_LEFT;  // escolho esquerda
                s = s->If.true;

            // ainda tenho trabalho a esquerda
            } else if (est->vec[est->cur] == IF_LEFT) {
                est->cur++;
                s = s->If.true;

            // ainda tenho trabalho a direita
            } else if (est->vec[est->cur] == IF_RIGHT) {
                est->cur++;
                s = s->If.false;

            } else {
                assert(0);
            }
        }
    }
    return EXEC_CONTINUE;   // last statement?
}

// 1=more, 0=exhausted  //  fret fs: 0=error, 1=success

//static int N = 0;

int exec1 (Exec_State* est, Stmt* s, F_Stmt fs, F_Expr fe, int* fret) {
    est->cur = 0;
    *fret = exec_stmt(est, s, fs, fe);

//N++;
//if (N > 10000) {
    //dump_stmt(s);
    //printf(">>> [%d] %d\n", s->tk.lin, N);
    //assert(0);
//}

    switch (*fret) {
        case 0:         return 0;
        case 1:         break;
        case EXEC_HALT: break;
    }

    // anda pra cima enquanto todas as ultimas escolhas foram DIR
    while (est->size>0 && est->vec[est->size-1]==IF_RIGHT) {
        est->size--;
    }
    // muda a ultima escolha de ESQ -> DIR e retorna 1 que ainda tem o que percorrer
    if (est->size>0 && est->vec[est->size-1]==IF_LEFT) {
        est->vec[est->size-1] = IF_RIGHT;
        return 1;
    }
    // retorna 0 que o percorrimento acabou
    return 0;
}

int exec (Stmt* s, F_Pre pre, F_Stmt fs, F_Expr fe) {      // 0=error, 1=success
//puts(">>>>>>>>>>>");
//N = 0;
    Exec_State est;
    exec_init(&est);
    while (1) {
        if (pre) {
            pre();
        }
        int ret2;
        int ret1 = exec1(&est, s, fs, fe, &ret2);
        if (ret2 == 0) {            // user returned error
//puts("<<<<<<<<<<<");
            return 0;               // so it's an error
        }
        if (ret1 == 0) {            // no more cases
//puts("<<<<<<<<<<<");
            return 1;   // so it's not an error
        }
    }
    assert(0);
}
