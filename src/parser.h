typedef enum {
    TYPE_NONE,
    TYPE_UNIT,
    TYPE_NATIVE,
    TYPE_TUPLE,
    TYPE_FUNC
} TYPE;

typedef enum {
    EXPR_NONE = 0,
    EXPR_UNIT,
    EXPR_NATIVE,
    EXPR_VAR,
    EXPR_TUPLE,
    EXPR_INDEX,
    EXPR_CALL
} EXPR;

typedef enum {
    STMT_VAR = 1,
    STMT_CALL,
    STMT_SEQ
} STMT;

///////////////////////////////////////////////////////////////////////////////

typedef struct Type {
    TYPE sub;
    union {
        Tk tk;          // TYPE_NATIVE
        struct {        // TYPE_TUPLE
            int size;
            struct Type* vec;
        } Tuple;
        struct {        // TYPE_FUNC
            struct Type* inp;
            struct Type* out;
        } Func;
    };
} Type;


typedef struct Expr {
    EXPR sub;
    union {
        Tk tk;          // EXPR_NATIVE, EXPR_VAR
        struct {        // EXPR_TUPLE
            int size;
            struct Expr* vec;
        } Tuple;
        struct {        // EXPR_INDEX
            struct Expr* tuple;
            int index;
        } Index;
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
        struct {
            Tk   id;
            Type type;
            Expr init;
        } Var;          // STMT_VAR
        struct {
            int size;
            struct Stmt* vec;
        } Seq;          // STMT_SEQ
    };
} Stmt;

///////////////////////////////////////////////////////////////////////////////

int parser_type (Type* ret);
int parser_expr (Expr* ret);
int parser_stmt (Stmt* ret);
