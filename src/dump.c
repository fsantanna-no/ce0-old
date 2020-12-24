#include <assert.h>

#include "all.h"

int _SPC_ = 0;

void dump_spc (void) {
    for (int i=0; i<_SPC_; i++) {
        putchar(' ');
    }
}

void dump_exp1 (Exp1* e) {
    dump_spc();
    //printf("[%d] %d (%d/%s) %d\n", e->N, e->sub, e->tk.enu,e->tk.val.s, e->isalias);
    if (e->isalias) {
        putchar('&');
    }
    switch (e->sub) {
        case EXPR_UNIT:
            printf("()\n");
            break;
        case EXPR_NATIVE:
            printf("%s\n", e->tk.val.s);
            break;
        case EXPR_NULL:
            printf("null\n");
            break;
        case EXPR_VAR:
            printf("%s\n", e->tk.val.s);
            break;
        case EXPR_TUPLE:
            printf("(...)\n");
            break;
        case EXPR_INDEX:
            printf("(...)\n");
            break;
        case EXPR_CALL:
            printf("%s(%s)\n", e->Call.func.val.s, e->Call.arg.val.s);
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
    dump_spc();
    switch (s->sub) {
        case STMT_NONE:
            break;
        case STMT_VAR:
            printf("var %s:\n", s->Var.id.val.s);
            _SPC_ += 4;
            dump_exp1(&s->Var.init);
            _SPC_ -= 4;
            break;
        case STMT_USER:
            printf("type %s:\n", s->User.id.val.s);
            break;
        case STMT_SEQ:
            dump_stmt(s->Seq.s1);
            dump_stmt(s->Seq.s2);
            break;
        case STMT_IF:
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
            printf("func %s:\n", s->Func.id.val.s);
            break;
        case STMT_BLOCK:
            printf("{\n");
            _SPC_ += 4;
            dump_stmt(s->Block);
            _SPC_ -= 4;
            dump_spc();
            printf("}\n");
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
