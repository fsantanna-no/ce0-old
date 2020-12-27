typedef enum {
    TYPE_AUTO = 0,
    TYPE_UNIT,
    TYPE_NATIVE,
    TYPE_USER,
    TYPE_TUPLE,
    TYPE_FUNC
} TYPE;

typedef enum {
    EXPR_UNIT = 1,
    EXPR_NATIVE,
    EXPR_NULL,
    EXPR_INT,
    EXPR_VAR,
    EXPR_ALIAS,
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
    STMT_SET,
    STMT_CALL,
    STMT_SEQ,
    STMT_IF,
    STMT_FUNC,
    STMT_RETURN,
    STMT_BLOCK,
    STMT_NATIVE
} STMT;

///////////////////////////////////////////////////////////////////////////////

typedef struct Type {
    TYPE sub;
    int isalias;
    union {
        Tk Native;      // TYPE_NATIVE
        Tk User;        // TYPE_USER
        struct {        // TYPE_TUPLE
            int size;                   // 2
            struct Type** vec;          // ((),())
        } Tuple;
        struct {        // TYPE_FUNC
            struct Type* inp;           // : ()
            struct Type* out;           // -> ()
        } Func;
    };
} Type;

extern int _N_;

typedef struct Expr {
    int N;
    EXPR sub;
    union {
        Tk Unit;        // EXPR_UNIT
        Tk Native;      // EXPR_NATIVE
        Tk Null;        // EXPR_NULL
        Tk Int;         // EXPR_INT
        Tk Var;         // EXPR_VAR
        struct Expr* Alias;  // EXPR_ALIAS
        struct {        // EXPR_TUPLE
            int size;                   // 2
            struct Expr** vec;          // (x,y)
        } Tuple;
        struct {        // EXPR_INDEX
            struct Expr* val;           // x
            Tk index;                   // .3
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
            struct Expr* val;           // x
            Tk subtype;                 // .True!
        } Disc;
        struct {        // EXPR_PRED
            struct Expr* val;           // x
            Tk subtype;                 // .True?
        } Pred;
    };
} Expr;

typedef struct {
    Tk    id;                           // True:
    Type* type;                         // ()
} Sub;

struct Env;

typedef struct Stmt {
    int N;
    STMT sub;
    struct Env* env;        // see env.c
    struct Stmt* seqs[2];   // {outer,inner}
    Tk tk;
    union {
        Expr* Call;         // STMT_CALL
        Expr* Return;       // STMT_RETURN
        struct Stmt* Block; // STMT_BLOCK
        struct {            // STMT_VAR
            Tk    id;                    // ns
            Type* type;                  // : Nat
            Expr* init;                  // = n
        } Var;
        struct {            // STMT_USER
            int  isrec;                 // rec
            Tk   id;                    // Bool
            int  size;                  // 2 subs
            Sub* vec;                   // [True,False]
        } User;
        struct {            // STMT_SET
            Expr* dst;
            Expr* src;
        } Set;
        struct {            // STMT_SEQ
            struct Stmt* s1;
            struct Stmt* s2;
        } Seq;
        struct {            // STMT_IF
            Expr* cond;                  // if (tst)
            struct Stmt* true;          // { ... }
            struct Stmt* false;         // else { ... }
        } If;
        struct {            // STMT_FUNC
            Tk id;                      // func f
            Type* type;                 // : () -> ()
            struct Stmt* body;          // { ... }
        } Func;
        struct {            // STMT_NATIVE
            int ispre;
            Tk  tk;
        } Native;
    };
} Stmt;

///////////////////////////////////////////////////////////////////////////////

int parser_type   (Type** ret);
int parser_expr   (Expr** ret);
int parser_stmt   (Stmt** ret);
int parser_stmts  (TK opt, Stmt** ret);
int parser        (Stmt** ret);
