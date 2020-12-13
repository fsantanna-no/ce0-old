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
        case EXPR_ARG:
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

#if 0
int execute_stmt (Stmt* s, F_Stmt fs, F_Expr fe) {
    if (fs!=NULL && !fs(s)) {
        return 0;
    }
    switch (s->sub) {
        case STMT_USER:
            break;
        case STMT_VAR:
            return visit_expr(&s->Var.init, fe);
        case STMT_CALL:
            return visit_expr(&s->Call, fe);
        case STMT_RETURN:
            return visit_expr(&s->Return, fe);
            break;
        case STMT_SEQ:
            for (int i=0; i<s->Seq.size; i++) {
                visit_stmt(&s->Seq.vec[i], fs, fe, ft);
            }
            break;
        case STMT_IF:
            visit_expr(&s->If.cond, fe);
            visit_stmt(s->If.true,  fs, fe, ft);
            visit_stmt(s->If.false, fs, fe, ft);
            break;
        case STMT_FUNC:
            visit_type(&s->Func.type, ft);
            visit_stmt(s->Func.body, fs, fe, ft);
            break;
    }
}
#endif
