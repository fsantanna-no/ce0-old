typedef enum {
    TYPE_ANY = 0,
    TYPE_UNIT,
    TYPE_NATIVE,
    TYPE_USER,
    TYPE_TUPLE,
    TYPE_FUNC
} TYPE;

typedef enum {
    EXPR_UNIT = 1,
    EXPR_UNK,
    EXPR_NATIVE,
    EXPR_NULL,
    EXPR_INT,
    EXPR_VAR,
    EXPR_UPREF,
    EXPR_DNREF,
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
    STMT_LOOP,
    STMT_BREAK,
    STMT_FUNC,
    STMT_RETURN,
    STMT_BLOCK,
    STMT_NATIVE
} STMT;

///////////////////////////////////////////////////////////////////////////////

typedef struct Type {
    TYPE sub;
    int isptr;
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

typedef enum {
    NO=0, TX, BW
} TXBW;

typedef struct Expr {
    int N;
    EXPR sub;
    int istx;   // set to null on tx to avoid double free
    union {
        Tk Unit;        // EXPR_UNIT
        Tk Unk;         // EXPR_UNK
        Tk Native;      // EXPR_NATIVE
        Tk Null;        // EXPR_NULL
        Tk Int;         // EXPR_INT
        struct Expr* Upref;  // EXPR_UPREF
        struct Expr* Dnref;  // EXPR_DNREF
        struct {        // EXPR_VAR
            Tk tk;
            TXBW txbw;  // evaluate var ownership accesses
        } Var;
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
    Tk    tk;                           // True:
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
        struct Stmt* Loop;  // STMT_LOOP
        //void  Break;      // STMT_BREAK
        struct Stmt* Block; // STMT_BLOCK
        struct {            // STMT_VAR
            Tk    tk;                    // ns
            Type* type;                  // : Nat
            Expr* init;                  // = n
        } Var;
        struct {            // STMT_USER
            int  isrec;                 // rec
            Tk   tk;                    // Bool
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
            Expr* tst;                  // if (tst)
            struct Stmt* true;          // { ... }
            struct Stmt* false;         // else { ... }
        } If;
        struct {            // STMT_FUNC
            Tk tk;                      // func f
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

Expr* expr_leftmost (Expr* e);

int parser_type   (Type** ret);
int parser_expr   (Expr** ret, int canpre);
int parser_stmt   (Stmt** ret);
int parser_stmts  (TK opt, Stmt** ret);
int parser        (Stmt** ret);
