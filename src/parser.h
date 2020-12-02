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
    STMT_DECL = 1,
    STMT_CALL
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
        struct {
            Tk   var;
            Type type;
            Expr init;
        } Decl;         // STMT_DECL
        Expr call;      // STMT_CALL
    };
} Stmt;

///////////////////////////////////////////////////////////////////////////////

int parser_type (Type* ret);
int parser_expr (Expr* ret);
int parser_stmt (Stmt* ret);
