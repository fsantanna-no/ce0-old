// returns of F_Stmt, F_Expr, F_Type

enum {
    VISIT_ERROR = 0,
    VISIT_CONTINUE,     // continue normally
    VISIT_BREAK         // continue but do not nest
};

enum {
    EXEC_ERROR = 0,
    EXEC_CONTINUE,     // continue normally
    EXEC_HALT          // stop everything
};

///////////////////////////////////////////////////////////////////////////////

typedef struct Exec_State {
    int size;
    int vec[100];
    int cur;
} Exec_State;

typedef int (*F_Stmt) (Stmt* s);
typedef int (*F_Expr) (Expr* e);
typedef int (*F_Type) (Type* tp);

int visit_type (Type* tp, F_Type f);
int visit_expr (Expr* e,  F_Expr fe);
int visit_stmt (Stmt* s,  F_Stmt fs, F_Expr fe, F_Type ft);

void exec_init (Exec_State* est);

// 1=more, 0=exhausted  //  fret (fs/fe): 0=error, 1=success
int exec (Exec_State* est, Stmt* s, F_Stmt fs, F_Expr fe, int* fret);
