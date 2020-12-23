typedef enum {
    TYPE_UNIT = 1,
    TYPE_NATIVE,
    TYPE_USER,
    TYPE_TUPLE,
    TYPE_FUNC
} TYPE;

typedef enum {
    EXPR_UNIT = 1,
    EXPR_NATIVE,
    EXPR_NULL,
    EXPR_VAR,
    //
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
    STMT_SEQ,
    STMT_IF,
    STMT_FUNC,
    STMT_RETURN,
    STMT_BLOCK
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
            struct Type* vec;           // ((),())
        } Tuple;
        struct {        // TYPE_FUNC
            struct Type* inp;           // : ()
            struct Type* out;           // -> ()
        } Func;
    };
} Type;

extern int _N_;

typedef struct Exp1 {
    int N;
    EXPR sub;
    int isalias;        // only for EXPR_VAR, EXPR_TUPLE, EXPR_INDEX, EXPR_DISC
    union {
        Tk tk;          // EXPR_EXP0
        struct {        // EXPR_TUPLE
            int size;                   // 2
            Tk* vec;                    // (x,y)
        } Tuple;
        struct {        // EXPR_CONS
            Tk subtype;                 // .True
            Tk arg;                     // ()
        } Cons;
        struct {        // EXPR_CALL
            Tk func;                    // f
            Tk arg;                     // x
        } Call;
        struct {        // EXPR_INDEX
            Tk val;                     // x
            Tk index;                   // .3
        } Index;
        struct {        // EXPR_DISC
            Tk val;                     // x
            Tk subtype;                 // .True!
        } Disc;
        struct {        // EXPR_PRED
            Tk val;                     // x
            Tk subtype;                 // .True?
        } Pred;
    };
} Exp1;

typedef struct {
    Tk   id;                            // True:
    Type type;                          // ()
} Sub;

struct Env;

typedef struct Stmt {
    int N;
    STMT sub;
    struct Env* env;        // see env.c
    struct Stmt* seqs[2];   // {outer,inner}
    Tk tk;
    union {
        Tk Return;          // STMT_RETURN
        struct Stmt* Block; // STMT_BLOCK
        struct {            // STMT_VAR
            Tk   id;                    // ns
            Type type;                  // : Nat
            Exp1 init;                  // = n
        } Var;
        struct {           // STMT_USER
            int  isrec;                 // rec
            Tk   id;                    // Bool
            int  size;                  // 2 subs
            Sub* vec;                   // [True,False]
        } User;
        struct {           // STMT_SEQ
            struct Stmt* s1;
            struct Stmt* s2;
        } Seq;
        struct {            // STMT_IF
            Tk cond;                    // if (tst)
            struct Stmt* true;          // { ... }
            struct Stmt* false;         // else { ... }
        } If;
        struct {            // STMT_FUNC
            Tk id;                      // func f
            Type type;                  // : () -> ()
            struct Stmt* body;          // { ... }
        } Func;
    };
} Stmt;

///////////////////////////////////////////////////////////////////////////////

int parser_type   (Type* ret);
int parser_expr   (Stmt** s, Exp1* e);
int parser_stmt   (Stmt** ret);
int parser_stmts  (TK opt, Stmt** ret);
int parser        (Stmt** ret);
