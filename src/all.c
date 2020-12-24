#include <stdio.h>
#include <string.h>

#include "all.h"

All ALL;

FILE* stropen (const char* mode, size_t size, char* str) {
    size = (size != 0) ? size : strlen(str);
    return fmemopen(str, size, mode);
}

int all_init (FILE* out, FILE* inp) {
    ALL = (All) {
        out, inp,
        {},                         // err
        1, 1,                       // lin, col
        {}, { TK_ERR, {}, 1, 1 },   // tk0, tk1
        NULL,                       // env
        0                           // n
    };
    if (inp != NULL) {
        lexer();
    }
    return 1;
}
