#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "all.h"

static char* reserved[] = {
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

static TK lx_token (TK_val* val) {
    int c = fgetc(ALL.inp);
//printf("0> [%c] [%d]\n", c, c);
    switch (c)
    {
        case '{':
        case '}':
        case ':':
        case EOF:
            return c;

        case '-':
            c = fgetc(ALL.inp);
            if (c != '-') {
                return TK_ERR;
            }
            while (1) {
                c = fgetc(ALL.inp);
                if (c == EOF) {
                    break;      // EOF stops comment
                }
                if (c == '\n') {
                    ungetc(c, ALL.inp);
                    break;      // NEWLINE stops comment
                }
            }
            return TK_COMMENT;

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
    // ignore blanks (spaces and lines)
    while (1) {
        int c = fgetc(ALL.inp);
        if (c == ' ' || c == '\n') {
            // continue ignoring
        } else {
            ungetc(c, ALL.inp);
            break;
        }
    }
}

Tk lexer (void) {
    Tk ret;
    lx_blanks();
    ret.enu = lx_token(&ret.val);
    lx_blanks();
    //printf("? %d %c\n", ret.enu, ret.enu);
    //printf(": n=%d %d %c\n", ret.val.n, ret.sym, ret.sym);
    return ret;
}
