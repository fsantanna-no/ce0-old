// returns of F_Stmt, F_Expr, F_Type

enum {
    VISIT_ERROR = 0,
    VISIT_CONTINUE,     // continue normally
    VISIT_BREAK         // continue but do not nest
};

enum {
    EXEC_ERROR = 0,
    EXEC_CONTINUE,      // continue normally
    EXEC_BREAK,         // stop current stmt/expr
    EXEC_HALT           // stop everything
};

///////////////////////////////////////////////////////////////////////////////

typedef struct Exec_State {
    int size;
    int vec[100];
    int cur;
} Exec_State;

typedef int  (*F_Stmt) (Stmt* s);
typedef int  (*F_Type) (Type* tp, void* arg);
typedef void (*F_Pre)  (void);

int visit_type (Type* tp, F_Type f, void* arg);
int visit_stmt (Stmt* s, F_Stmt fs);

int exec (Stmt* s, F_Pre pre, F_Stmt fs);   // 0=error, 1=success
