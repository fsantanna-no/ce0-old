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
    "func lt: (&Nat,&Nat) -> Bool {\n"          // 10
    "    var x: &Nat = arg.1\n"
    "    var y: &Nat = arg.2\n"
    "    var tst: Bool = ?y.$Nat\n"
    "    if tst {\n"
    "        var b: Bool = False ()\n"
    "        return b\n"
    "    } else {\n"
    "        var tst: Bool = ?x.$Nat\n"
    "        if tst {\n"
    "            var b: Bool = True ()\n"       // 20
    "            return b\n"
    "        } else {\n"
    "            var x_: &Nat = !x.Succ\n"
    "            var y_: &Nat = !y.Succ\n"
    "            var a : Bool = lt(x_,y_)\n"
    "            return a\n"
    "        }\n"
    "    }\n"
    "}\n"
    "\n"                                        // 30
#if 0
    "func lte: (Nat,Nat) -> Bool {\n"
    "    var x: Nat = arg.1\n"
    "    var y: Nat = arg.2\n"
    "    return or(lt(x,y), eq(x,y))\n"
    "}\n"
    "\n"
#endif
    "func add: (Nat,&Nat) -> Nat {\n"
    "    var x: Nat  = arg.1\n"
    "    var y: &Nat = arg.2\n"
    "    var tst: Bool = ?y.$Nat\n"
    "    if tst {\n"
    "        return x\n"
    "    } else {\n"
    "        var a: &Nat = !y.Succ\n"
    "        var b: Nat = add(x,a)\n"
    "        var c: Nat = Succ(b)\n"            // 40
    "        return c\n"
    "    }\n"
    "}\n"
    "\n"
    "func sub: (Nat,Nat) -> Nat {\n"
    "    var x: Nat = arg.1\n"
    "    var y: Nat = arg.2\n"
    "    var tst: Bool = ?y.$Nat\n"
    "    if tst {\n"
    "        return x\n"                        // 50
    "    } else {\n"
    "        var a: Nat = !x.Succ\n"
    "        var b: Nat = !y.Succ\n"
    "        var c: Nat = sub(a,b)\n"
    "        return c\n"
    "    }\n"
    "}\n"
    "\n"
    "func mul: (&Nat,&Nat) -> Nat {\n"
    "    var x: &Nat = arg.1\n"                 // 60
    "    var y: &Nat = arg.2\n"
    "    var tst: Bool = ?y.$Nat\n"
    "    if tst {\n"
    "        var a:Nat = $Nat\n"
    "        return a\n"
    "    } else {\n"
    "        var a: &Nat = !y.Succ\n"
    "        var z: Nat = mul(x,a)\n"
    "        var b: Nat = add(z,x)\n"   // x ja foi consumido: dar erro
    "        return b\n"                // x ja foi consumido: dar erro // 70
    "    }\n"
    "}\n"
    "\n"
    "func rem: (Nat,Nat) -> Nat {\n"
    "    var x: Nat = arg.1\n"
    "    var y: Nat = arg.2\n"
    "    var x_: &Nat = &x\n"
    "    var y_: &Nat = &y\n"
    "    var tst: Bool = lt(x_,y_)\n"
    "    if tst {\n"                            // 80
    "        return x\n"
    "    } else {\n"
    "        var a: Nat = sub(x,y)\n"
    "        var b: Nat = rem(a,y)\n"
    "        return b\n"
    "    }\n"
    "}\n"
    "\n"
    "func one:   ()->Nat { var a:Nat=Succ($Nat) ; return a }\n"
    "func two:   ()->Nat { var a:Nat=one()   ; var b:Nat=Succ(a) ; return b }\n"    // 90
    "func three: ()->Nat { var a:Nat=two()   ; var b:Nat=Succ(a) ; return b }\n"
    "func four:  ()->Nat { var a:Nat=three() ; var b:Nat=Succ(a) ; return b }\n"
    "func five:  ()->Nat { var a:Nat=four()  ; var b:Nat=Succ(a) ; return b }\n"
    "func six:   ()->Nat { var a:Nat=five()  ; var b:Nat=Succ(a) ; return b }\n"    // 50
    "func seven: ()->Nat { var a:Nat=six()   ; var b:Nat=Succ(a) ; return b }\n"
    "func eight: ()->Nat { var a:Nat=seven() ; var b:Nat=Succ(a) ; return b }\n"
    "func nine:  ()->Nat { var a:Nat=eight() ; var b:Nat=Succ(a) ; return b }\n"
    "func ten:   ()->Nat { var a:Nat=nine()  ; var b:Nat=Succ(a) ; return b }\n"    // 98
;

void chap_pre (void) {
    strcpy (INP,
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var a: Nat = Succ($Nat)\n"
        "var n: (Nat,Nat) = ($Nat, a)\n"
        "call output(n)\n"
    );
    assert(all("($,Succ ($))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = $Nat\n"
        "var y: Nat = Succ($Nat)\n"
        "var x_: &Nat = &x\n"
        "var y_: &Nat = &y\n"
        "var l: Bool = lt(x_,y_)\n"
        "call output(l)\n"
    );
    assert(all("True\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = $Nat\n"
        "var y: Nat = Succ($Nat)\n"
        "var y_: &Nat = &y\n"
        "var z: Nat = add(x,y_)\n"
        "call output(z)\n"
    );
    assert(all("Succ ($)\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = Succ($Nat)\n"
        "var y: Nat = $Nat\n"
        "var y_: &Nat = &y\n"
        "var z: Nat = add(x,y_)\n"
        "call output(z)\n"
    );
    assert(all("Succ ($)\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = two()\n"
        "var y: Nat = three()\n"
        "var y_: &Nat = &y\n"
        "var z: Nat = add(x,y_)\n"
        "call output(z)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ ($)))))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var tw: Nat = two()\n"
        "var th: Nat = three()\n"
        "var tw_: &Nat = &tw\n"
        "var th_: &Nat = &th\n"
        "var n: Nat = mul(tw_,th_)\n"
        "call output(n)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ (Succ ($))))))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var a: Nat = two()\n"
        "var b: Nat = three()\n"
        "var n: Nat = rem(a,b)\n"
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
        "    var a: &Nat = &arg\n"
        "    var b: Nat = mul(a,a)\n"
        "    return b\n"
        "}\n"
        "var a: Nat = two()\n"
        "var b: Nat = square(a)\n"
        "call output(b)\n"
    );
    assert(all("Succ (Succ (Succ (Succ ($))))\n", INP));

// TODO: esse esta falhando pois nao consigo passar smaller() que retorna & para add que espera *
    strcpy(INP, _bool);         // pg 2
    strcat(INP, _nat);
    strcat (INP,
        "func smaller: (&Nat,&Nat) -> &Nat {\n"
        "    var x: &Nat = arg.1\n"
        "    var y: &Nat = arg.2\n"
        "    var tst: Bool = lt(x,y)\n"
        "    if tst {\n"
        "        return x\n"
        "    } else {\n"
        "        return y\n"
        "    }\n"
        "}\n"
        "var one_ : Nat = one()\n"
        "var four_: Nat = four()\n"
        "var ten_ : Nat = ten()\n"
        "var five_: Nat = five()\n"
        "var c: &Nat = &ten_\n"
        "var d: &Nat = &five_\n"
        "var a: &Nat = smaller(c,d)\n"
        "var e: &Nat = &one_\n"
        "var f: &Nat = &four_\n"
        "var b: &Nat = smaller(e,f)\n"
        "var n: Nat = add (a, b)\n"
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
