#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../all.h"

void t_lexer (void) {
    {
        all_init(stropen("r", 0, "-- foobar"));
        assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "-- foobar"));
        assert(ALL.tk1.enu == TK_EOF); assert(ALL.tk1.lin == 1); assert(ALL.tk1.col == 10);
        lexer(); assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "-- c1\n--c2\n\n"));
        assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "c1\nc2 c3  \n    \nc4"));
        assert(ALL.tk1.enu == TK_VAR); assert(!strcmp(ALL.tk1.val.s, "c1"));
        lexer(); assert(ALL.tk1.enu == TK_VAR); assert(!strcmp(ALL.tk1.val.s, "c2")); assert(ALL.tk1.lin == 2);
        lexer(); assert(ALL.tk1.enu == TK_VAR); assert(!strcmp(ALL.tk1.val.s, "c3")); assert(ALL.tk1.col == 4);
        lexer(); assert(ALL.tk1.enu == TK_VAR); assert(!strcmp(ALL.tk1.val.s, "c4"));
        lexer(); assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "c1 C1 Ca a C"));
        assert(ALL.tk1.enu == TK_VAR);  assert(!strcmp(ALL.tk1.val.s, "c1"));
        lexer(); assert(ALL.tk1.enu == TK_TYPE); assert(!strcmp(ALL.tk1.val.s, "C1"));
        lexer(); assert(ALL.tk1.enu == TK_TYPE); assert(!strcmp(ALL.tk1.val.s, "Ca")); assert(ALL.tk1.lin == 1);
        lexer(); assert(ALL.tk1.enu == TK_VAR);  assert(!strcmp(ALL.tk1.val.s, "a")); assert(ALL.tk1.col == 10);
        lexer(); assert(ALL.tk1.enu == TK_TYPE); assert(!strcmp(ALL.tk1.val.s, "C"));
        lexer(); assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "val xval valx"));
        assert(ALL.tk1.enu == TK_VAL);
        lexer(); assert(ALL.tk1.enu == TK_VAR); assert(!strcmp(ALL.tk1.val.s, "xval")); assert(ALL.tk1.col == 5);
        lexer(); assert(ALL.tk1.enu == TK_VAR); assert(!strcmp(ALL.tk1.val.s, "valx"));
        lexer(); assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, ": }{ :"));
        assert(ALL.tk1.enu == ':');
        lexer(); assert(ALL.tk1.enu == '}');
        lexer(); assert(ALL.tk1.enu == '{');
        lexer(); assert(ALL.tk1.enu == ':'); assert(ALL.tk1.col == 6);
        fclose(ALL.inp);
    }
}

void t_parser_expr (void) {
    // UNIT
    {
        all_init(stropen("r", 0, "()"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    // PARENS
    {
        all_init(stropen("r", 0, "(())"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "("));
        Expr e;
        assert(!parser_expr(&e));
//puts(ALL.err);
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "( ( ) )"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "(("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "(\n( \n"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 3, col 1): expected expression : have end of file"));
        fclose(ALL.inp);
    }
}

void t_parser (void) {
    t_parser_expr();
}

int main (void) {
    t_lexer();
    t_parser();
    puts("OK");
}
