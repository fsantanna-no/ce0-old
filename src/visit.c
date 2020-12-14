#include <assert.h>

#include "all.h"

int visit_type (Type* tp, F_Type ft) {
    if (ft != NULL) {
        switch (ft(tp)) {
            case VISIT_ERROR:
                return 0;
            case VISIT_CONTINUE:
                break;
            case VISIT_BREAK:
                return 1;
        }
    }

    switch (tp->sub) {
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_USER:
            return 1;
        case TYPE_TUPLE:
            for (int i=0; i<tp->Tuple.size; i++) {
                if (!visit_type(&tp->Tuple.vec[i],ft)) {
                    return 0;
                }
            }
            return 1;
        case TYPE_FUNC:
            return (visit_type(tp->Func.inp, ft) && visit_type(tp->Func.out, ft));
    }
    assert(0 && "bug found");
}

int visit_expr (Expr* e, F_Expr fe) {
    if (fe != NULL) {
        switch (fe(e)) {
            case VISIT_ERROR:
                return 0;
            case VISIT_CONTINUE:
                break;
            case VISIT_BREAK:
                return 1;
        }
    }

    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_NATIVE:
        case EXPR_VAR:
            return 1;
        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                if (!visit_expr(&e->Tuple.vec[i], fe)) {
                    return 0;
                }
            }
            return 1;
        case EXPR_INDEX:
            return visit_expr(e->Index.tuple, fe);
        case EXPR_CALL:
            return (visit_expr(e->Call.func, fe) && visit_expr(e->Call.arg, fe));
        case EXPR_ALIAS:
            return visit_expr(e->Alias, fe);
        case EXPR_CONS:
            return visit_expr(e->Cons.arg, fe);
        case EXPR_DISC:
            return visit_expr(e->Disc.val, fe);
        case EXPR_PRED:
            return visit_expr(e->Disc.val, fe);
    }
    assert(0 && "bug found");
}

// 0=error, 1=success

int visit_stmt (Stmt* s, F_Stmt fs, F_Expr fe, F_Type ft) {
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

    switch (s->sub) {
        case STMT_USER:
            for (int i=0; i<s->User.size; i++) {
                if (!visit_type(&s->User.vec[i].type, ft)) {
                    return 0;
                }
            }
            return 1;
        case STMT_VAR:
            return (visit_type(&s->Var.type, ft) && visit_expr(&s->Var.init, fe));
        case STMT_CALL:
            return visit_expr(&s->Call, fe);
        case STMT_RETURN:
            return visit_expr(&s->Return, fe);
        case STMT_SEQ:
            for (int i=0; i<s->Seq.size; i++) {
                if (!visit_stmt(&s->Seq.vec[i], fs, fe, ft)) {
                    return 0;
                }
            }
            return 1;
        case STMT_IF:
            return (
                visit_expr(&s->If.cond, fe)         &&
                visit_stmt(s->If.true,  fs, fe, ft) &&
                visit_stmt(s->If.false, fs, fe, ft)
            );
        case STMT_FUNC:
            return (visit_type(&s->Func.type, ft) && visit_stmt(s->Func.body, fs, fe, ft));
    }
    assert(0 && "bug found");
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

int exec_expr (Exec_State* est, Expr* e, F_Expr fe) {
    if (fe != NULL) {
        switch (fe(e)) {
            case EXEC_ERROR:
                return 0;
            case EXEC_CONTINUE:
                break;
            case EXEC_HALT:
                return EXEC_HALT;
        }
    }

    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_NATIVE:
        case EXPR_VAR:
            return 1;
        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                int ret = exec_expr(est, &e->Tuple.vec[i], fe);
                if (ret != EXEC_CONTINUE) {
                    return ret;
                }
            }
            return 1;
        case EXPR_INDEX:
            return exec_expr(est, e->Index.tuple, fe);
        case EXPR_CALL: {
            int ret = exec_expr(est, e->Call.func, fe);
            if (ret != EXEC_CONTINUE) {
                return ret;
            }
            return exec_expr(est, e->Call.arg, fe);
        }
        case EXPR_ALIAS:
            return exec_expr(est, e->Alias, fe);
        case EXPR_CONS:
            return exec_expr(est, e->Cons.arg, fe);
        case EXPR_DISC:
            return exec_expr(est, e->Disc.val, fe);
        case EXPR_PRED:
            return exec_expr(est, e->Disc.val, fe);
    }
    assert(0 && "bug found");
}

// 0=error, 1=success, EXEC_HALT=aborted

int exec_stmt (Exec_State* est, Stmt* s, F_Stmt fs, F_Expr fe) {
    while (s != NULL) {
        if (fs != NULL) {
            switch (fs(s)) {
                case EXEC_ERROR:
                    return 0;
                case EXEC_CONTINUE:
                    break;
                case EXEC_HALT:
                    return EXEC_HALT;
            }
        }

        int ret;
        switch (s->sub) {
            case STMT_USER:
            case STMT_SEQ:
            case STMT_FUNC:
            case STMT_IF:   // handle in separate b/c of if/else
                break;
            case STMT_VAR:
                ret = exec_expr(est, &s->Var.init, fe);
                break;
            case STMT_CALL:
                ret = exec_expr(est, &s->Call, fe);
// TODO: exec_stmt do body
                break;
            case STMT_RETURN:
                ret = exec_expr(est, &s->Return, fe);
                break;
        }
        if (s->sub != STMT_IF) {
            if (ret != EXEC_CONTINUE) {
                return ret;
            };
            s = s->seq;
        } else {
            int ret = exec_expr(est,&s->If.cond,fe);
            if (ret != EXEC_CONTINUE) {
                return ret;
            };

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
    assert(0);
}

int exec (Exec_State* est, Stmt* s, F_Stmt fs, F_Expr fe, int* fret) {
    est->cur = 0;
    *fret = exec_stmt(est, s, fs, fe);

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
