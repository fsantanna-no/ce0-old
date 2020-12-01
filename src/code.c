#include <stdio.h>

#include "all.h"

void out (const char* v) {
    fputs(v, ALL.out);
}

void code_expr (Expr e) {
    switch (e.sub) {
        case EXPR_UNIT:
            out("1");
            break;
        case EXPR_NATIVE:
            out(&e.tk.val.s[1]);
            break;
        case EXPR_VAR:
            out(e.tk.val.s);
            break;
    }
}
