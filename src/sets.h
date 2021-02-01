typedef struct Env {
    Stmt* stmt;         // STMT_USER, STMT_VAR
    struct Env* prev;
    int depth;
} Env;

Stmt* stmt_xmost (Stmt* s, int right);

int sets (Stmt* s);
