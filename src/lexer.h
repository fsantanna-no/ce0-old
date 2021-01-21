typedef enum {
    TK_ERR = 0,

    TK_SINGLE = 256,   // all single-char tokens

    TK_EOF,
    TK_ARROW,
    TK_UNIT,

    TX_NATIVE,
    TX_VAR,
    TX_NULL,
    TX_USER,
    TX_NUM,

    TK_RESERVED,    // all reserved keywords
    //TK_ARG,
    TK_BREAK,
    TK_CALL,
    //TK_CLONE,
    TK_ELSE,
    TK_FUNC,
    TK_IF,
    TK_INPUT,
    TK_LOOP,
    TK_NATIVE,
    TK_OUTPUT,
    TK_PRE,
    TK_REC,
    TK_RETURN,
    TK_SET,
    TK_TYPE,
    TK_VAR
} TK;

#define TK_BUF 1024

typedef union {
    int  n;
    char s[TK_BUF];
} TK_val;

struct Env;

typedef struct {
    TK enu;
    TK_val val;
    long  lin;   // line at token
    long  col;   // column at token
} Tk;

int err_message (Tk* tk, const char* v);
const char* lexer_tk2str (Tk* tk);
const char* lexer_tk2err (TK enu);
void lexer (void);
