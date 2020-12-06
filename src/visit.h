typedef int (*f_stmt) (Stmt* s);
typedef int (*f_expr) (Expr* e);
typedef int (*f_type) (Type* tp);

void visit_type (Type* tp, f_type f);
void visit_expr (Expr* e,  f_expr fe);
void visit_stmt (Stmt* s,  f_stmt fs, f_expr fe, f_type ft);
