#include <stdio.h>
#include <string.h>

#include "all.h"

All ALL;

FILE* stropen (const char* mode, size_t size, char* str) {
    size = (size != 0) ? size : strlen(str);
    return fmemopen(str, size, mode);
}

void all_init (FILE* out, FILE* inp) {
    ALL = (All) {
        out, inp,
        {},                         // err
        1, 1,                       // lin, col
        {}, { TK_ERR, {}, 1, 1 }    // tk0, tk1
    };
    if (inp != NULL) {
        lexer();
    }
}
