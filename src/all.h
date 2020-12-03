#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "code.h"

FILE* stropen  (const char* mode, size_t size, char* str);

typedef struct {
    FILE  *out, *inp;
    char  err[256];
    long  lin,col;
    Tk    tk0,tk1;
} All;

extern All ALL;

void all_init (FILE* out, FILE* inp);
