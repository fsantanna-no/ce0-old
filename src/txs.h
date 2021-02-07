typedef void (*F_txed_vars) (Expr* e_, Expr* e);
void txs_txed_vars (Env* env, Expr* e, F_txed_vars f);
int txs (Stmt* s);
    // "invalid ownership transfer : expected `move´ before expression"
    // "invalid ownership transfer : mode `growable´ only allows root transfers"
    // "invalid dnref : cannot transfer value"
    // "unexpected `move´ call : no ownership transfer"
