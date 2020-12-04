typedef enum {
    TYPE_NONE,
    TYPE_UNIT,
    TYPE_NATIVE,
    TYPE_USER,
    TYPE_TUPLE,
    TYPE_FUNC
} TYPE;

typedef enum {
    EXPR_NONE = 0,
    EXPR_UNIT,
    EXPR_NIL,
    EXPR_ARG,
    EXPR_NATIVE,
    EXPR_VAR,
    EXPR_TUPLE,
    EXPR_INDEX,
    EXPR_CALL,
    EXPR_CONS,
    EXPR_DISC,
    EXPR_PRED
} EXPR;

typedef enum {
    STMT_NONE = 0,
    STMT_VAR,
    STMT_USER,
    STMT_CALL,
    STMT_SEQ,
    STMT_IF,
    STMT_FUNC,
    STMT_RETURN
} STMT;

///////////////////////////////////////////////////////////////////////////////

typedef struct Type {
    TYPE sub;
    struct Env* env;    // see types.c
    union {
        Tk tk;          // TYPE_NATIVE, TYPE_USER
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

struct Env;

typedef struct Expr {
    EXPR sub;
    struct Env* env;    // see types.c
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
        struct {        // EXPR_DISC
            struct Expr* cons;
            Tk subtype;
        } Disc;
        struct {        // EXPR_PRED
            struct Expr* cons;
            Tk subtype;
        } Pred;
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
        Expr ret;       // STMT_RETURN
        struct {
            Tk   id;
            Type type;
            Expr init;
        } Var;          // STMT_VAR
        struct {
            int  isrec;
            Tk   id;        // Bool
            int  size;      // 2 subs
            Sub* vec;       // [True,False]
        } User;         // STMT_USER
        struct {
            int size;
            struct Stmt* vec;
        } Seq;          // STMT_SEQ
        struct {        // STMT_IF
            struct Expr  cond;
            struct Stmt* true;
            struct Stmt* false;
        } If;
        struct {        // STMT_FUNC
            Tk id;
            Type type;
            struct Stmt* body;
        } Func;
    };
} Stmt;

///////////////////////////////////////////////////////////////////////////////

int parser_type  (Type* ret);
int parser_expr  (Expr* ret);
int parser_stmt  (Stmt* ret);
int parser_stmts (Stmt* ret);
int parser       (Stmt* ret);
