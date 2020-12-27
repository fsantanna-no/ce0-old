#include <assert.h>
#include <stdio.h>
#include <string.h>

//#define VALGRIND

#include "../all.h"

int all (const char* xp, char* src) {
    static char out[65000];
    Stmt* s;
    if (!all_init (
        stropen("w", sizeof(out), out),
        stropen("r", 0, src)
    )) {
        puts(ALL.err);
        return 0;
    };
//puts(src);
    if (!parser(&s)) {
        puts(ALL.err);
        return 0;
    }
    if (!env(s)) {
        puts(ALL.err);
        return 0;
    }
    code(s);
    fclose(ALL.out);
#if 0
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
#ifdef VALGRIND
        FILE* f = popen("valgrind ./a.out", "r");
#else
        FILE* f = popen("./a.out", "r");
#endif
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
    "    var tst: Bool = y.$Nat?\n"
    "    if tst {\n"
    "        var b: Bool = False ()\n"
    "        return b\n"
    "    } else {\n"
    "        var tst: Bool = x.$Nat?\n"
    "        if tst {\n"
    "            var b: Bool = True ()\n"       // 20
    "            return b\n"
    "        } else {\n"
    "            var x_: &Nat = x.Succ!\n"
    "            var y_: &Nat = y.Succ!\n"
    "            var b : (&Nat,&Nat) = (x_,y_)\n"
    "            var a : Bool = lt b\n"
    "            return a\n"
    "        }\n"
    "    }\n"
    "}\n"                                       // 30
    "\n"
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
    "}\n"                                       // 60
    "\n"
    "func mul: (&Nat,&Nat) -> Nat {\n"
    "    var x: &Nat = arg.1\n"
    "    var y: &Nat = arg.2\n"
    "    var tst: Bool = y.$Nat?\n"
    "    if tst {\n"
    "        var a:Nat = $Nat\n"
    "        return a\n"
    "    } else {\n"
    "        var a: &Nat = y.Succ!\n"           // 70
    "        var c: (&Nat,&Nat) = (x,a)\n"
    "        var z: Nat = mul c\n"
    "        var d: (Nat,&Nat) = (z,x)\n"   // x ja foi consumido: dar erro
    "        var b: Nat = add d\n"
    "        return b\n"
    "    }\n"
    "}\n"
    "\n"
    "func rem: (&Nat,&Nat) -> Nat {\n"  // 79
    "    var x: &Nat = arg.1\n"
    "    var y: &Nat = arg.2\n"
    "    if lt (x,y) {\n"
    "        return clone(x)\n"
    "    } else {\n"
    "        var x_: Nat = clone x\n"
    "        var y_: Nat = clone y\n"
    "        var xy: Nat = sub (x_,y_)\n"
    "        return rem (&xy, y)\n"
    "    }\n"
    "}\n"
    "\n"
    "func one:   ()->Nat { var a:Nat=Succ $Nat ; return a }\n"
    "func two:   ()->Nat { var a:Nat=one()   ; var b:Nat=Succ a ; return b }\n"
    "func three: ()->Nat { var a:Nat=two()   ; var b:Nat=Succ a ; return b }\n"
    "func four:  ()->Nat { var a:Nat=three() ; var b:Nat=Succ a ; return b }\n" // 100
    "func five:  ()->Nat { var a:Nat=four()  ; var b:Nat=Succ a ; return b }\n"
    "func six:   ()->Nat { var a:Nat=five()  ; var b:Nat=Succ a ; return b }\n"
    "func seven: ()->Nat { var a:Nat=six()   ; var b:Nat=Succ a ; return b }\n"
    "func eight: ()->Nat { var a:Nat=seven() ; var b:Nat=Succ a ; return b }\n"
    "func nine:  ()->Nat { var a:Nat=eight() ; var b:Nat=Succ a ; return b }\n"
    "func ten:   ()->Nat { var a:Nat=nine()  ; var b:Nat=Succ a ; return b }\n" // 106
;

void chap_pre (void) {
    strcpy (INP,
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var n: (Nat,Nat) = ($Nat, Succ($Nat))\n"
        "call show n\n"
    );
    assert(all("($,Succ ($))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = $Nat\n"
        "var y: Nat = Succ $Nat\n"
        "var x_: &Nat = &x\n"
        "var y_: &Nat = &y\n"
        "var a: (&Nat,&Nat) = (x_,y_)\n"
        "var l: Bool = lt a\n"
        "call show l\n"
    );
    assert(all("True\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = $Nat\n"
        "var y: Nat = Succ($Nat)\n"
        "var z: Nat = add(x,&y)\n"
        "call show z\n"
    );
    assert(all("Succ ($)\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = Succ $Nat\n"
        "var y: Nat = $Nat\n"
        "var y_: &Nat = &y\n"
        "var a: (Nat,&Nat) = (x,y_)\n"
        "var z: Nat = add a\n"
        "var z_: &Nat = &z\n"
        "call show z_\n"
    );
    assert(all("Succ ($)\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var x: Nat = two()\n"
        "var y: Nat = three()\n"
        "var z: Nat = add(x,&y)\n"
        "call show(z)\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ ($)))))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var tw: Nat = two()\n"
        "var th: Nat = three()\n"
        "var tw_: &Nat = &tw\n"
        "var th_: &Nat = &th\n"
        "var a: (&Nat,&Nat) = (tw_,th_)\n"
        "var n: Nat = mul a\n"
        "var n_: &Nat = &n\n"
        "call show n_\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ (Succ ($))))))\n", INP));

    strcpy(INP, _bool);
    strcat(INP, _nat);
    strcat (INP,
        "var a: Nat = two()\n"
        "var b: Nat = three()\n"
        "var n: Nat = rem (&a,&b)\n"
        "var n_: &Nat = &n\n"
        "call show n_\n"
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
        "    var c: (&Nat,&Nat) = (a,a)\n"
        "    var b: Nat = mul c\n"
        "    return b\n"
        "}\n"
        "var a: Nat = two()\n"
        "var b: Nat = square a\n"
        "var b_: &Nat = &b\n"
        "call show b_\n"
    );
    assert(all("Succ (Succ (Succ (Succ ($))))\n", INP));

    strcpy(INP, _bool);         // pg 2
    strcat(INP, _nat);
    strcat (INP,
        "func smaller: (&Nat,&Nat) -> &Nat {\n"
        "    var x: &Nat = arg.1\n"
        "    var y: &Nat = arg.2\n"
        "    var a: (&Nat,&Nat) = (x,y)\n"
        "    var tst: Bool = lt a\n"
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
        "var i: (&Nat,&Nat) = (c,d)\n"
        "var a: &Nat = smaller i\n"
        "var e: &Nat = &one_\n"
        "var f: &Nat = &four_\n"
        "var j: (&Nat,&Nat) = (e,f)\n"
        "var b: &Nat = smaller j\n"
        "var g: Nat = clone a\n"
        "var k: (Nat,&Nat) = (g, b)\n"
        "var n: Nat = add k\n"
        "var n_: &Nat = &n\n"
        "call show n_\n"
    );
    assert(all("Succ (Succ (Succ (Succ (Succ (Succ ($))))))\n", INP));

    strcpy(INP, _bool);         // pg 3
    strcat(INP, _nat);
    strcat (INP,
        "func square: Nat -> Nat {\n"
        "    var n1: &Nat = &arg\n"
        "    var n2: &Nat = &arg\n"
        "    var n1n2: (&Nat,&Nat) = (n1,n2)\n"
        "    var n: Nat = mul n1n2\n"
        "    return n\n"
        "}\n"
        "func smaller: (&Nat,&Nat) -> &Nat {\n"
        "    var x: &Nat = arg.1\n"
        "    var y: &Nat = arg.2\n"
        "    var a: (&Nat,&Nat) = (x,y)\n"
        "    var tst: Bool = lt a\n"
        "    if tst {\n"
        "        return x\n"
        "    } else {\n"
        "        return y\n"
        "    }\n"
        "}\n"
        "var a: Nat = four()\n"
        "var b: Nat = two()\n"
        "var a_: &Nat = &a\n"
        "var b_: &Nat = &b\n"
        "var e: (&Nat,&Nat) = (a_,b_)\n"
        "var c: &Nat = smaller e\n"
        "var d: Nat = clone c\n"
        "var n: Nat = square d\n"
        "var n_: &Nat = &n\n"
        "call show n_\n"
    );
    assert(all("Succ (Succ (Succ (Succ ($))))\n", INP));

    // TODO-delta               // pg 3

    // 1.2                      // pg 1

    strcpy(INP, _bool);         // pg 3
    strcat(INP, _nat);
    strcat (INP,
        "func fthree: () -> Nat {\n"
        "    var n: Nat = three()\n"
        "    return n\n"
        "}\n"
        "var x: Nat = fthree()\n"
        "var x_: &Nat = &x\n"
        "call show x_\n"
    );
    assert(all("Succ (Succ (Succ ($)))\n", INP));

}

int main (void) {
    chap_pre();
    chap_01();
    puts("OK");
}
