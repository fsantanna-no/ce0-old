typedef struct Env {
    Stmt* stmt;         // STMT_USER, STMT_VAR
    struct Env* prev;
} Env;

Stmt* env_get (Env* env, const char* xp);
Type* env_type (Expr* e);
int types (Stmt* s);
