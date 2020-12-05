typedef struct Env {
    Stmt* stmt;         // STMT_USER, STMT_VAR
    struct Env* prev;
} Env;

// Returns the declaration of given variable or user type identifier.
Stmt* env_stmt (Env* env, const char* xp);  // env_stmt(e, "x") -> {STMT_VAR, .Var={"x",(),init)}}

// Returns the type of given expression.
Type* env_type (Expr* e);                   // env_type({EXPR_VAR, "x")}) -> ()

//

int types (Stmt* s);
