#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../all.h"

int all (const char* xp, char* src) {
    static char out[65000];
    Stmt s;
    if (!all_init (
        stropen("w", sizeof(out), out),
        stropen("r", 0, src)
    )) {
        puts(ALL.err);
        return 0;
    };
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
#if 1
puts(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
puts(out);
puts("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
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
    "\n"                                        // 5
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
    "\n"                                        // 9
#if 0
    "func eq: (Nat,Nat) -> Bool {\n"
    "    var x: Nat = arg.1\n"
    "    var y: Nat = arg.2\n"
    "    if and(x.$Nat?,y.$Nat?) {\n"
    "        return True\n"
    "    } else {\n"
    "        if or(x.$Nat?,y.$Nat?) {\n"
    "            return False\n"
    "        } else {\n"
    "            return eq(x.Succ!, y.Succ!)\n"
    "        }\n"
    "    }\n"
    "}\n"
    "\n"
#endif
    "func lt: (Nat&,Nat&) -> Bool {\n"
    "    var x: Nat& = arg.1\n"
    "    var y: Nat& = arg.2\n"
    "    if y.$Nat? {\n"
    "        return False\n"
    "    } else {\n"
    "        if x.$Nat? {\n"
    "            return True\n"
    "        } else {\n"
    "            return lt(x.Succ!, y.Succ!)\n"
    "        }\n"
    "    }\n"
    "}\n"
    "\n"                                        // 23
#if 0
    "func lte: (Nat,Nat) -> Bool {\n"
    "    var x: Nat = arg.1\n"
    "    var y: Nat = arg.2\n"
    "    return or(lt(x,y), eq(x,y))\n"
    "}\n"
    "\n"
#endif
    "func add: (Nat,Nat&) -> Nat {\n"
    "    var x: Nat  = arg.1\n"
    "    var y: Nat& = arg.2\n"
    "    if y.$Nat? {\n"
    "        return x\n"
    "    } else {\n"
    "        return Succ(add(x,y.Succ!))\n"     // 30
    "    }\n"
    "}\n"
    "\n"
    "func sub: (Nat,Nat) -> Nat {\n"
    "    var x: Nat = arg.1\n"
    "    var y: Nat = arg.2\n"
    "    if y.$Nat? {\n"
    "        return x\n"
    "    } else {\n"
    "        return sub(x.Succ!,y.Succ!)\n"     // 40
    "    }\n"
    "}\n"
    "\n"
    "func mul: (Nat&,Nat&) -> Nat {\n"
    "    var x: Nat& = arg.1\n"
    "    var y: Nat& = arg.2\n"
    "    if y.$Nat? {\n"
    "        return $Nat\n"
    "    } else {\n"
    "        var z: Nat = mul(x,y.Succ!)\n"    // 50
    "        return add(z,x)\n"                         // x ja foi consumido: dar erro
    "    }\n"
    "}\n"
    "\n"
    "func rem: (Nat,Nat) -> Nat {\n"
    "    var x: Nat = arg.1\n"
    "    var y: Nat = arg.2\n"
    "    if lt(&x,&y) {\n"
    "        return x\n"
    "    } else {\n"                            // 60
    "        return rem(sub(x,y),y)\n"
    "    }\n"
    "}\n"
    "\n"
    "func one:   ()->Nat { return Succ($Nat) }\n"
    "func two:   ()->Nat { return Succ(one  ()) }\n"
    "func three: ()->Nat { return Succ(two  ()) }\n"
    "func four:  ()->Nat { return Succ(three()) }\n"
    "func five:  ()->Nat { return Succ(four ()) }\n"
    "func six:   ()->Nat { return Succ(five ()) }\n"
    "func seven: ()->Nat { return Succ(six  ()) }\n"
    "func eight: ()->Nat { return Succ(seven()) }\n"
    "func nine:  ()->Nat { return Succ(eight()) }\n"
    "func ten:   ()->Nat { return Succ(nine ()) }\n"
;

void chap_pre (void) {
    strcpy (INP,
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var n: (Nat,Nat) = ($Nat, Succ($Nat))\n"
        "call output(n)\n"
    );
    assert(all("($,Succ ($))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = $Nat\n"
        "var y: Nat = Succ($Nat)\n"
        "call output(lt(&x,&y))\n"
    );
    assert(all("True\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = $Nat\n"
        "var y: Nat = Succ($Nat)\n"
        "var z: Nat = add(x,&y)\n"
        "call output(z)\n"
    );
    assert(all("Succ ($)\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = Succ($Nat)\n"
        "var y: Nat = $Nat\n"
        "var z: Nat = add(x,&y)\n"
        "call output(z)\n"
    );
    assert(all("Succ ($)\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = two()\n"
        "var y: Nat = three()\n"
        "var z: Nat = add(x,&y)\n"
        "call output(z)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ ($)))))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var tw: Nat = two()\n"
        "var th: Nat = three()\n"
        "var n: Nat = mul(&tw,&th)\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ (Succ ($))))))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var n: Nat = rem(two(),three())\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ ($))\n", INP));
}

void chap_01 (void) {           // pg 1

    // 1.1                      // pg 1

    strcpy(INP, _bool);         // pg 1
    strcat(INP, _nat);
    strcat (INP,
        "func square: Nat -> Nat {\n"
        "    return mul(&arg,&arg)\n"
        "}\n"
        "var n: Nat = square(two)\n"
        "call output(n)\n"
    );
puts(INP);
    assert(all("Succ (Succ (Succ (Succ ($))))\n", INP));

    strcpy(INP, _bool);         // pg 2
    strcat(INP, _nat);
    strcat (INP,
        "func smaller: (Nat,Nat) -> Nat {\n"
        "    var x: Nat = arg.1\n"
        "    var y: Nat = arg.2\n"
        "    if lt(x,y) {\n"
        "        return x\n"
        "    } else {\n"
        "        return y\n"
        "    }\n"
        "}\n"
        "var n[]: Nat = add (smaller(ten,five), smaller(one,four))\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ (Succ ($))))))\n", INP));

    strcpy(INP, _bool);         // pg 3
    strcat(INP, _nat);
    strcat (INP,
        "func square: Nat -> Nat {\n"
        "    return mul(arg,arg)\n"
        "}\n"
        "func smaller: (Nat,Nat) -> Nat {\n"
        "    var x: Nat = arg.1\n"
        "    var y: Nat = arg.2\n"
        "    if lt(x,y) {\n"
        "        return x\n"
        "    } else {\n"
        "        return y\n"
        "    }\n"
        "}\n"
        "var n[]: Nat = square (smaller(four,two))\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Succ (Succ ($))))\n", INP));

    // TODO-delta               // pg 3

    // 1.2                      // pg 1

    strcpy(INP, _bool);         // pg 3
    strcat(INP, _nat);
    strcat (INP,
        "func fthree: () -> Nat {\n"
        "    return three\n" // should not allocate since comes from outside
        "}\n"                // but what if callee returns again?
        "var x[]: Nat = fthree()\n" // no problem b/c outside will detect and allocate three in received pool
        "call output(x)\n"
    );
    assert(all("Succ (Succ (Succ ($)))\n", INP));

}

int main (void) {
    chap_pre();
    chap_01();
    puts("OK");
}
