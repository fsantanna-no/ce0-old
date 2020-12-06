typedef struct Env {
    Stmt* stmt;         // STMT_USER, STMT_VAR
    struct Env* prev;
} Env;

// Returns the statement declaration of given variable or user type identifier.
Stmt* env_find_decl (Env* env, const char* id);  // env_find_decl(e, "x") -> {STMT_VAR, {"x",(),init)}}

// Returns the type of given expression.
Type* env_expr_type (Expr* e);                   // env_expr_type({EXPR_VAR, "x")}) -> ()

// Returs the super user statement declaration given a sub user identifier.
Stmt* env_find_super (Env* env, const char* sub);   // env_find_super(e, "True") -> {STMT_USER, {"Bool",...}}

void env_dump (Env* env);

int env (Stmt* s);
