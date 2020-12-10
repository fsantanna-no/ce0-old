#include <assert.h>
#include <stdio.h>
#include <string.h>

#define DEBUG

#include "../all.h"

int all (const char* xp, char* src) {
    static char out[65000];
    Stmt s;

    if (!all_init (
        stropen("w", sizeof(out), out),
        stropen("r", 0, src)
    )) {
#ifdef DEBUG
        puts(ALL.err);
#endif
        return !strcmp(ALL.err, xp);
    }

    if (!parser(&s)) {
#ifdef DEBUG
        puts(ALL.err);
#endif
        return !strcmp(ALL.err, xp);
    }

    if (!env(&s)) {
        puts(ALL.err);
        return !strcmp(ALL.err, xp);
    }
    code(&s);
    fclose(ALL.out);

#ifdef DEBUG
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

#ifdef DEBUG
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
    // SYMBOLS
    {
        all_init(NULL, stropen("r", 0, "( -> ,"));
        assert(ALL.tk1.enu == '(');
        lexer(); assert(ALL.tk1.enu == TK_ARROW);
        lexer(); assert(ALL.tk1.enu == ',');
    }
    {
        all_init(NULL, stropen("r", 0, ": }{ :"));
        assert(ALL.tk1.enu == ':');
        lexer(); assert(ALL.tk1.enu == '}');
        lexer(); assert(ALL.tk1.enu == '{');
        lexer(); assert(ALL.tk1.enu == ':'); assert(ALL.tk1.col == 6);
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
        lexer(); assert(ALL.tk1.enu == TX_USER); assert(!strcmp(ALL.tk1.val.s, "C1"));
        lexer(); assert(ALL.tk1.enu == TX_USER); assert(!strcmp(ALL.tk1.val.s, "Ca")); assert(ALL.tk1.lin == 1);
        lexer(); assert(ALL.tk1.enu == TX_VAR);  assert(!strcmp(ALL.tk1.val.s, "a")); assert(ALL.tk1.col == 10);
        lexer(); assert(ALL.tk1.enu == TX_USER); assert(!strcmp(ALL.tk1.val.s, "C"));
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
        lexer(); assert(ALL.tk1.enu == TX_NUM);
        assert(ALL.tk1.val.n == 10);
        fclose(ALL.inp);
    }
}

