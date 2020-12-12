typedef struct Env {
    Stmt* stmt;         // STMT_USER, STMT_VAR
    struct Env* prev;
} Env;

Stmt* env_id_to_stmt (Env* env, const char* id);
Type* env_expr_to_type (Expr* e);
Stmt* env_sub_id_to_user_stmt (Env* env, const char* sub);
Stmt* env_expr_to_type_to_user_stmt (Expr* e);
Stmt* env_stmt_to_func (Stmt* s);
//void env_dump (Env* env);

int env (Stmt* s);
