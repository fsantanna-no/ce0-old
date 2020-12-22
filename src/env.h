typedef struct Env {
    Stmt* stmt;         // STMT_USER, STMT_VAR
    struct Env* prev;
} Env;

Stmt* env_id_to_stmt   (Env* env, const char* id, int* scope);
Type* env_tk_to_type   (Env* env, Tk* tk);
Type* env_expr_to_type (Env* env, Exp1* e);
Stmt* env_sub_id_to_user_stmt (Env* env, const char* sub);
Stmt* env_tk_to_type_to_user_stmt (Env* env, Tk* tk);
Stmt* env_stmt_to_func (Stmt* s);
int   env_type_isrec   (Env* env, Type* tp);
int   env_type_hasalloc (Env* env, Type* tp);
//void env_dump (Env* env);

int env (Stmt* s);