void t_parser_type (void) {
    // TYPE_NATIVE
    {
        all_init(NULL, stropen("r", 0, "_char"));
        Type tp;
        parser_type(&tp);
        assert(tp.sub == TYPE_NATIVE);
        assert(!strcmp(tp.tk.val.s,"_char"));
        fclose(ALL.inp);
    }
    // TYPE_UNIT
    {
        all_init(NULL, stropen("r", 0, "()"));
        Type tp;
        parser_type(&tp);
        assert(tp.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    // TYPE_TUPLE
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
    // TYPE_FUNC
    {
        all_init(NULL, stropen("r", 0, "() -> ()"));
        Type tp;
        parser_type(&tp);
        assert(tp.sub == TYPE_FUNC);
        assert(tp.Func.inp->sub == TYPE_UNIT);
        assert(tp.Func.out->sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
}

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
        all_init(NULL, stropen("r", 0, "True ()"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_CONS);
        assert(!strcmp(e.Cons.sub.val.s,"True"));
        assert(e.Cons.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "Zz1 ((),())"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_CONS);
        assert(!strcmp(e.Cons.sub.val.s,"Zz1"));
        assert(e.Cons.arg->sub == EXPR_TUPLE);
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
        assert(s.sub == STMT_USER);
        assert(!strcmp(s.User.id.val.s, "Bool"));
        assert(s.User.size == 2);
        assert(s.User.vec[0].type.sub == TYPE_UNIT);
        assert(!strcmp(s.User.vec[1].id.val.s, "True"));
        fclose(ALL.inp);
    }
    // STMT_CALL
    {
        all_init(NULL, stropen("r", 0, "output()"));
        Stmt s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 1): expected statement (maybe `call´?) : have `output´"));
        fclose(ALL.inp);
    }
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
    // STMT_FUNC
    {
        all_init(NULL, stropen("r", 0, "func f : () -> () { return () }"));
        Stmt s;
        assert(parser_stmt(&s));
        assert(s.sub == STMT_FUNC);
        assert(s.Func.type.sub == TYPE_FUNC);
        assert(!strcmp(s.Func.id.val.s, "f"));
        assert(s.Func.body->sub == STMT_SEQ);
        assert(s.Func.body->Seq.vec[0].sub == STMT_RETURN);
        fclose(ALL.inp);
    }
}

void t_code (void) {
    // EXPR_UNIT
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr e = { _N_++, EXPR_UNIT, NULL };
        code_expr_1(&e);
        fclose(ALL.out);
        assert(!strcmp(out,"1"));
    }
    // EXPR_VAR
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr e = { _N_++, EXPR_VAR, NULL, {} };
            e.tk.enu = TX_VAR;
            strcpy(e.tk.val.s, "xxx");
        code_expr_1(&e);
        fclose(ALL.out);
        assert(!strcmp(out,"xxx"));
    }
    // EXPR_NATIVE
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr e = { _N_++, EXPR_NATIVE, NULL, {} };
            e.tk.enu = TX_NATIVE;
            strcpy(e.tk.val.s, "_printf");
        code_expr_1(&e);
        fclose(ALL.out);
        assert(!strcmp(out,"printf"));
    }
    // EXPR_TUPLE
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr es[2] = {{_N_++,EXPR_UNIT},{_N_++,EXPR_UNIT}};
        Expr e = { _N_++, EXPR_TUPLE, NULL, {.Tuple={2,es}} };
        code_expr_1(&e);
        fclose(ALL.out);
        assert(!strcmp(out,"((TUPLE__Unit__Unit) { 1,1 })"));
    }
    // EXPR_INDEX
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr es[2] = {{_N_++,EXPR_UNIT,NULL},{_N_++,EXPR_UNIT,NULL}};
        Expr tuple = { _N_++, EXPR_TUPLE, NULL, {.Tuple={2,es}} };
        Expr e = { _N_++, EXPR_INDEX, NULL, { .Index={.tuple=&tuple,.index=2} } };
        code_expr_1(&e);
        fclose(ALL.out);
        assert(!strcmp(out,"((TUPLE__Unit__Unit) { 1,1 })._2"));
    }
    {
        char out[8192] = "";
        all_init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "val a : () = () ; call _output_Unit(a)")
        );
        Stmt s;
        assert(parser_stmts(&s));
        code(&s);
        fclose(ALL.out);
        char* ret =
            "#include <assert.h>\n"
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n"
            "#define MIN(x,y)   (((x) < (y)) ? (x) : (y))\n"
            "#define MAX(x,y)   (((x) > (y)) ? (x) : (y))\n"
            "#define BET(x,y,z) (MIN(x,z)<y && y<MAX(x,z))\n"
            "#define output_Unit_(x) (assert(((long)(x))==1), printf(\"()\"))\n"
            "#define output_Unit(x)  (output_Unit_(x), puts(\"\"))\n"
            "typedef struct {\n"
            "    void* buf;\n"
            "    int max;\n"
            "    int cur;\n"
            "} Pool;\n"
            "void* pool_alloc (Pool* pool, int n) {\n"
            "    if (pool == NULL) {\n"
            "        return malloc(n);\n"
            "    } else {\n"
            "        void* ret = pool->buf + pool->cur;\n"
            "        pool->cur += n;\n"
            "        if (pool->cur <= pool->max) {\n"
            "            return ret;\n"
            "        } else {\n"
            "            return NULL;\n"
            "        }\n"
            "    }\n"
            "}\n"
            "int main (void) {\n"
            "    void* _STACK = &_STACK;\n"
            "\n"
            "int a = 1;\n"
            "output_Unit(a);\n"
            "\n"
            "}\n";
        assert(!strcmp(out,ret));
    }
    // STMT_TYPE
    {
        char out[8192] = "";
        all_init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "type Bool { False: () ; True: () }")
        );
        Stmt s;
        assert(parser_stmts(&s));
        code(&s);
        fclose(ALL.out);
        char* ret =
            "#include <assert.h>\n"
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n"
            "#define MIN(x,y)   (((x) < (y)) ? (x) : (y))\n"
            "#define MAX(x,y)   (((x) > (y)) ? (x) : (y))\n"
            "#define BET(x,y,z) (MIN(x,z)<y && y<MAX(x,z))\n"
            "#define output_Unit_(x) (assert(((long)(x))==1), printf(\"()\"))\n"
            "#define output_Unit(x)  (output_Unit_(x), puts(\"\"))\n"
            "typedef struct {\n"
            "    void* buf;\n"
            "    int max;\n"
            "    int cur;\n"
            "} Pool;\n"
            "void* pool_alloc (Pool* pool, int n) {\n"
            "    if (pool == NULL) {\n"
            "        return malloc(n);\n"
            "    } else {\n"
            "        void* ret = pool->buf + pool->cur;\n"
            "        pool->cur += n;\n"
            "        if (pool->cur <= pool->max) {\n"
            "            return ret;\n"
            "        } else {\n"
            "            return NULL;\n"
            "        }\n"
            "    }\n"
            "}\n"
            "int main (void) {\n"
            "    void* _STACK = &_STACK;\n"
            "\n"
            "struct Bool;\n"
            "typedef struct Bool Bool;\n"
            "auto void output_Bool_ (Bool v);\n"
            "typedef enum {\n"
            "    False,\n"
            "    True\n"
            "} BOOL;\n"
            "\n"
            "struct Bool {\n"
            "    BOOL sub;\n"
            "    union {\n"
            "        int _False;\n"
            "        int _True;\n"
            "    };\n"
            "};\n"
            "\n"
            "void output_Bool_ (Bool v) {\n"
            "    switch (v.sub) {\n"
            "        case False:\n"
            "            printf(\"False\");\n"
            "            ;\n"
            "            printf(\"\");\n"
            "            break;\n"
            "        case True:\n"
            "            printf(\"True\");\n"
            "            ;\n"
            "            printf(\"\");\n"
            "            break;\n"
            "    }\n"
            "}\n"
            "void output_Bool (Bool v) {\n"
            "    output_Bool_(v);\n"
            "    puts(\"\");\n"
            "}\n"
            "\n"
            "}\n";
        assert(!strcmp(out,ret));
    }
}

