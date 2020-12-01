typedef enum {
    TK_ERR = 0,

    TK_SINGLE = 256,   // all single-char tokens

    TK_EOF,

    TK_VAR,
    TK_TYPE,

    TK_RESERVED,    // all reserved keywords
    TK_CALL,
    TK_VAL
} TK;

typedef union {
    int  n;
    char s[256];
} TK_val;

typedef struct {
    TK enu;
    TK_val val;
    long  lin;   // line at token
    long  col;   // column at token
} Tk;

const char* lexer_tk2str (Tk* tk);
void lexer (void);
