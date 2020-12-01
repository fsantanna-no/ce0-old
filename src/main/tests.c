#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../all.h"

void t_lexer (void) {
    // COMMENTS
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
    // IDENTIFIERS
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
        all_init(stropen("r", 0, "_char _Tp"));
        assert(ALL.tk1.enu == TK_NATIVE);  assert(!strcmp(ALL.tk1.val.s, "_char"));
        lexer(); assert(ALL.tk1.enu == TK_NATIVE); assert(!strcmp(ALL.tk1.val.s, "_Tp"));
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
    // SYMBOLS
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
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "(x"));
        Expr e;
        assert(!parser_expr(&e));
//puts(ALL.err);
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected `)´ : have end of file"));
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
    // EXPR_VAR
    {
        all_init(stropen("r", 0, "x"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_VAR); assert(!strcmp(e.tk.val.s,"x"));
        fclose(ALL.inp);
    }
    // EXPR_TUPLE
    {
        all_init(stropen("r", 0, "((),x,"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 7): expected expression : have end of file"));
    }
    {
        all_init(stropen("r", 0, "((),)"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 5): expected expression : have `)´"));
    }
    {
        all_init(stropen("r", 0, "((),x:"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 6): expected `)´ : have `:´"));
    }
    {
        all_init(stropen("r", 0, "((),x,())"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_TUPLE);
        assert(e.Tuple.size == 3);
        assert(e.Tuple.vec[1].sub == EXPR_VAR && !strcmp(e.Tuple.vec[1].tk.val.s,"x"));
        assert(e.Tuple.vec[2].sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    // EXPR_CALL
    {
        all_init(stropen("r", 0, "xxx (  )"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_CALL);
        assert(e.Call.func->sub == EXPR_VAR);
        assert(!strcmp(e.Call.func->tk.val.s, "xxx"));
        assert(e.Call.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(stropen("r", 0, "f()\n(  )\n()"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_CALL);
        assert(e.Call.func->sub == EXPR_CALL);
        assert(e.Call.func->Call.func->sub == EXPR_CALL);
        assert(e.Call.func->Call.func->Call.func->sub == EXPR_VAR);
        assert(!strcmp(e.Call.func->Call.func->Call.func->tk.val.s, "f"));
        fclose(ALL.inp);
    }
}

void t_parser_stmt (void) {
    {
        all_init(stropen("r", 0, "call f()"));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.sub == STMT_CALL);
        assert(s.call.sub == EXPR_CALL);
        assert(s.call.Call.func->sub == EXPR_VAR);
        assert(!strcmp(s.call.Call.func->tk.val.s,"f"));
        fclose(ALL.inp);
    }
}

void t_parser (void) {
    t_parser_expr();
    t_parser_stmt();
}

int main (void) {
    t_lexer();
    t_parser();
    puts("OK");
}