void t_all (void) {
    // ERROR
    assert(all(
        "(ln 1, col 1): invalid token `/´",
        "//call output()\n"
    ));
    assert(all(
        "(ln 1, col 27): undeclared variable \"x\"",
        "func f: () -> () { return x }\n"
    ));
    assert(all(
        "(ln 1, col 15): undeclared type \"Nat\"",
        "func f: () -> Nat { return () }\n"
    ));
    // UNIT
    assert(all(
        "()\n",
        "call output()\n"
    ));
    assert(all(
        "()\n",
        "val x: () = ()\n"
        "call _output_Unit(x)\n"
    ));
    // NATIVE
    assert(all(
        "A",
        "val x: _char = _65\n"
        "call _putchar(x)\n"
    ));
    assert(all(
        "()\n",
        "call _output_Unit(((),()).1)\n"
    ));
    assert(all(
        "()\n",
        "call _output_Unit(((),((),())).2.1)\n"
    ));
    // OUTPUT
    assert(all(
        "()\n",
        "val x: () = ()\n"
        "call output(x)\n"
    ));
    assert(all(
        "((),())\n",
        "call output((),())\n"
    ));
    // TYPE
    assert(all(
        "False\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = False()\n"
        "call _output_Bool(b)\n"
    ));
    assert(all(
        "Zz1\n",
        "type Zz { Zz1:() }\n"
        "type Yy { Yy1:Zz }\n"
        "type Xx { Xx1:Yy }\n"
        "val x : Xx = Xx1(Yy1(Zz1))\n"
        "call _output_Zz(x.Xx1!.Yy1!)\n"
    ));
    assert(all(
        "Zz1 ((),())\n",
        "type Zz { Zz1:((),()) }\n"
        "val x : Zz = Zz1 ((),())\n"
        "call output(x)\n"
    ));
    assert(all(
        "(ln 1, col 8): undeclared type \"Bool\"",
        "val x: Bool = ()\n"
    ));
    assert(all(
        "Xx1 (Yy1,Zz1)\n",
        "type Zz { Zz1:() }\n"
        "type Yy { Yy1:() }\n"
        "type Xx { Xx1:(Yy,Zz) }\n"
        "val x : Xx = Xx1(Yy1,Zz1)\n"
        "call output(x)\n"
    ));
    // IF
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = False()\n"
        "if b { } else { call output() }\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = True\n"
        "if b { call output() }\n"
    ));
    // PREDICATE
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = True()\n"
        "if b.True? { call output(()) }\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "val b : Bool = True()\n"
        "if b.False? { } else { call output(()) }\n"
    ));
    // FUNC
    assert(all(
        "()\n",
        "func f : () -> () { return arg }\n"
        "call output(f())\n"
    ));
    assert(all(
        "False\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "func inv : (Bool -> Bool) {\n"
        "    if arg.True? {\n"
        "        return False\n"
        "    } else {\n"
        "        return True ()\n"
        "    }\n"
        "}\n"
        "call output(inv(True))\n"
    ));
    // ENV
    assert(all(
        "(ln 1, col 1): expected statement (maybe `call´?) : have \"_output_Unit\"",
        "_output_Unit(x)\n"
    ));
    assert(all(
        "(ln 1, col 13): undeclared variable \"x\"",
        "call output(x)\n"
    ));
    assert(all(
        "(ln 2, col 13): undeclared variable \"x\"",
        "func f : ()->() { val x:()=(); return x }\n"
        "call output(x)\n"
    ));
    assert(all(
        "(ln 2, col 13): undeclared variable \"x\"",
        "if () { val x:()=() }\n"
        "call output(x)\n"
    ));
    assert(all(
        "(ln 1, col 8): undeclared type \"Xx\"",
        "val x: Xx = Xx1\n"
    ));
    // TYPE REC
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "val n: Nat = $Nat\n"
        "call output(n)\n"
    ));
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "val n: Nat = ($Nat,($Nat,$Nat)).1\n"
        "call output(n)\n"
    ));
    assert(all(
        "True\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "val n: Nat = $Nat\n"
        "call _output_Bool(n.$Nat?)\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "val n: Nat = Succ(Succ($Nat))\n"
        "call output(n)\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "val n: Nat = Succ(Succ($Nat))\n"
        "call _output_Nat(n)\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "call _output_Nat(Succ(Succ($Nat)))\n"
    ));
    // POOL
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "call output($Nat)\n"
    ));
    assert(all(
        "Succ ($)\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "call output(Succ($Nat))\n"
    ));
    assert(all(
        "(ln 5, col 13): missing pool for return of \"f\"",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {}\n"
        "call output(f())\n"
    ));
    assert(all(
        "(ln 5, col 6): missing pool for return of \"f\"",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {}\n"
        "call f()\n"
    ));
    assert(all(
        "(ln 5, col 9): invalid pool : data returns",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    val x[]: Nat = $Nat\n"
        "    return x\n"
        "}\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    return Succ(Succ($Nat))\n"
        "}\n"
        "val y[]: Nat = f()\n"
        "call output(y)\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    return Succ(Succ($Nat))\n"
        "}\n"
        "val y[5]: Nat = f()\n"
        "call output(y)\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    val x: Nat = Succ(Succ($Nat))\n"
        "    return x\n"
        "}\n"
        "val y[]: Nat = f()\n"
        "call output(y)\n"
    ));
    assert(all(
        "Succ (Succ (Succ ($)))\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func len: Nat -> Nat {\n"
        "    if arg.$Nat? {\n"
        "        return $Nat\n"
        "    } else {\n"
        "        return Succ(len(arg.Succ!))\n"
        "    }\n"
        "}\n"
        "val x: Nat = Succ(Succ(Succ($Nat)))\n"
        "val y[]: Nat = len(x)\n"
        "call output(y)\n"
    ));
//puts("===============================================================================");
//assert(0);
assert(0 && "OK");
    // TODO: bounded allocation fail
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    return Succ(Succ($Nat))\n"
        "}\n"
        "val y[1]: Nat = f()\n"
        "call output(y)\n"
    ));
}

void t_parser (void) {
    t_parser_type();
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
