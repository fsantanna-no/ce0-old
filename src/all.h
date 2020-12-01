#include <stdio.h>

#include "lexer.h"
#include "parser.h"

FILE* stropen  (const char* mode, size_t size, char* str);

typedef struct {
    FILE* inp;
    long  lin,col;
    Tk    tk0,tk1;
} All;

extern All ALL;

void all_init (FILE* inp);
