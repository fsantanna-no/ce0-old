typedef enum {
    TK_ERR = 0,

    TK_TOK = 256,   // all single-char tokens

    TK_COMMENT,

    TK_VAR,
    TK_TYPE,

    TK_RESERVED,    // all reserved keywords
    TK_VAL
} TK;

typedef union {
    int  n;
    char s[256];
} TK_val;

typedef struct {
    TK enu;
    TK_val val;
} Tk;

Tk lexer (void);
