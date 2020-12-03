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

void t_lexer (void) {
    // COMMENTS
    {
        all_init(NULL, stropen("r", 0, "-- foobar"));
        assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "-- foobar"));
        assert(ALL.tk1.enu == TK_EOF); assert(ALL.tk1.lin == 1); assert(ALL.tk1.col == 10);
        lexer(); assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "-- c1\n--c2\n\n"));
        assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    // KEYWORDS
    {
        all_init(NULL, stropen("r", 0, "xval val else valx type"));
        assert(ALL.tk1.enu == TX_VAR);
        lexer(); assert(ALL.tk1.enu == TK_VAL);
        lexer(); assert(ALL.tk1.enu == TK_ELSE);
        lexer(); assert(ALL.tk1.enu == TX_VAR);
        lexer(); assert(ALL.tk1.enu == TK_TYPE);
        fclose(ALL.inp);
    }
    // IDENTIFIERS
    {
        all_init(NULL, stropen("r", 0, "c1\nc2 c3  \n    \nc4"));
        assert(ALL.tk1.enu == TX_VAR); assert(!strcmp(ALL.tk1.val.s, "c1"));
        lexer(); assert(ALL.tk1.enu == TX_VAR); assert(!strcmp(ALL.tk1.val.s, "c2")); assert(ALL.tk1.lin == 2);
        lexer(); assert(ALL.tk1.enu == TX_VAR); assert(!strcmp(ALL.tk1.val.s, "c3")); assert(ALL.tk1.col == 4);
        lexer(); assert(ALL.tk1.enu == TX_VAR); assert(!strcmp(ALL.tk1.val.s, "c4"));
        lexer(); assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "c1 C1 Ca a C"));
        assert(ALL.tk1.enu == TX_VAR);  assert(!strcmp(ALL.tk1.val.s, "c1"));
        lexer(); assert(ALL.tk1.enu == TX_TYPE); assert(!strcmp(ALL.tk1.val.s, "C1"));
        lexer(); assert(ALL.tk1.enu == TX_TYPE); assert(!strcmp(ALL.tk1.val.s, "Ca")); assert(ALL.tk1.lin == 1);
        lexer(); assert(ALL.tk1.enu == TX_VAR);  assert(!strcmp(ALL.tk1.val.s, "a")); assert(ALL.tk1.col == 10);
        lexer(); assert(ALL.tk1.enu == TX_TYPE); assert(!strcmp(ALL.tk1.val.s, "C"));
        lexer(); assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "_char _Tp"));
        assert(ALL.tk1.enu == TX_NATIVE);  assert(!strcmp(ALL.tk1.val.s, "_char"));
        lexer(); assert(ALL.tk1.enu == TX_NATIVE); assert(!strcmp(ALL.tk1.val.s, "_Tp"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "val xval valx"));
        assert(ALL.tk1.enu == TK_VAL);
        lexer(); assert(ALL.tk1.enu == TX_VAR); assert(!strcmp(ALL.tk1.val.s, "xval")); assert(ALL.tk1.col == 5);
        lexer(); assert(ALL.tk1.enu == TX_VAR); assert(!strcmp(ALL.tk1.val.s, "valx"));
        lexer(); assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    // SYMBOLS
    {
        all_init(NULL, stropen("r", 0, ": }{ :"));
        assert(ALL.tk1.enu == ':');
        lexer(); assert(ALL.tk1.enu == '}');
        lexer(); assert(ALL.tk1.enu == '{');
        lexer(); assert(ALL.tk1.enu == ':'); assert(ALL.tk1.col == 6);
        fclose(ALL.inp);
    }
    // TUPLE INDEX
    {
        all_init(NULL, stropen("r", 0, ".1a"));
        assert(ALL.tk1.enu == '.');
        lexer(); assert(ALL.tk1.enu == TK_ERR);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ".10"));
        assert(ALL.tk1.enu == '.');
        lexer(); assert(ALL.tk1.enu == TX_INDEX);
        assert(ALL.tk1.val.n == 10);
        fclose(ALL.inp);
    }
}

