#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "visit.h"
#include "env.h"
#include "code.h"

#define GCC "gcc -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-local-typedefs -Wno-unused-function -Wno-format-zero-length"

//#define TODO(msg) fprintf(stderr, msg)
#define TODO(msg)

FILE* stropen  (const char* mode, size_t size, char* str);

typedef struct {
    FILE  *out, *inp;
    char  err[256];
    long  lin,col;
    Tk    tk0,tk1;
    Env*  env;
    int   nn;
} All;

extern All ALL;

int all_init (FILE* out, FILE* inp);
