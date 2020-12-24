#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "../all.h"

enum {
    MODE_CODE,
    MODE_EXE
} MODE;

int main (int argc, char* argv[]) {
    char* inp = NULL;
    char* out = "ce0.out";
    int mode = MODE_EXE;

    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-o")) {
            out = argv[++i];
        } else if (!strcmp(argv[i], "-c")) {
            mode = MODE_CODE;
        } else {
            inp = argv[i];
        }
    }
    assert(inp != NULL);
    assert(strlen(out) < 64);

    FILE* finp = fopen(inp, "r");
    FILE* fout; {
        if (mode == MODE_CODE) {
            fout = fopen(out, "w");
        } else {
            char gcc[256];
            sprintf(gcc, GCC " -o %s -xc -", out);
            fout = popen(gcc, "w");
        }
    }
    assert(finp!=NULL && fout!=NULL);

    Stmt* s;

    if (!all_init(fout, finp)) {
        fprintf(stderr, "%s\n", ALL.err);
        exit(EXIT_FAILURE);
    }

    if (!parser(&s)) {
        fprintf(stderr, "%s\n", ALL.err);
        fputs("int main (void) {}", fout);
        exit(EXIT_FAILURE);
    }

    if (!env(s)) {
        fprintf(stderr, "%s\n", ALL.err);
        fputs("int main (void) {}", fout);
        exit(EXIT_FAILURE);
    }
    code(s);

    return 0;
}
