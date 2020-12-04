#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../all.h"

int all (const char* xp, char* src) {
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
