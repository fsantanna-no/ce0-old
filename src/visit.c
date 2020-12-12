#include <assert.h>

#include "all.h"

void visit_type (Type* tp, f_type ft) {
    if (ft!=NULL && !ft(tp)) {
        return;
    }
    switch (tp->sub) {
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_USER:
            break;
        case TYPE_TUPLE:
            for (int i=0; i<tp->Tuple.size; i++) {
                visit_type(&tp->Tuple.vec[i], ft);
            }
            break;
        case TYPE_FUNC:
            visit_type(tp->Func.inp, ft);
            visit_type(tp->Func.out, ft);
            break;
    }
}

void visit_expr (Expr* e, f_expr fe) {
    if (fe!=NULL && !fe(e)) {
        return;
    }
    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_ARG:
        case EXPR_NATIVE:
        case EXPR_VAR:
            break;
        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                visit_expr(&e->Tuple.vec[i], fe);
            }
            break;
        case EXPR_INDEX:
            visit_expr(e->Index.tuple, fe);
            break;
        case EXPR_CALL:
            visit_expr(e->Call.func, fe);
            visit_expr(e->Call.arg, fe);
            break;
        case EXPR_CONS:
            visit_expr(e->Cons.arg, fe);
            break;
        case EXPR_DISC:
            visit_expr(e->Disc.val, fe);
            break;
        case EXPR_PRED:
            visit_expr(e->Disc.val, fe);
            break;
    }
}

void visit_stmt (Stmt* s, f_stmt fs, f_expr fe, f_type ft) {
    if (fs!=NULL && !fs(s)) {
        return;
    }
    switch (s->sub) {
        case STMT_USER:
            for (int i=0; i<s->User.size; i++) {
                visit_type(&s->User.vec[i].type, ft);
            }
            break;
        case STMT_VAR:
            visit_type(&s->Var.type, ft);
            visit_expr(&s->Var.init, fe);
        case STMT_POOL:
            visit_type(&s->Var.type, ft);
            break;
        case STMT_CALL:
            visit_expr(&s->call, fe);
            break;
        case STMT_RETURN:
            visit_expr(&s->ret, fe);
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
