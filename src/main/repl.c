#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "../all.h"

int main (int argc, char* argv[]) {
    char lines[256][256];
    int N = 0;
    while (1) {
        assert(N < 250);

        // read line
        printf("> ");
        char line[200];
        char* ret = fgets(line, 200, stdin);
        assert(ret == line);

        // check if expr or stmt
        int isexp = 0;
        {
            all_init(NULL, stropen("r", 0, line));
            Expr e;
            Stmt s;
            if (parser_expr(&e)) {
                isexp = 1;
                line[strlen(line)-1] = '\0';
                sprintf(lines[N], "call output(%s)\n", line);
            } else if (parser_stmt(&s)) {
                strcpy(lines[N], line);
            } else {
                puts(ALL.err);
                fclose(ALL.inp);
                continue;
            }
            fclose(ALL.inp);
            N++;
        }

        // execute everything

        char INP[65536] = "";
        char OUT[65536];
        for (int i=0; i<N; i++) {
            strcat(INP, lines[i]);
        }
        if (isexp) {
            N--;
        }
//puts(INP);

        all_init (
            stropen("w", sizeof(OUT), OUT),
            stropen("r", 0, INP)
        );
        Stmt s;
        if (!parser(&s)) {
            puts(ALL.err);
        }
        if (!env(&s)) {
            puts(ALL.err);
        }
        code(&s);
        fclose(ALL.out);
//puts(OUT);
        remove("a.out");

        // compile
        {
            FILE* f = popen("gcc -Wall -Wno-unused-variable -Wno-unused-function -Wno-format-zero-length -xc -", "w");
            assert(f != NULL);
            fputs(OUT, f);
            fclose(f);
        }

        // execute
        system("./a.out");
    }

    return 0;
}
