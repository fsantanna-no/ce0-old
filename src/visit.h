enum {
    VISIT_ERROR = 0,
    VISIT_CONTINUE,     // continue normally
    VISIT_BREAK         // continue but do not nest
};

struct Env;

typedef int  (*F_Stmt) (Stmt* s);
typedef int  (*F_Type) (struct Env* env, Type* tp);
typedef int  (*F_Expr) (struct Env* env, Expr* e);
typedef void (*F_Pre)  (void);

int visit_type (struct Env* env, Type* tp, F_Type ft);
int visit_expr (int ord, struct Env* env, Expr* e,  F_Expr fe);
int visit_stmt (int ord, Stmt* s, F_Stmt fs, F_Expr fe, F_Type ft);
