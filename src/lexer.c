#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "all.h"

static char* reserved[] = {
    //"arg",
    "call",
    //"clone",
    "else",
    "func",
    "if",
    //"output",
    "rec",
    "return",
    "type",
    "var"
};

static int is_reserved (TK_val* val) {
    int N = sizeof(reserved) / sizeof(reserved[0]);
    for (int i=0; i<N; i++) {
        if (!strcmp(val->s, reserved[i])) {
            return TK_RESERVED+1 + i;
        }
    }
    return 0;
}

const char* lexer_tk2err (TK enu) {
    static char str[512];
    switch (enu) {
        case TK_EOF:
            sprintf(str, "end of file");
            break;
        case TX_VAR:
            sprintf(str, "variable identifier");
            break;
        case TX_USER:
            sprintf(str, "type identifier");
            break;
        case TX_NUM:
            sprintf(str, "tuple index");
            break;
        default:
            if (enu>0 && enu<TK_SINGLE) {
                sprintf(str, "`%c´", enu);
            } else {
//printf("%d\n", enu);
                assert(0 && "TODO");
            }
            break;
    }
    return str;
}

const char* lexer_tk2str (Tk* tk) {
    static char str[512];
    switch (tk->enu) {
        case TK_EOF:
            sprintf(str, "end of file");
            break;
        case TK_UNIT:
            sprintf(str, "`()´");
            break;
        case TX_NATIVE:
            sprintf(str, "\"_%s\"", tk->val.s);
            break;
        case TX_VAR:
        case TX_USER:
        case TK_ERR:
            sprintf(str, "\"%s\"", tk->val.s);
            break;
        default:
            if (tk->enu>0 && tk->enu<TK_SINGLE) {
                sprintf(str, "`%c´", tk->enu);
           } else if (tk->enu > TK_RESERVED) {
                sprintf(str, "`%s´", reserved[tk->enu-TK_RESERVED-1]);
            } else {
                assert(0 && "TODO");
            }
            break;
    }
    return str;
}

static TK lx_token (TK_val* val) {
    int c = fgetc(ALL.inp);
//printf("0> [%c] [%d]\n", c, c);
    switch (c)
    {
        case '{':
        case '}':
        case ')':
        case ';':
        case ':':
        case '=':
        case ',':
        case '.':
        case '&':
        case '$':
        case '!':
        case '?':
            return c;

        case EOF:
            return TK_EOF;

        case '(':
            c = fgetc(ALL.inp);
            if (c == ')') {
                return TK_UNIT;
            }
            ungetc(c, ALL.inp);
            return '(';

        case '-':
            c = fgetc(ALL.inp);
            if (c == '>') {
                return TK_ARROW;
            } else {
                ungetc(c, ALL.inp);
                val->s[0] = '-';
                val->s[1] = '\0';
                return TK_ERR;
            }

        case '_': {
            c = fgetc(ALL.inp);
            char delim = 0;
            if (c=='(' || c=='{') {
                delim = (c == '(' ? ')' : '}');
                c = fgetc(ALL.inp);
            }
            int i = 0;
            while (delim || isalnum(c) || c=='_') {
                if (c == delim) {
                    break;
                }
                val->s[i++] = c;
                c = fgetc(ALL.inp);
            }
            val->s[i] = '\0';
            if (!delim) {
                ungetc(c, ALL.inp);
            }
            return TX_NATIVE;
        }

        default:
            // TX_NUM
            if (isdigit(c)) {
                int i = 0;
                while (isalnum(c)) {
                    if (isdigit(c)) {
                        val->s[i++] = c;
                    } else {
                        val->s[i] = '\0';
                        return TK_ERR;
                    }
                    c = fgetc(ALL.inp);
                }
                val->n = atoi(val->s);
                ungetc(c, ALL.inp);
                return TX_NUM;
            } else if (isalpha(c)) {
                // var,user
            } else {
                val->s[0] = c;
                val->s[1] = '\0';
                return TK_ERR;
            }

            // TK_VAR, TK_TYPE
            int i = 0;
            while (isalnum(c) || c=='_') {
                val->s[i++] = c;
                c = fgetc(ALL.inp);
            }
            val->s[i] = '\0';
            ungetc(c, ALL.inp);

            int key = is_reserved(val);
            if (key) {
                return key;
            }

            if (islower(val->s[0])) {
                return TX_VAR;
            } else if (isupper(val->s[0])) {
                return TX_USER;
            } else {
                // impossible case
            }
    }
    assert(0 && "bug found");
}

static void lx_blanks (void) {
    while (1) {
        int c = fgetc(ALL.inp);
//printf("0> [%c] [%d]\n", c, c);
        switch (c) {
            case '\n':              // ignore new line
                ALL.lin++;
                ALL.col = 1;
                break;
            case ' ':               // ignore space
                ALL.col++;
                break;
            case '-':
                c = fgetc(ALL.inp);
                if (c == '-') {     // ignore comment
                    ALL.col++;
                    ALL.col++;
                    while (1) {
                        c = fgetc(ALL.inp);
                        if (c == EOF) {
                            break;      // EOF stops comment
                        }
                        if (c == '\n') {
                            ungetc(c, ALL.inp);
                            break;      // NEWLINE stops comment
                        }
                        ALL.col++;
                    }
                } else {
                    ungetc(c, ALL.inp);
                    ungetc('-', ALL.inp);
                    return;
                }
                break;
            default:
                ungetc(c, ALL.inp);
                return;
        }
    }
}

void lexer (void) {
    ALL.tk0 = ALL.tk1;
    lx_blanks();
    ALL.tk1.lin = ALL.lin;
    ALL.tk1.col = ALL.col;
    long bef = ftell(ALL.inp);
    ALL.tk1.enu = lx_token(&ALL.tk1.val);
    ALL.col += ftell(ALL.inp) - bef;
    lx_blanks();
}
