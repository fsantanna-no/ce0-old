int check_dcls (Stmt* s);
    // "invalid return : no enclosing function"
    // "undeclared subtype \"%s\""
    // "undeclared subtype \"%s\""

int check_types (Stmt* s);
    // "invalid `\\´ : unexpected pointer type"
    // "invalid `\\´ : expected pointer type"
    // "invalid `.´ : expected tuple type"
    // "invalid `.´ : index is out of range"
    // "invalid `.´ : expected user type"
    // "invalid `.´ : expected user type"
    // "invalid call to \"%s\" : type mismatch"
    // "invalid constructor \"%s\"
    // "invalid assignment to \"%s\" : type mismatch"
    // "invalid return : type mismatch"
    // "invalid assignment to \"%s\" : type mismatch"
    // "invalid assignment : type mismatch"
    // "invalid condition : type mismatch"

int check_ptrs (Stmt* s);
    // "invalid return : cannot return local pointer \"%s\" (ln %d)"
    // "invalid assignment : cannot hold pointer \"%s\" (ln %d) in outer scope"
    // "invalid tuple : pointers must be ordered from outer to deep scopes"

typedef void (*F_txed_vars) (Expr* e_, Expr* e);
void txs_txed_vars (Env* env, Expr* e, F_txed_vars f);
int check_txs (Stmt* s);
    // "invalid ownership transfer : expected `move´ before expression"
    // "invalid ownership transfer : mode `growable´ only allows root transfers"
    // "invalid dnref : cannot transfer value"
    // "unexpected `move´ call : no ownership transfer"

int check_owner (Stmt* s);
    // "invalid access to \"%s\" : ownership was transferred (ln %d)"
    // "invalid transfer of \"%s\" : active pointer in scope (ln %d)"
    // "invalid assignment : cannot transfer ownsership to itself"
