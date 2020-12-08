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
    "\n"
#if 0
    "func not: Bool -> Bool {\n"
    "    if arg.1.True? {\n"
    "        return False\n"
    "    } else {\n"
    "        return True\n"
    "    }\n"
    "}\n"
    "\n"
    "func and: (Bool,Bool) -> Bool {\n"
    "    if arg.1.False? {\n"
    "        return False\n"
    "    } else {\n"
    "        if arg.2.False? {\n"
    "            return False\n"
    "        } else {\n"
    "            return True\n"
    "        }\n"
    "    }\n"
    "}\n"
    "\n"
    "func or: (Bool,Bool) -> Bool {\n"
    "    if arg.1.True? {\n"
    "        return True\n"
    "    } else { if arg.2.True? {\n"
    "        if arg.2.True? {\n"
    "            return True\n"
    "        } else {\n"
    "            return False\n"
    "        }\n"
    "    }\n"
    "}\n"
#endif
;

const char _nat[] =
    "type rec Nat {\n"
    "   Succ: Nat\n"
    "}\n"
    "\n"
#if 0
    "func eq: (Nat,Nat) -> Bool {\n"
    "    val x: Nat = arg.1\n"
    "    val y: Nat = arg.2\n"
    "    if and(x.Nil?,y.Nil?) {\n"
    "        return True\n"
    "    } else {\n"
    "        if or(x.Nil?,y.Nil?) {\n"
    "            return False\n"
    "        } else {\n"
    "            return eq(x.Succ!, y.Succ!)\n"
    "        }\n"
    "    }\n"
    "}\n"
    "\n"
#endif
    "func lt: (Nat,Nat) -> Bool {\n"
    "    val x: Nat = arg.1\n"
    "    val y: Nat = arg.2\n"
    "    if y.Nil? {\n"
    "        return False\n"
    "    } else {\n"
    "        if x.Nil? {\n"
    "            return True\n"
    "        } else {\n"
    "            return lt(x.Succ!, y.Succ!)\n"
    "        }\n"
    "    }\n"
    "}\n"
    "\n"
#if 0
    "func lte: (Nat,Nat) -> Bool {\n"
    "    val x: Nat = arg.1\n"
    "    val y: Nat = arg.2\n"
    "    return or(lt(x,y), eq(x,y))\n"
    "}\n"
    "\n"
#endif
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
    "func rem: (Nat,Nat) -> Nat {\n"
    "    val x: Nat = arg.1\n"
    "    val y: Nat = arg.2\n"
    "    if lt(x,y) {\n"
    "        return x\n"
    "    } else {\n"
    "        return rem(sub(x,y),y)\n"
    "    }\n"
    "}\n"
    "\n"
    "val one:   Nat = Succ(Nil)\n"
    "val two:   Nat = Succ(one)\n"
    "val three: Nat = Succ(two)\n"
    "val four:  Nat = Succ(three)\n"
    "val five:  Nat = Succ(four)\n"
    "val six:   Nat = Succ(five)\n"
    "val seven: Nat = Succ(six)\n"
    "val eight: Nat = Succ(seven)\n"
    "val nine:  Nat = Succ(eight)\n"
    "val ten:   Nat = Succ(nine)\n"
;

void chap_pre (void) {
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
        "call output(lt(Nil, Succ(Nil)))\n"
    );
    assert(all("True\n", INP));

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

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "val n[]: Nat = rem(two,three)\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Nil))\n", INP));
}

void chap_01 (void) {           // pg 1

    // 1.1                      // pg 1

    strcpy(INP, _bool);         // pg 1
    strcat(INP, _nat);
    strcat (INP,
        "func square: Nat -> Nat {\n"
        "    return mul(arg,arg)\n"
        "}\n"
        "val n[]: Nat = square(two)\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Nil))))\n", INP));

    strcpy(INP, _bool);         // pg 2
    strcat(INP, _nat);
    strcat (INP,
        "func smaller: (Nat,Nat) -> Nat {\n"
        "    val x: Nat = arg.1\n"
        "    val y: Nat = arg.2\n"
        "    if lt(x,y) {\n"
        "        return x\n"
        "    } else {\n"
        "        return y\n"
        "    }\n"
        "}\n"
        "val n[]: Nat = add (smaller(ten,five), smaller(one,four))\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ (Succ (Nil))))))\n", INP));

    strcpy(INP, _bool);         // pg 3
    strcat(INP, _nat);
    strcat (INP,
        "func square: Nat -> Nat {\n"
        "    return mul(arg,arg)\n"
        "}\n"
        "func smaller: (Nat,Nat) -> Nat {\n"
        "    val x: Nat = arg.1\n"
        "    val y: Nat = arg.2\n"
        "    if lt(x,y) {\n"
        "        return x\n"
        "    } else {\n"
        "        return y\n"
        "    }\n"
        "}\n"
        "val n[]: Nat = square (smaller(four,two))\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Nil))))\n", INP));

    // TODO-delta               // pg 3

    // 1.2                      // pg 1

    strcpy(INP, _bool);         // pg 3
    strcat(INP, _nat);
    strcat (INP,
        "func fthree: () -> Nat {\n"
        "    return tree\n"
        "}\n"
        "call output(fthree())\n"
    );
    assert(all("Succ (Succ (Succ (Nil)))\n", INP));

}

int main (void) {
    chap_pre();
    chap_01();
    puts("OK");
}
