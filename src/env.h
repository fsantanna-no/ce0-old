typedef struct Env {
    Stmt* stmt;         // STMT_USER, STMT_VAR
    struct Env* prev;
    int depth;
} Env;

int type_is_sup_sub (Type* sup, Type* sub, int isset);

Stmt* env_id_to_stmt    (Env* env, const char* id);
Type* env_tk_to_type    (Env* env, Tk* tk);
Type* env_expr_to_type  (Env* env, Expr* e);
Stmt* env_stmt_to_func  (Stmt* s);
int   env_type_isrec    (Env* env, Type* tp);
int   env_type_hasrec   (Env* env, Type* tp);
int   env_type_ishasrec (Env* env, Type* tp);
int   env_type_ishasptr (Env* env, Type* tp);
Stmt* env_sub_id_to_user_stmt (Env* env, const char* sub);
Type* env_sub_id_to_user_type (Env* env, const char* sub);
Stmt* env_tk_to_type_to_user_stmt (Env* env, Tk* tk);
Stmt* env_expr_leftmost_decl (Env* env, Expr* e);
void  env_held_vars     (Env* env, Expr* e, int* vars_n, Expr** vars, int* uprefs);

int env (Stmt* s);
