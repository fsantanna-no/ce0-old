typedef void (*F_txed_vars) (Expr* e_, Expr* e);
void txs_txed_vars (Env* env, Expr* e, F_txed_vars f);
int txs (Stmt* s);
