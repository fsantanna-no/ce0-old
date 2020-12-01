#include "all.h"

int accept (TK enu) {
    if (ALL.tk1.enu == enu) {
        lexer();
        return 1;
    } else {
        return 0;
    }
}

static int parser_expr_one (Expr* ret) {
    // EXPR_UNIT
    if (accept('(')) {
        if (accept(')')) {
            *ret = (Expr) { EXPR_UNIT };
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

int parser_expr (Expr* ret) {
    return parser_expr_one(ret);
}
