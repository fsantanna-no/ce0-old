#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../all.h"

int all (const char* xp, char* src) {
    static char out[65000];
    all_init (
        stropen("w", sizeof(out), out),
        stropen("r", 0, src)
    );
    Stmt s;
    if (!parser_prog(&s)) {
        puts(ALL.err);
        return !strcmp(ALL.err, xp);
    }
    code(s);
    fclose(ALL.out);
#if 0
puts(">>>");
puts(out);
puts("<<<");
#endif
    remove("a.out");

    // compile
    {
        FILE* f = popen("gcc -xc -", "w");
        assert(f != NULL);
        fputs(out, f);
        fclose(f);
    }

    // execute
    {
        FILE* f = popen("./a.out", "r");
        assert(f != NULL);
        char* cur = out;
        int n = sizeof(out) - 1;
        while (1) {
            char* ret = fgets(cur,n,f);
            if (ret == NULL) {
                break;
            }
            n -= strlen(ret);
            cur += strlen(ret);
        }
    }
#if 0
puts(">>>");
puts(out);
puts("---");
puts(xp);
puts("<<<");
#endif
    return !strcmp(out,xp);
}

void prelude (void) {
    assert(all(
        "()\n",
        "type Nat {\n"
        "   Succ: Nat\n"
        "}\n"
    ));
}

void chap_01 (void) {
    assert(all(
        "()\n",
        "call _show_Unit()"
    ));
}

int main (void) {
    prelude();
    chap_01();
    puts("OK");
}
