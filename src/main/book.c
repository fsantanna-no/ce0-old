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
    assert(parser(&s));
    assert(env(&s));
    code(&s);
    fclose(ALL.out);
#if 1
puts(">>>");
puts(out);
puts("<<<");
#endif
    remove("a.out");

    // compile
    {
        FILE* f = popen("gcc -Wall -Wno-unused-function -Wno-format-zero-length -xc -", "w");
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

static char INP[8192];

const char _nat[] =
    "type Nat {\n"
    "   Succ: Nat\n"
    "}\n"
    "\n"
    "func add: (Nat,Nat) -> Nat {\n"
    "    val x: Nat = arg.1\n"
    "    val y: Nat = arg.2\n"
    "    if y.Nil? {\n"
    "        return x\n"
    "    } else {\n"
    "        return Succ(add(x,y.Succ!))\n"
    "    }\n"
    "}\n"
;

void chap_01 (void) {
    strcpy(INP, _nat);
    strcat (INP,
        "call _show_Unit()"
    );
    assert(all("()\n", INP));
}

int main (void) {
    chap_01();
    puts("OK");
}
