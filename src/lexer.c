#include <assert.h>
#include <stdio.h>

#include "all.h"

TK_enu lx_token (TK_val* val) {
    int c = fgetc(ALL.inp);
//printf("0> [%c] [%d]\n", c, c);
    switch (c)
    {
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
    }
    assert(0 && "bug found");
}

void lx_blanks (void) {
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

TK_all lexer (void) {
    TK_all ret;
    lx_blanks();
    ret.enu = lx_token(&ret.val);
    lx_blanks();
    //printf("? %d %c\n", ret.enu, ret.enu);
    //printf(": n=%d %d %c\n", ret.val.n, ret.sym, ret.sym);
    return ret;
}
