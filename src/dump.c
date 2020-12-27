#include <assert.h>

#include "all.h"

int _SPC_ = 0;

void dump_spc (void) {
    for (int i=0; i<_SPC_; i++) {
        putchar(' ');
    }
}

void dump_type (Type* tp) {
    if (tp->isalias) {
        putchar('&');
    }
    switch (tp->sub) {
        case TYPE_AUTO:
            printf("?");
            break;
        case TYPE_UNIT:
            printf("()");
            break;
        case TYPE_NATIVE:
            printf("%s", tp->Native.val.s);
            break;
        case TYPE_USER:
            printf("%s", tp->User.val.s);
            break;
        case TYPE_TUPLE:
            printf("(...)");
            break;
        case TYPE_FUNC:
            printf("a -> b");
            break;
    }
}

void dump_expr (Expr* e) {
    //printf("[%s] ", e->tk.val.s);
    switch (e->sub) {
        case EXPR_UNIT:
            printf("()");
            break;
        case EXPR_NATIVE:
            printf("%s", e->Native.val.s);
            break;
        case EXPR_NULL:
            printf("$");
            break;
        case EXPR_INT:
            printf("%d", e->Int.val.n);
            break;
        case EXPR_VAR:
            printf("%s", e->Var.val.s);
            break;
        case EXPR_ALIAS:
            putchar('&');
            dump_expr(e->Alias);
            break;
        case EXPR_TUPLE:
            printf("(...)");
            break;
        case EXPR_INDEX:
            dump_expr(e->Index.val);
            printf(".%d", e->Index.index.val.n);
            break;
        case EXPR_CALL:
            dump_expr(e->Call.func);
            putchar(' ');
            dump_expr(e->Call.arg);
            break;
        case EXPR_CONS:
            printf("cons\n");
            break;
        case EXPR_DISC:
            printf("disc\n");
            break;
        case EXPR_PRED:
            printf("pred\n");
            break;
    }
}

void dump_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_NONE:
            break;
        case STMT_CALL:
            dump_spc();
            dump_expr(s->Call);
            puts("");
            break;
        case STMT_VAR:
            dump_spc();
            printf("var %s: ", s->Var.id.val.s);
            dump_type(s->Var.type);
            printf(" = ");
            dump_expr(s->Var.init);
            break;
        case STMT_USER:
            dump_spc();
            printf("type %s:\n", s->User.id.val.s);
            break;
        case STMT_SEQ:
            dump_stmt(s->Seq.s1);
            dump_stmt(s->Seq.s2);
            break;
        case STMT_IF:
            dump_spc();
            printf("if:\n");
            _SPC_ += 4;
            dump_stmt(s->If.true);
            _SPC_ -= 4;
            dump_spc();
            printf("else:\n");
            _SPC_ += 4;
            dump_stmt(s->If.false);
            _SPC_ -= 4;
            break;
        case STMT_FUNC:
            dump_spc();
            printf("func %s:\n", s->Func.id.val.s);
            if (s->Func.body != NULL) {
                _SPC_ += 4;
                dump_stmt(s->Func.body);
                _SPC_ -= 4;
            }
            break;
        case STMT_BLOCK:
            dump_spc();
            printf("{\n");
            _SPC_ += 4;
            dump_stmt(s->Block);
            _SPC_ -= 4;
            dump_spc();
            printf("}\n");
            break;
        case STMT_RETURN:
            dump_spc();
            printf("return ");
            dump_expr(s->Return);
            puts("");
            break;
        case STMT_NATIVE:
            dump_spc();
            printf("_{ ... }\n");
            break;
        default:
            printf("ERR: %d\n", s->sub);
            assert(0);
    }
}

void dump_env (Env* env) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            puts(env->stmt->User.id.val.s);
        } else if (env->stmt->sub == STMT_VAR) {
            puts(env->stmt->Var.id.val.s);
        } else {
            assert(env->stmt->sub == STMT_FUNC);
            puts(env->stmt->Func.id.val.s);
        }
        env = env->prev;
    }
}
