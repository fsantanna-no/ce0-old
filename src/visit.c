#include <assert.h>

#include "all.h"

int visit_type (Type* tp, F_Type ft, void* arg) {
    if (ft != NULL) {
        switch (ft(tp,arg)) {
            case VISIT_ERROR:
                return 0;
            case VISIT_CONTINUE:
                break;
            case VISIT_BREAK:
                return 1;
        }
    }

    switch (tp->sub) {
        case TYPE_AUTO:
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_USER:
            return 1;
        case TYPE_TUPLE:
            for (int i=0; i<tp->Tuple.size; i++) {
                if (!visit_type(&tp->Tuple.vec[i],ft,arg)) {
                    return 0;
                }
            }
            return 1;
        case TYPE_FUNC:
            return (visit_type(tp->Func.inp,ft,arg) && visit_type(tp->Func.out,ft,arg));
    }
    assert(0 && "bug found");
}

// 0=error, 1=success

int visit_stmt (Stmt* s, F_Stmt fs) {
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
        case STMT_NONE:
        case STMT_RETURN:
        case STMT_USER:
        case STMT_VAR:
            return 1;

        case STMT_SEQ:
            return visit_stmt(s->Seq.s1,fs) && visit_stmt(s->Seq.s2,fs);

        case STMT_IF:
            return (visit_stmt(s->If.true,fs) && visit_stmt(s->If.false,fs));

        case STMT_FUNC:
            return (s->Func.body==NULL) || visit_stmt(s->Func.body,fs);

        case STMT_BLOCK:
            return visit_stmt(s->Block, fs);
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

int exec_stmt (Exec_State* est, Stmt* s, F_Stmt fs) {
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

int exec1 (Exec_State* est, Stmt* s, F_Stmt fs, int* fret) {
    est->cur = 0;
    *fret = exec_stmt(est, s, fs);

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

int exec (Stmt* s, F_Pre pre, F_Stmt fs) {      // 0=error, 1=success
    Exec_State est;
    exec_init(&est);
    while (1) {
        if (pre) {
            pre();
        }
        int ret2;
        int ret1 = exec1(&est, s, fs, &ret2);
        if (ret2 == 0) {            // user returned error
            return 0;               // so it's an error
        }
        if (ret1 == 0) {            // no more cases
            return 1;   // so it's not an error
        }
    }
    assert(0);
}
