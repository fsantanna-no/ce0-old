typedef enum {
    EXPR_UNIT = 1,
    EXPR_NATIVE,
    EXPR_VAR,
    EXPR_TUPLE,
    EXPR_CALL
} EXPR;

typedef enum {
    STMT_CALL = 1
} STMT;

///////////////////////////////////////////////////////////////////////////////

typedef struct Expr {
    EXPR sub;
    union {
        Tk tk;          // EXPR_NATIVE, EXPR_VAR
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

typedef struct Stmt {
    STMT sub;
    union {
        Expr call;      // STMT_CALL
    };
} Stmt;

///////////////////////////////////////////////////////////////////////////////

int parser_expr (Expr* ret);
int parser_stmt (Stmt* ret);
