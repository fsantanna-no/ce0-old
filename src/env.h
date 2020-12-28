typedef struct Env {
    Stmt* stmt;         // STMT_USER, STMT_VAR
    struct Env* prev;
} Env;

Stmt* env_id_to_stmt    (Env* env, const char* id, int* scope);
Type* env_tk_to_type    (Env* env, Tk* tk);
Type* env_expr_to_type  (Env* env, Expr* e);
Stmt* env_stmt_to_func  (Stmt* s);
int   env_type_isrec    (Env* env, Type* tp);
int   env_type_hasalloc (Env* env, Type* tp);
int   env_type_isptr    (Env* env, Type* tp);
Stmt* env_sub_id_to_user_stmt (Env* env, const char* sub);
Type* env_sub_id_to_user_type (Env* env, const char* sub);
Stmt* env_tk_to_type_to_user_stmt (Env* env, Tk* tk);

int env (Stmt* s);
