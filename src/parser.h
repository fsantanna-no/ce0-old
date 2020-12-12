typedef enum {
    TYPE_UNIT = 1,
    TYPE_NATIVE,
    TYPE_USER,
    TYPE_TUPLE,
    TYPE_FUNC
} TYPE;

typedef enum {
    EXPR_UNIT = 1,
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
    STMT_VAR = 1,
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
    struct Env* env;    // see env.c
    union {
        Tk tk;          // TYPE_NATIVE, TYPE_USER
        struct {        // TYPE_TUPLE
            int size;                   // 2
            struct Type* vec;           // ((),())
        } Tuple;
        struct {        // TYPE_FUNC
            struct Type* inp;           // : ()
            struct Type* out;           // -> ()
        } Func;
    };
} Type;

struct Env;

extern int _N_;

typedef struct Expr {
    int N;
    EXPR sub;
    struct Env* env;    // see env.c
    union {
        Tk tk;          // EXPR_NATIVE, EXPR_VAR
        struct {        // EXPR_TUPLE
            int size;                   // 2
            struct Expr* vec;           // (x,y)
        } Tuple;
        struct {        // EXPR_INDEX
            struct Expr* tuple;         // x
            int index;                  // .3
        } Index;
        struct {        // EXPR_CALL
            struct Expr* func;          // f
            struct Expr* arg;           // (x,y)
        } Call;
        struct {        // EXPR_CONS
            Tk subtype;                 // .True
            struct Expr* arg;
        } Cons;
        struct {        // EXPR_DISC
            struct Expr* val;           // Bool.True
            Tk subtype;                 // .True!
        } Disc;
        struct {        // EXPR_PRED
            struct Expr* val;           // Bool.True
            Tk subtype;                 // .True?
        } Pred;
    };
} Expr;

typedef struct {
    Tk   id;                            // True:
    Type type;                          // ()
} Sub;

typedef struct Stmt {
    int N;
    STMT sub;
    struct Env* env;    // see env.c
    union {
        Expr call;      // STMT_CALL
        Expr ret;       // STMT_RETURN
        struct {
            Tk   id;                    // ns
            Type type;                  // : Nat
            Expr init;                  // = n
        } Var;          // STMT_VAR
        struct {
            int  isrec;                 // rec
            Tk   id;                    // Bool
            int  size;                  // 2 subs
            Sub* vec;                   // [True,False]
        } User;         // STMT_USER
        struct {
            int size;                   // 3
            struct Stmt* vec;           // a ; b ; c
        } Seq;          // STMT_SEQ
        struct {        // STMT_IF
            struct Expr  cond;          // if (tst)
            struct Stmt* true;          // { ... }
            struct Stmt* false;         // else { ... }
        } If;
        struct {        // STMT_FUNC
            Tk id;                      // func f
            Type type;                  // : () -> ()
            struct Stmt* body;          // { ... }
        } Func;
    };
} Stmt;

///////////////////////////////////////////////////////////////////////////////

int parser_type  (Type* ret);
int parser_expr  (Expr* ret);
int parser_stmt  (Stmt* ret);
int parser_stmts (Stmt* ret);
int parser       (Stmt* ret);