void t_parser_type (void) {
    {
        all_init(NULL, stropen("r", 0, "_char"));
        Type tp;
        parser_type(&tp);
        assert(tp.sub == TYPE_NATIVE);
        assert(!strcmp(tp.tk.val.s,"_char"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "()"));
        Type tp;
        parser_type(&tp);
        assert(tp.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "((),())"));
        Type tp;
        parser_type(&tp);
        assert(tp.sub == TYPE_TUPLE);
        assert(tp.Tuple.size == 2);
        assert(tp.Tuple.vec[1].sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "xxx"));
        Type tp;
        parser_type(&tp);
        assert(!strcmp(ALL.err, "(ln 1, col 1): expected type : have \"xxx\""));
        fclose(ALL.inp);
    }
}

#if 0
void t_parser_data (void) {
    {
        all_init(NULL, stropen("r", 0, "data Err"));
        Data dts;
        parser_data(&dts);
        assert(!strcmp(ALL.err, "(ln 1, col 9): expected data declaration : have end of line"));
        //assert(parser_data(&dts));
        //assert(ALL.data.datas.size == 1);
        fclose(ALL.inp);
    }
}
#endif

void t_parser_expr (void) {
    // UNIT
    {
        all_init(NULL, stropen("r", 0, "()"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    // PARENS
    {
        all_init(NULL, stropen("r", 0, "(())"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "(x"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected `)´ : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "( ( ) )"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "(("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "(\n( \n"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 3, col 1): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    // EXPR_VAR
    {
        all_init(NULL, stropen("r", 0, "x"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_VAR); assert(!strcmp(e.tk.val.s,"x"));
        fclose(ALL.inp);
    }
    // EXPR_NATIVE
    {
        all_init(NULL, stropen("r", 0, "_x"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_NATIVE); assert(!strcmp(e.tk.val.s,"_x"));
        fclose(ALL.inp);
    }
    // EXPR_TUPLE
    {
        all_init(NULL, stropen("r", 0, "((),x,"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 7): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "((),)"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 5): expected expression : have `)´"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "((),x:"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 6): expected `)´ : have `:´"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "((),x,())"));
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
        all_init(NULL, stropen("r", 0, "xxx (  )"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_CALL);
        assert(e.Call.func->sub == EXPR_VAR);
        assert(!strcmp(e.Call.func->tk.val.s, "xxx"));
        assert(e.Call.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "f()\n(  )\n()"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_CALL);
        assert(e.Call.func->sub == EXPR_CALL);
        assert(e.Call.func->Call.func->sub == EXPR_CALL);
        assert(e.Call.func->Call.func->Call.func->sub == EXPR_VAR);
        assert(!strcmp(e.Call.func->Call.func->Call.func->tk.val.s, "f"));
        fclose(ALL.inp);
    }
    // EXPR_CONS
    {
        all_init(NULL, stropen("r", 0, "Bool.True ()"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_CONS);
        assert(!strcmp(e.Cons.type.val.s,"Bool"));
        assert(!strcmp(e.Cons.subtype.val.s,"True"));
        assert(e.Cons.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    // EXPR_INDEX
    {
        all_init(NULL, stropen("r", 0, "x.1"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_INDEX);
        assert(e.Index.tuple->sub == EXPR_VAR);
        assert(e.Index.index == 1);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "x().1()"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_CALL);
        assert(e.Call.func->sub == EXPR_INDEX);
        assert(e.Call.func->Index.tuple->sub == EXPR_CALL);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "x().."));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 5): expected index or subtype : have `.´"));
        fclose(ALL.inp);
    }
}

void t_parser_stmt (void) {
    // STMT_VAR
    {
        all_init(NULL, stropen("r", 0, "val :"));
        Stmt s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 5): expected variable identifier : have `:´"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "val x x"));
        Stmt s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 7): expected `:´ : have \"x\""));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "val x: x"));
        Stmt s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 8): expected type : have \"x\""));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "val x: ()"));
        Stmt s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 10): expected `=´ : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "val x: () = ()"));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.sub == STMT_VAR);
        assert(s.Var.id.enu == TX_VAR);
        assert(s.Var.type.sub == TYPE_UNIT);
        assert(s.Var.init.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "val x: ((),((),())) = ()"));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.sub == STMT_VAR);
        assert(s.Var.id.enu == TX_VAR);
        assert(s.Var.type.sub == TYPE_TUPLE);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "val a : (_char) = ()"));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.Var.type.sub == TYPE_NATIVE);
        fclose(ALL.inp);
    }
    // STMT_TYPE
    {
        all_init(NULL, stropen("r", 0, "type Bool"));
        Stmt s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 10): expected `{´ : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "type Bool {}"));
        Stmt s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 12): expected type identifier : have `}´"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "type Bool { True: ()"));
        Stmt s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 21): expected `}´ : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "type Bool { False:() ; True:() }"));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.sub == STMT_TYPE);
        assert(!strcmp(s.Type.id.val.s, "Bool"));
        assert(s.Type.size == 2);
        assert(s.Type.vec[0].type.sub == TYPE_UNIT);
        assert(!strcmp(s.Type.vec[1].id.val.s, "True"));
        fclose(ALL.inp);
    }
    // STMT_CALL
    {
        all_init(NULL, stropen("r", 0, "call f()"));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.sub == STMT_CALL);
        assert(s.call.sub == EXPR_CALL);
        assert(s.call.Call.func->sub == EXPR_VAR);
        assert(!strcmp(s.call.Call.func->tk.val.s,"f"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "call _printf()"));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.sub == STMT_CALL);
        assert(s.call.sub == EXPR_CALL);
        assert(s.call.Call.func->sub == EXPR_NATIVE);
        assert(!strcmp(s.call.Call.func->tk.val.s,"_printf"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "call f() ; call g()"));
        Stmt s;
        assert(parser_stmts(&s));
        assert(s.sub == STMT_SEQ);
        assert(s.Seq.size == 2);
        assert(s.Seq.vec[1].sub == STMT_CALL);
        fclose(ALL.inp);
    }
    // STMT_IF
    {
        all_init(NULL, stropen("r", 0, "if () { } else { call () }"));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.sub == STMT_IF);
        assert(s.If.cond.sub == EXPR_UNIT);
        assert(s.If.true ->sub==STMT_SEQ && s.If.true ->Seq.size==0);
        assert(s.If.false->sub==STMT_SEQ && s.If.false->Seq.size==1);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "if () { call () } "));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.sub == STMT_IF);
        assert(s.If.false->sub==STMT_SEQ && s.If.false->Seq.size==0);
        fclose(ALL.inp);
    }
}

void t_code (void) {
    // EXPR_UNIT
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr e = { EXPR_UNIT };
        code_expr(e);
        fclose(ALL.out);
        assert(!strcmp(out,"1"));
    }
    // EXPR_VAR
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr e = { EXPR_VAR, {} };
            e.tk.enu = TX_VAR;
            strcpy(e.tk.val.s, "xxx");
        code_expr(e);
        fclose(ALL.out);
        assert(!strcmp(out,"xxx"));
    }
    // EXPR_NATIVE
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr e = { EXPR_NATIVE, {} };
            e.tk.enu = TX_NATIVE;
            strcpy(e.tk.val.s, "_printf");
        code_expr(e);
        fclose(ALL.out);
        assert(!strcmp(out,"printf"));
    }
    // EXPR_TUPLE
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr es[2] = {{EXPR_UNIT},{EXPR_UNIT}};
        Expr e = { EXPR_TUPLE, {.Tuple={2,es}} };
        code_expr(e);
        fclose(ALL.out);
        assert(!strcmp(out,"((TUPLE2){ (void*)1,(void*)1 })"));
    }
    // EXPR_INDEX
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr es[2] = {{EXPR_UNIT},{EXPR_UNIT}};
        Expr tuple = { EXPR_TUPLE, {.Tuple={2,es}} };
        Expr e = { EXPR_INDEX, { .Index={.tuple=&tuple,.index=2} } };
        code_expr(e);
        fclose(ALL.out);
        assert(!strcmp(out,"((TUPLE2){ (void*)1,(void*)1 })._2"));
    }
    {
        char out[1024] = "";
        all_init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "val a : () = () ; call _show_Unit(a)")
        );
        Stmt s;
        assert(parser_stmts(&s));
        code(s);
        fclose(ALL.out);
        char* ret =
            "#include <assert.h>\n"
            "#include <stdio.h>\n"
            "typedef struct { void *_1, *_2;      } TUPLE2;\n"
            "typedef struct { void *_1, *_2, *_3; } TUPLE3;\n"
            "#define show_Unit_(x) (assert(((long)(x))==1), printf(\"()\"))\n"
            "#define show_Unit(x) (show_Unit_(x), puts(\"\"))\n"
            "int main (void) {\n"
            "\n"
            "int a = 1;\n"
            "show_Unit(a);\n"
            "\n"
            "}\n";
        assert(!strcmp(out,ret));
    }
    // STMT_TYPE
    {
        char out[1024] = "";
        all_init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "type Bool { False: () ; True: () }")
        );
        Stmt s;
        assert(parser_stmts(&s));
        code(s);
        fclose(ALL.out);
        char* ret =
            "#include <assert.h>\n"
            "#include <stdio.h>\n"
            "typedef struct { void *_1, *_2;      } TUPLE2;\n"
            "typedef struct { void *_1, *_2, *_3; } TUPLE3;\n"
            "#define show_Unit_(x) (assert(((long)(x))==1), printf(\"()\"))\n"
            "#define show_Unit(x) (show_Unit_(x), puts(\"\"))\n"
            "int main (void) {\n"
            "\n"
            "typedef enum {\n"
            "    False,\n"
            "    True\n"
            "} BOOL;\n"
            "\n"
            "typedef struct Bool {\n"
            "    BOOL sub;\n"
            "    union {\n"
            "        int _False;\n"
            "        int _True;\n"
            "    };\n"
            "} Bool;\n"
            "\n"
            "void show_Bool_ (Bool v) {\n"
            "    switch (v.sub) {\n"
            "        case False:\n"
            "            printf(\"False \");\n"
            "            show_Unit_(v._False);\n"
            "            break;\n"
            "        case True:\n"
            "            printf(\"True \");\n"
            "            show_Unit_(v._True);\n"
            "            break;\n"
            "    }\n"
            "}\n"
            "void show_Bool (Bool v) {\n"
            "    show_Bool_(v);\n"
            "    puts(\"\");\n"
            "}\n"
            "\n"
            "}\n";
        assert(!strcmp(out,ret));
    }
}

void t_all (void) {
    // UNIT
    assert(all(
        "()\n",
        "call _show_Unit(())\n"
    ));
    assert(all(
        "()\n",
        "val x: () = ()\n"
        "call _show_Unit(x)\n"
    ));
    // NATIVE
    assert(all(
        "A",
        "val x: _char = _65\n"
        "call _putchar(x)\n"
    ));
    assert(all(
        "()\n",
        "call _show_Unit(((),()).1)\n"
    ));
    // TYPE
    assert(all(
        "False ()\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = Bool.False()\n"
        "call _show_Bool(b)\n"
    ));
    assert(all(
        "Zz1 ()\n",
        "type Zz { Zz1:() }\n"
        "type Yy { Yy1:Zz }\n"
        "type Xx { Xx1:Yy }\n"
        "val x : Xx = Xx.Xx1(Yy.Yy1(Zz.Zz1()))\n"
        "call _show_Zz(x.Xx1!.Yy1!)\n"
    ));
    // IF
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = Bool.False()\n"
        "if b { } else { call _show_Unit() }\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = Bool.True()\n"
        "if b { call _show_Unit() }\n"
    ));
    // PREDICATE
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = Bool.True()\n"
        "if b.True? { call _show_Unit(()) }\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = Bool.True()\n"
        "if b.False? { } else { call _show_Unit(()) }\n"
    ));
}

void t_parser (void) {
    t_parser_type();
    //t_parser_data();
    t_parser_expr();
    t_parser_stmt();
}

int main (void) {
    t_lexer();
    t_parser();
    t_code();
    t_all();
    puts("OK");
}
