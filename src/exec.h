enum {
    EXEC_ERROR = 0,
    EXEC_CONTINUE,      // continue normally
    EXEC_BREAK,         // stop current stmt/expr
    EXEC_HALT           // stop everything
};

typedef struct Exec_State {
    int size;
    int vec[100];
    int cur;
} Exec_State;

int exec (Stmt* s, F_Pre pre, F_Stmt fs, F_Expr fe);   // 0=error, 1=success
