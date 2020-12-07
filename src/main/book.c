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
    if (!parser(&s)) {
        puts(ALL.err);
        return 0;
    }
    if (!env(&s)) {
        puts(ALL.err);
        return 0;
    }
    code(&s);
    fclose(ALL.out);
#if 0
puts(">>>");
puts(out);
puts("<<<");
#endif
    remove("a.out");

    // compile
    {
        FILE* f = popen(GCC " -xc -", "w");
        assert(f != NULL);
        fputs(out, f);
        fclose(f);
    }

    // execute
    {
        FILE* f = popen("./a.out", "r");
        assert(f != NULL);
        char* cur = out;
        out[0] = '\0';
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
puts(out);
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

const char _bool[] =
    "type Bool {\n"
    "    False: ()\n"
    "    True:  ()\n"
    "}\n"
;

const char _nat[] =
    "type rec Nat {\n"
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
    "\n"
    "func sub: (Nat,Nat) -> Nat {\n"
    "    val x: Nat = arg.1\n"
    "    val y: Nat = arg.2\n"
    "    if y.Nil? {\n"
    "        return x\n"
    "    } else {\n"
    "        return sub(x.Succ!,y.Succ!)\n"
    "    }\n"
    "}\n"
    "\n"
    "func mul: (Nat,Nat) -> Nat {\n"
    "    val x: Nat = arg.1\n"
    "    val y: Nat = arg.2\n"
    "    if y.Nil? {\n"
    "        return Nil\n"
    "    } else {\n"
    "        return add(mul(x,y.Succ!),x)\n"
    "    }\n"
    "}\n"
    "\n"
    "val one:   Nat = Succ(Nil)\n"
    "val two:   Nat = Succ(one)\n"
    "val three: Nat = Succ(two)\n"
    "val four:  Nat = Succ(three)\n"
    "val five:  Nat = Succ(four)\n"
    "val six:   Nat = Succ(five)\n"
;

void chap_01 (void) {
    strcpy (INP,
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "val n: (Nat,Nat) = (Nil, Succ(Nil))\n"
        "call output(n)\n"
    );
    assert(all("(Nil,Succ (Nil))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "val n[]: Nat = add (Nil, Succ(Nil))\n"
        "call output(n)\n"
    );
    assert(all("Succ (Nil)\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "val n[]: Nat = Succ(add(Nil, Succ(Nil)))\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Nil))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "val n[]: Nat = sub(Succ(Nil), Nil)\n"
        "call output(n)\n"
    );
    assert(all("Succ (Nil)\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "val n[]: Nat = mul(two,three)\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ (Succ (Nil))))))\n", INP));
}

int main (void) {
    chap_01();
    puts("OK");
}
