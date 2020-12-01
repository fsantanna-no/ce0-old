typedef enum {
    EXPR_UNIT = 1,
    EXPR_VAR,
    EXPR_TUPLE,
    EXPR_CALL
} EXPR;

typedef struct Expr {
    EXPR sub;
    union {
        Tk tk;          // EXPR_VAR
        struct {        // EXPR_TUPLE
            int size;
            struct Expr* vec;
        } Tuple;
        struct {        // EXPR_CALL
            struct Expr* func;
            struct Expr* arg;
        } Call;
    };
} Expr;

int parser_expr (Expr* ret);
