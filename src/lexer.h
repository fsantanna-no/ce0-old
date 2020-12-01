typedef enum {
    TK_ERR = 0,

    // all single-char tokens
    TK_TOK = 256,
    TK_VAR,
    TK_TYPE,

    TK_COMMENT,
} TK_enu;

typedef union {
    int  n;
    char s[256];
} TK_val;

typedef struct {
    TK_enu enu;
    TK_val val;
} TK_all;

TK_all lexer (void);
