typedef enum {
    TK_ERR = 0,

    TK_SINGLE = 256,   // all single-char tokens

    TK_EOF,
    TK_ARROW,

    TX_NATIVE,
    TX_VAR,
    TX_NIL,
    TX_USER,
    TX_NUM,

    TK_RESERVED,    // all reserved keywords
    TK_ARG,
    TK_CALL,
    TK_ELSE,
    TK_FUNC,
    TK_IF,
    TK_OUTPUT,
    TK_REC,
    TK_RETURN,
    TK_TYPE,
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
const char* lexer_tk2err (TK enu);
int lexer (void);
