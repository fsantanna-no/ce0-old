#include "all.h"

int accept (TK enu) {
    if (ALL.tk1.enu == enu) {
        lexer();
        return 1;
    } else {
        return 0;
    }
}

int err_expected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): expected %s : have %s",
        ALL.tk1.lin, ALL.tk1.col, v, lexer_tk2str(&ALL.tk1));
    return 0;
}

static int parser_expr_one (Expr* ret) {
    // EXPR_UNIT
    if (accept('(')) {
        if (accept(')')) {
            *ret = (Expr) { EXPR_UNIT };
        } else {
            if (!parser_expr(ret)) {
                return 0;
            }
    // EXPR_PARENS
            if (!accept(')')) {
                return err_expected("`)´");
            }
        }
    } else {
        return err_expected("expression");
    }
    return 1;
}

int parser_expr (Expr* ret) {
    return parser_expr_one(ret);
}
