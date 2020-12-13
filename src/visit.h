enum {
    VISIT_ERROR = 0,
    VISIT_CONTINUE,     // continue normally
    VISIT_BREAK         // continue but do not nest
};

typedef int (*F_Stmt) (Stmt* s);
typedef int (*F_Expr) (Expr* e);
typedef int (*F_Type) (Type* tp);

int visit_type (Type* tp, F_Type f);
int visit_expr (Expr* e,  F_Expr fe);
int visit_stmt (Stmt* s,  F_Stmt fs, F_Expr fe, F_Type ft);
