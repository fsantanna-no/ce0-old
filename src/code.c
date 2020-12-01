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
        case EXPR_CALL:
            code_expr(*e.Call.func);
            out("(");
            code_expr(*e.Call.arg);
            out(")");
            break;
    }
}

void code_stmt (Stmt s) {
    switch (s.sub) {
        case STMT_CALL:
            code_expr(s.call);
            out(";\n");
            break;
    }
}

void code (Stmt s) {
    out (
        "#include <assert.h>\n"
        "#include <stdio.h>\n"
        "#define show_Unit(x) (assert(x==1), puts(\"()\"))\n"
        "int main (void) {\n"
        "\n"
    );
    code_stmt(s);
    fprintf(ALL.out, "\n");
    out("}\n");
}
