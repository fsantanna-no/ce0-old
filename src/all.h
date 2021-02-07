#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "visit.h"
#include "exec.h"
#include "sets.h"
#include "env.h"
#include "check.h"
#include "code.h"
#include "dump.h"

#define GCC "gcc -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-local-typedefs -Wno-unused-function -Wno-format-zero-length"

//#define TODO(msg) fprintf(stderr, msg)
#define TODO(msg)

FILE* stropen  (const char* mode, size_t size, char* str);

typedef struct {
    FILE  *out, *inp;
    char  err[256];
    int   lin,col;
    Tk    tk0,tk1;
    Env*  env;
    int   nn;
} All;

extern All ALL;

int all_init (FILE* out, FILE* inp);
