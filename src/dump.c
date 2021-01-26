#include <assert.h>

#include "all.h"

int _SPC_ = 0;

void dump_spc (void) {
    for (int i=0; i<_SPC_; i++) {
        putchar(' ');
    }
}

void dump_type (Type* tp) {
    if (tp->isptr) {
        putchar('\\');
    }
    switch (tp->sub) {
        case TYPE_ANY:
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
    switch (e->sub) {
        case EXPR_UNIT:
            printf("()");
            break;
        case EXPR_UNK:
            printf("?");
            break;
        case EXPR_NATIVE:
            printf("%s", e->tk.val.s);
            break;
        case EXPR_NULL:
            printf("$");
            break;
        case EXPR_INT:
            printf("%d", e->tk.val.n);
            break;
        case EXPR_VAR:
            printf("%s", e->tk.val.s);
            break;
        case EXPR_UPREF:
            putchar('\\');
            dump_expr(e->Upref);
            break;
        case EXPR_DNREF:
            dump_expr(e->Dnref);
            putchar('\\');
            break;
        case EXPR_TUPLE:
            printf("(...)");
            break;
        case EXPR_INDEX:
            dump_expr(e->Index.val);
            printf(".%d", e->tk.val.n);
            break;
        case EXPR_CALL:
            dump_expr(e->Call.func);
            putchar(' ');
            dump_expr(e->Call.arg);
            break;
        case EXPR_CONS:
            printf("cons");
            break;
        case EXPR_DISC:
            dump_expr(e->Disc.val);
            printf(".%s!", e->tk.val.s);
            break;
        case EXPR_PRED:
            dump_expr(e->Pred.val);
            printf(".%s?", e->tk.val.s);
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
            printf("var %s: ", s->Var.tk.val.s);
            dump_type(s->Var.type);
            printf(" = ");
            dump_expr(s->Var.init);
            puts("");
            break;
        case STMT_SET:
            dump_spc();
            printf("set ");
            dump_expr(s->Set.dst);
            printf(" = ");
            dump_expr(s->Set.src);
            puts("");
            break;
        case STMT_USER:
            dump_spc();
            printf("type %s:", s->User.tk.val.s);
            puts("");
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
        case STMT_BREAK:
            dump_spc();
            printf("break");
            puts("");
            break;
        case STMT_LOOP:
            printf("loop:\n");
            _SPC_ += 4;
            dump_stmt(s->Loop);
            break;
        case STMT_FUNC:
            dump_spc();
            printf("func %s:\n", s->Func.tk.val.s);
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
            printf("}");
            puts("");
            break;
        case STMT_RETURN:
            dump_spc();
            printf("return ");
            dump_expr(s->Return);
            puts("");
            break;
        case STMT_NATIVE:
            dump_spc();
            printf("_{ ... }");
            puts("");
            break;
    }
}

void dump_env (Env* env) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            puts(env->stmt->User.tk.val.s);
        } else if (env->stmt->sub == STMT_VAR) {
            puts(env->stmt->Var.tk.val.s);
        } else {
            assert(env->stmt->sub == STMT_FUNC);
            puts(env->stmt->Func.tk.val.s);
        }
        env = env->prev;
    }
}
