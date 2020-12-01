#include <stdio.h>
#include <string.h>

#include "all.h"

All ALL;

FILE* stropen (const char* mode, size_t size, char* str) {
    size = (size != 0) ? size : strlen(str);
    return fmemopen(str, size, mode);
}

void all_init (FILE* inp) {
    ALL = (All) {
        inp,
        1, 1,
        {}, { TK_ERR, {}, 1, 1 }
    };
    lexer();
}
