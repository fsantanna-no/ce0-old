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
}

int main (void) {
    t_lexer();
    puts("OK");
}
