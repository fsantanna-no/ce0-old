#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "all.h"

static char* reserved[] = {
    "call",
    "val"
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

const char* lexer_tk2str (Tk* tk) {
    static char str[512];
    switch (tk->enu) {
        case TK_EOF:
            sprintf(str, "end of file");
            break;
        default:
            if (tk->enu>0 && tk->enu<TK_SINGLE) {
                sprintf(str, "`%c´", tk->enu);
            } else {
//printf("%d\n", tk->enu);
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
        case '(':
        case ')':
        case '{':
        case '}':
        case ',':
        case ':':
            return c;

        case EOF:
            return TK_EOF;

        default:
            if (!isalpha(c)) {
                val->n = c;
                return TK_ERR;
            }

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

            return (islower(val->s[0]) ? TK_VAR : TK_TYPE);
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
                    ungetc(c, ALL.inp);
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
    //printf("? %d %c\n", ret.enu, ret.enu);
    //printf(": n=%d %d %c\n", ret.val.n, ret.sym, ret.sym);
}
