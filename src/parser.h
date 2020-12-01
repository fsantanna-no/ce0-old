typedef enum {
    EXPR_UNIT,
    EXPR_VAR
} EXPR;

typedef struct Expr {
    EXPR sub;
    union {
        Tk tk;          // EXPR_VAR
    };
} Expr;

int parser_expr (Expr* ret);
