#include <assert.h>
#include <stdio.h>
#include <string.h>

FILE* stropen  (const char* mode, size_t size, char* str);

FILE* stropen (const char* mode, size_t size, char* str) {
    size = (size != 0) ? size : strlen(str);
    return fmemopen(str, size, mode);
}

#include "../all.h"

void t_lexer (void) {
    {
        ALL.inp = stropen("r", 0, "-- foobar");
        assert(lexer().enu == TK_COMMENT);
        assert(lexer().enu == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "-- c1\n--c2\n\n");
        assert(lexer().enu == TK_COMMENT);
        assert(lexer().enu == TK_COMMENT);
        assert(lexer().enu == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "c1\nc2 c3  \n    \nc4");
        TK_all tk1 = lexer(); assert(tk1.enu == TK_VAR); assert(!strcmp(tk1.val.s, "c1"));
        TK_all tk3 = lexer(); assert(tk3.enu == TK_VAR); assert(!strcmp(tk3.val.s, "c2"));
        TK_all tk4 = lexer(); assert(tk4.enu == TK_VAR); assert(!strcmp(tk4.val.s, "c3"));
        TK_all tk5 = lexer(); assert(tk5.enu == TK_VAR); assert(!strcmp(tk5.val.s, "c4"));
        assert(lexer().enu == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "c1 C1 Ca a C");
        TK_all tk1 = lexer(); assert(tk1.enu == TK_VAR);  assert(!strcmp(tk1.val.s, "c1"));
        TK_all tk2 = lexer(); assert(tk2.enu == TK_TYPE); assert(!strcmp(tk2.val.s, "C1"));
        TK_all tk3 = lexer(); assert(tk3.enu == TK_TYPE); assert(!strcmp(tk3.val.s, "Ca"));
        TK_all tk4 = lexer(); assert(tk4.enu == TK_VAR);  assert(!strcmp(tk4.val.s, "a"));
        TK_all tk5 = lexer(); assert(tk5.enu == TK_TYPE); assert(!strcmp(tk5.val.s, "C"));
        assert(lexer().enu == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "val xval valx");
        assert(lexer().enu == TK_VAL);
        TK_all tk1 = lexer(); assert(tk1.enu == TK_VAR); assert(!strcmp(tk1.val.s, "xval"));
        TK_all tk2 = lexer(); assert(tk2.enu == TK_VAR); assert(!strcmp(tk2.val.s, "valx"));
        assert(lexer().enu == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, ": }{ :");
        assert(lexer().enu == ':');
        assert(lexer().enu == '}');
        assert(lexer().enu == '{');
        assert(lexer().enu == ':');
        fclose(ALL.inp);
    }
}

int main (void) {
    t_lexer();
    puts("OK");
}
