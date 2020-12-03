typedef enum {
    TYPE_NONE,
    TYPE_UNIT,
    TYPE_NATIVE,
    TYPE_TYPE,
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
    EXPR_CALL,
    EXPR_CONS,
    EXPR_DEST
} EXPR;

typedef enum {
    STMT_VAR = 1,
    STMT_TYPE,
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
        struct {        // EXPR_CONS
            Tk type;
            Tk subtype;
            struct Expr* arg;
        } Cons;
        struct {        // EXPR_DEST
            struct Expr* cons;
            Tk subtype;
        } Dest;
    };
} Expr;

typedef struct {
    Tk   id;        // True
    Type type;      // ()
} Sub;

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
            Tk   id;        // Bool
            int  size;      // 2 subs
            Sub* vec;       // [True,False]
        } Type;         // STMT_TYPE
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
int parser_prog (Stmt* ret);
