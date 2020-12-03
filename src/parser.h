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
    EXPR_DISC,
    EXPR_PRED
} EXPR;

typedef enum {
    STMT_NONE = 0,
    STMT_VAR,
    STMT_TYPE,
    STMT_CALL,
    STMT_SEQ,
    STMT_IF,
    STMT_FUNC
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
        struct {        // STMT_IF
            struct Expr  cond;
            struct Stmt* true;
            struct Stmt* false;
        } If;
        struct {        // STMT_FUNC
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
int parser_prog  (Stmt* ret);
