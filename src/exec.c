#include <assert.h>

#include "all.h"

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
//printf(">>> [%d] %d\n", s->tk.lin, s->sub);
//dump_stmt(s);
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

        switch (s->sub) {
#if 0
            case STMT_BREAK:
            case STMT_RETURN:
                s = s->seq;     // set to outer stmt in sequence (same as default below)
                break;
#endif

            case STMT_IF:
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
                break;

            default:
                s = s->seq;
                break;
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
