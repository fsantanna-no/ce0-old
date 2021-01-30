#include <assert.h>
#include <stdio.h>
#include <string.h>

// check visually 3 errors

#define DEBUG
//#define VALGRIND

#include "../all.h"

int all (const char* xp, char* src) {
    static char out[65000];
    Stmt* s;

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

#ifdef DEBUG
    dump_stmt(s);
#endif
    if (!env(s)) {
#ifdef DEBUG
        puts(ALL.err);
#endif
        return !strcmp(ALL.err, xp);
    }
    if (!owner(s)) {
#ifdef DEBUG
        puts(ALL.err);
#endif
        return !strcmp(ALL.err, xp);
    }

    code(s);
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
#ifdef VALGRIND
        FILE* f = popen("valgrind ./a.out", "r");
#else
        FILE* f = popen("./a.out", "r");
#endif
        assert(f != NULL);
        char* cur = out;
        cur[0] = '\0';
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
        all_init(NULL, stropen("r", 0, "xvar var else varx type"));
        assert(ALL.tk1.enu == TX_VAR);
        lexer(); assert(ALL.tk1.enu == TK_VAR);
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
        lexer(); assert(ALL.tk1.enu == TX_VAR); assert(!strcmp(ALL.tk1.val.s, "a")); assert(ALL.tk1.col == 10);
        lexer(); assert(ALL.tk1.enu == TX_USER); assert(!strcmp(ALL.tk1.val.s, "C"));
        lexer(); assert(ALL.tk1.enu == TK_EOF);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "_char _Tp"));
        assert(ALL.tk1.enu == TX_NATIVE);  assert(!strcmp(ALL.tk1.val.s, "char"));
        lexer(); assert(ALL.tk1.enu == TX_NATIVE); assert(!strcmp(ALL.tk1.val.s, "Tp"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "_{(1)} _(2+2)"));
        assert(ALL.tk1.enu == TX_NATIVE); assert(!strcmp(ALL.tk1.val.s, "(1)"));
        lexer(); assert(ALL.tk1.enu == TX_NATIVE); assert(!strcmp(ALL.tk1.val.s, "2+2"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "var xvar varx"));
        assert(ALL.tk1.enu == TK_VAR);
        lexer(); assert(ALL.tk1.enu == TX_VAR); assert(!strcmp(ALL.tk1.val.s, "xvar")); assert(ALL.tk1.col == 5);
        lexer(); assert(ALL.tk1.enu == TX_VAR); assert(!strcmp(ALL.tk1.val.s, "varx"));
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
        Type* tp;
        parser_type(&tp);
        assert(tp->sub == TYPE_NATIVE);
        assert(!strcmp(tp->Native.val.s,"char"));
        fclose(ALL.inp);
    }
    // TYPE_UNIT
    {
        all_init(NULL, stropen("r", 0, "()"));
        Type* tp;
        assert(parser_type(&tp));
        assert(tp->sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    // TYPE_TUPLE
    {
        all_init(NULL, stropen("r", 0, "((),())"));
        Type* tp;
        assert(parser_type(&tp));
        assert(tp->sub == TYPE_TUPLE);
        assert(tp->Tuple.size == 2);
        assert(tp->Tuple.vec[1]->sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "xxx"));
        Type* tp;
        assert(!parser_type(&tp));
        assert(!strcmp(ALL.err, "(ln 1, col 1): expected type : have \"xxx\""));
        fclose(ALL.inp);
    }
    // TYPE_FUNC
    {
        all_init(NULL, stropen("r", 0, "() -> ()"));
        Type* tp;
        parser_type(&tp);
        assert(tp->sub == TYPE_FUNC);
        assert(tp->Func.inp->sub == TYPE_UNIT);
        assert(tp->Func.out->sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
}

void t_parser_expr (void) {
    // UNIT
    {
        all_init(NULL, stropen("r", 0, "()"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    // PARENS
    {
        all_init(NULL, stropen("r", 0, "(())"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "("));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "(x"));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected `,´ : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "( () )"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "(("));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "(\n( \n"));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 3, col 1): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    // EXPR_VAR
    {
        all_init(NULL, stropen("r", 0, "x"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_VAR); assert(!strcmp(e->tk.val.s,"x"));
        fclose(ALL.inp);
    }
    // EXPR_NATIVE
    {
        all_init(NULL, stropen("r", 0, "_x"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_NATIVE); assert(!strcmp(e->tk.val.s,"x"));
        fclose(ALL.inp);
    }
    // EXPR_TUPLE
    {
        all_init(NULL, stropen("r", 0, "((),x,"));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 1, col 7): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "((),)"));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 1, col 5): expected expression : have `)´"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "((),x:"));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 1, col 6): expected `)´ : have `:´"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "((),x,())"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_TUPLE);
        assert(e->Tuple.size == 3);
        assert(e->Tuple.vec[1]->sub == EXPR_VAR && !strcmp(e->Tuple.vec[1]->tk.val.s,"x"));
        assert(e->Tuple.vec[2]->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    // EXPR_CALL
    {
        all_init(NULL, stropen("r", 0, "xxx ()"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_CALL);
        assert(e->Call.func->sub == EXPR_VAR);
        assert(!strcmp(e->Call.func->tk.val.s, "xxx"));
        assert(e->Call.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "call xxx ()"));
        Expr* e;
        assert(parser_expr(&e,1));
        assert(e->sub == EXPR_CALL);
        assert(e->Call.func->sub == EXPR_VAR);
        assert(!strcmp(e->Call.func->tk.val.s, "xxx"));
        assert(e->Call.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "f()\n()\n()")); // f () ~~f [() [() ()]]~~
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_CALL);
        assert(e->Call.func->sub == EXPR_VAR);
        assert(!strcmp(e->Call.func->tk.val.s, "f"));
        assert(e->Call.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    // EXPR_CONS
    {
        all_init(NULL, stropen("r", 0, "True ()"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_CONS);
        assert(!strcmp(e->tk.val.s,"True"));
        assert(e->Cons->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "Zz1 ((),())"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_CONS);
        assert(!strcmp(e->tk.val.s,"Zz1"));
        assert(e->Cons->sub == EXPR_TUPLE);
        fclose(ALL.inp);
    }
    // EXPR_INDEX
    {
        all_init(NULL, stropen("r", 0, "x.1"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_INDEX);
        assert(e->Index.val->sub == EXPR_VAR);
        assert(e->tk.val.n == 1);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "x () .1 ()"));  // x [[() .1] ()]
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_CALL);
        assert(e->Call.func->sub == EXPR_VAR);
        assert(e->Call.arg->sub == EXPR_INDEX);
        //assert(e->Call.arg->Call.func->sub == EXPR_INDEX);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "x().."));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 1, col 5): expected index or subtype : have `.´"));
        fclose(ALL.inp);
    }
    // EXPR_UPREF, EXPR_DNREF
    {
        all_init(NULL, stropen("r", 0, "\\x.1"));
        Expr* e;
        assert(parser_expr(&e,0));
        assert(e->sub == EXPR_UPREF);
        assert(e->Upref->sub == EXPR_INDEX);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "\\1"));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 1, col 1): unexpected operand to `\\´"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "1\\"));
        Expr* e;
        assert(!parser_expr(&e,0));
        assert(!strcmp(ALL.err, "(ln 1, col 2): unexpected operand to `\\´"));
        fclose(ALL.inp);
    }
}

void t_parser_stmt (void) {
    // STMT_VAR
    {
        all_init(NULL, stropen("r", 0, "var :"));
        Stmt* s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 5): expected variable identifier : have `:´"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "var x x"));
        Stmt* s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 7): expected `:´ : have \"x\""));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "var x: x"));
        Stmt* s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 8): expected type : have \"x\""));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "var x: ()"));
        Stmt* s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 10): expected `=´ : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "var x: () = ()"));
        Stmt* s;
        assert(parser_stmt(&s));
        assert(s->sub == STMT_VAR);
        assert(s->Var.tk.enu == TX_VAR);
        assert(s->Var.type->sub == TYPE_UNIT);
        assert(s->Var.init->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "var x: ((),((),())) = ()"));
        Stmt* s;
        assert(parser_stmt(&s));
        assert(s->sub == STMT_VAR);
        assert(s->Var.tk.enu == TX_VAR);
        assert(s->Var.type->sub == TYPE_TUPLE);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "var a : _char = ()"));
        Stmt* s;
        assert(parser_stmt(&s));
        assert(s->Var.type->sub == TYPE_NATIVE);
        fclose(ALL.inp);
    }
    // STMT_USER
    {
        all_init(NULL, stropen("r", 0, "type Bool"));
        Stmt* s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 10): expected `{´ : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "type Bool {}"));
        Stmt* s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 12): expected type identifier : have `}´"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "type Bool { True: ()"));
        Stmt* s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 21): expected `}´ : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "type Bool { False:() ; True:() }"));
        Stmt* s;
        assert(parser_stmt(&s));
        assert(s->sub == STMT_USER);
        assert(!strcmp(s->User.tk.val.s, "Bool"));
        assert(s->User.size == 2);
        assert(s->User.vec[0].type->sub == TYPE_UNIT);
        assert(!strcmp(s->User.vec[1].tk.val.s, "True"));
        fclose(ALL.inp);
    }
    // STMT_CALL
    {
        all_init(NULL, stropen("r", 0, "std()"));
        Stmt* s;
        assert(!parser_stmt(&s));
        assert(!strcmp(ALL.err, "(ln 1, col 1): expected statement : have \"std\""));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "call f()"));
        Stmt* s;
        assert(parser_stmt(&s));
        assert(s->sub == STMT_CALL);
        assert(s->Call->Call.func->sub == EXPR_VAR);
        assert(s->Call->Call.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "call _printf()"));
        Stmt* s;
        assert(parser_stmt(&s));
        assert(s->sub == STMT_CALL);
        assert(s->Call->Call.func->sub == EXPR_NATIVE);
        assert(s->Call->Call.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "call f() ; call g()"));
        Stmt* s;
        assert(parser_stmts(TK_EOF,&s));
        assert(s->sub == STMT_SEQ);
        assert(s->Seq.s1->sub == STMT_CALL);
        assert(s->Seq.s2->sub == STMT_CALL);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "{ call () }"));
        Stmt* s;
        assert(!parser_stmt(&s));
        //assert(!strcmp(ALL.err, "(ln 1, col 8): expected call expression : have `()´"));
        assert(!strcmp(ALL.err, "(ln 1, col 8): unexpected `()´ : wanted variable identifier"));
        fclose(ALL.inp);
    }
    // STMT_IF
    {
        all_init(NULL, stropen("r", 0, "if () { } else { call f() }"));
        Stmt* s;
        assert(parser_stmt(&s));
        assert(s->sub == STMT_IF);
        assert(s->If.tst->sub == EXPR_UNIT);
        assert(s->If.true->sub==STMT_BLOCK && s->If.false->sub==STMT_BLOCK);
        assert(s->If.true->Block->sub==STMT_NONE);
        assert(s->If.false->Block->sub==STMT_CALL);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "if () { call f() } "));
        Stmt* s;
        assert(parser_stmt(&s));
        assert(s->sub==STMT_IF && s->If.false->sub==STMT_BLOCK);
        assert(s->If.false->Block->sub==STMT_NONE);
        fclose(ALL.inp);
    }
    // STMT_FUNC
    {
        all_init(NULL, stropen("r", 0, "func f : () -> () { return }"));
        Stmt* s;
        assert(parser_stmt(&s));
        assert(s->sub == STMT_FUNC);
        assert(s->Func.type->sub == TYPE_FUNC);
        assert(!strcmp(s->Func.tk.val.s, "f"));
        assert(s->Func.body->sub == STMT_BLOCK);
        assert(s->Func.body->Block->sub == STMT_SEQ);
        assert(s->Func.body->Block->Seq.s1->sub == STMT_SEQ);
        assert(s->Func.body->Block->Seq.s2->sub == STMT_BLOCK);
        assert(s->Func.body->Block->Seq.s2->Block->Seq.s1->sub == STMT_SET);
        assert(s->Func.body->Block->Seq.s2->Block->Seq.s2->sub == STMT_RETURN);
        assert(s->Func.body->Block->Seq.s2->Block->Seq.s1->Set.src->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
}

void t_env (void) {
    // ENV_HELD_VARS
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0, "var x: Int = 10")));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->env, s->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 0);
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "var x: Int = 10\n"
            "var y: \\Int = \\x"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 1);
        assert(!strcmp(vars[0]->tk.val.s, "x"));
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "var x: Int = 10\n"
            "var y: Int = 10\n"
            "var z: Int = 10\n"
            "var t: (\\Int,Int,\\Int) = (\\x,y,\\z)"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 2);
        assert(!strcmp(vars[1]->tk.val.s, "z"));
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "var x: Int = 10\n"
            "var y: \\Int = \\x\n"
            "var t: (\\Int,(Int,\\Int)) = (\\x,(x,y))"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 2);
        assert(!strcmp(vars[1]->tk.val.s, "y"));
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "var x: Int = 10\n"
            "var y: \\Int = \\x\n"
            "var t: (\\Int,(Int,\\Int)) = (\\x,(x,y))\n"
            "var p: (Int,\\Int) = t.2"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 1);
        assert(!strcmp(vars[0]->tk.val.s, "t"));
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "var x: Int = 10\n"
            "var y: \\Int = \\x\n"
            "var t: (\\Int,(Int,\\Int)) = (\\x,(x,y))\n"
            "var p: \\Int = t.2.2"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 1);
        assert(!strcmp(vars[0]->tk.val.s, "t"));
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "func f: \\Int -> \\Int { return arg }\n"
            "var x: Int = 10\n"
            "var p: \\Int = f(\\x)"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 1);
        assert(!strcmp(vars[0]->tk.val.s, "x"));
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "func f: \\Int -> \\Int { return arg }\n"
            "func g: \\Int -> Int { return arg\\ }\n"
            "var x: Int = 10\n"
            "var y: Int = 10\n"
            "var p: (\\Int,Int) = (f(\\x),g(\\y))"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 1);
        assert(!strcmp(vars[0]->tk.val.s, "x"));
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "type Xx { Xx1: (\\Int,Int) }\n"
            "func f: \\Int -> \\Int { return arg }\n"
            "func g: \\Int -> Int { return arg\\ }\n"
            "var x: Int = 10\n"
            "var y: Int = 10\n"
            "var p: Xx = Xx1 (f(\\x),g(\\y))"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 1);
        assert(!strcmp(vars[0]->tk.val.s, "x"));
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "type Xx { Xx1: (Int,\\Int) }\n"
            "var x: Int = 10\n"
            "var y: \\Int = \\x\n"
            "var t: (\\Int,(Int,\\Int)) = (\\x,(x,y))\n"
            "var m: Xx = Xx1 t.2\n"
            "var p: \\Int = m.Xx1!.2"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256]; int uprefs[256];
        env_held_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars, uprefs);
        assert(n == 1);
        assert(!strcmp(vars[0]->tk.val.s, "m"));
    }

    // ENV_TXED_VARS
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0, "var x: Int = 10")));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256];
        env_txed_vars(s->Seq.s2->env, s->Seq.s2->Var.init, &n, vars);
        assert(n == 0);
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "var x: Int = 10\n"
            "var y: Int = x\n"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256];
        env_txed_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars);
        assert(n == 0);
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "type rec Nat { Succ: Nat }\n"
            "var x: Nat = $Nat\n"
            "var y: Nat = x\n"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256];
        env_txed_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars);
        assert(n == 1);
        assert(!strcmp(vars[0]->tk.val.s, "x"));
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "type rec Nat { Succ: Nat }\n"
            "var x: Nat = $Nat\n"
            "var y: \\Nat = \\x\n"
            "var z: Nat = y\\\n"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256];
        env_txed_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars);
        assert(n == 1);
        assert(vars[0]->sub==EXPR_DNREF);
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "type rec Nat { Succ: Nat }\n"
            "var x: Nat = $Nat\n"
            "var y: \\Nat = \\x\n"
            "var z: (Nat,\\Nat,Nat) = (x,y,y\\)\n"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256];
        env_txed_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars);
        assert(n == 2);
        assert(!strcmp(vars[0]->tk.val.s, "x"));
        assert(vars[1]->sub==EXPR_DNREF);
    }
    {
        Stmt* s;
        assert(all_init(NULL, stropen("r", 0,
            "type rec Nat { Succ: Nat }\n"
            "type Xx { Xx1: Nat }\n"
            "var x: Nat = $Nat\n"
            "var y: \\Nat = \\x\n"
            "var z: (Nat,\\Nat,Xx) = (x,y,Xx1 y\\)\n"
        )));
        assert(parser(&s));
        assert(env(s));
        int n=0; Expr* vars[256];
        //dump_stmt(s->Seq.s2->Seq.s2);
        env_txed_vars(s->Seq.s2->Seq.s2->env, s->Seq.s2->Seq.s2->Var.init, &n, vars);
        assert(n == 2);
        assert(!strcmp(vars[0]->tk.val.s, "x"));
        assert(vars[1]->sub==EXPR_DNREF);
    }
}

void t_code (void) {
    // EXPR_UNIT
    {
        char out[256] = "";
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr e = { ALL.nn++, EXPR_UNIT, {TK_UNIT,{},0,0} };
        code_expr(NULL, &e, 0);
        fclose(ALL.out);
        assert(!strcmp(out,""));
    }
    // EXPR_VAR
    {
        char out[256] = "";
        all_init(stropen("w",sizeof(out),out), NULL);
        Type tp = { TYPE_UNIT, 0 };
        Stmt var = { ALL.nn++, STMT_VAR, .Var={{TX_VAR,{.s="xxx"},0,0},&tp,NULL} };
        Env env = { &var, NULL };
        Expr e = { ALL.nn++, EXPR_VAR, {TX_VAR,{.s="xxx"},0,0},.Var={0,0} };
        code_expr(&env, &e, 0);
        fclose(ALL.out);
        assert(!strcmp(out,""));
    }
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Type tp = { TYPE_NATIVE, 0 };
        Stmt var = { ALL.nn++, STMT_VAR, .Var={{TX_VAR,{.s="xxx"},0,0},&tp,NULL} };
        Env env = { &var, NULL };
        Expr e = { ALL.nn++, EXPR_VAR, {TX_VAR,{.s="xxx"},0,0},.Var={0,0} };
        code_expr(&env, &e, 0);
        fclose(ALL.out);
        assert(!strcmp(out,"xxx"));
    }
    // EXPR_NATIVE
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr e = { ALL.nn++, EXPR_NATIVE, {TX_NATIVE,{},0,0} };
            strcpy(e.tk.val.s, "printf");
        code_expr(NULL, &e, 0);
        fclose(ALL.out);
        assert(!strcmp(out,"printf"));
    }
    // EXPR_TUPLE
    {
        char out[256];
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr unit = { ALL.nn++, EXPR_UNIT, {TK_UNIT,{},0,0} };
        Expr* es[2] = {&unit, &unit};
        Expr e = { ALL.nn++, EXPR_TUPLE, .Tuple={2,es} };
        code_expr(NULL, &e, 0);
        fclose(ALL.out);
        //assert(!strcmp(out,"((TUPLE__Unit__Unit) {  })"));
        assert(!strcmp(out,"_tmp_1"));
    }
    // EXPR_INDEX
    {
        char out[256];
        Type tp = { TYPE_NATIVE, 0 };
        Stmt var = { ALL.nn++, STMT_VAR, .Var={{TX_VAR,{.s="x"},0,0},&tp,NULL} };
        Env env = { &var, NULL };
        all_init(stropen("w",sizeof(out),out), NULL);
        Expr val = { ALL.nn++, EXPR_VAR, {TX_VAR,{.s="x"},0,0}, .Var={0,0} };
        Expr e = { ALL.nn++, EXPR_INDEX, {TX_NUM,{2},0,0}, .Index={&val,0} };
        code_expr(&env, &e, 0);
        fclose(ALL.out);
        assert(!strcmp(out,"x._2"));
    }
    {
        char out[8192] = "";
        all_init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "var a : () = () ; call _stdo a")
        );
        Stmt s = { STMT_NONE };
        code(&s);
        fclose(ALL.out);
        char* xp =
            "#include <assert.h>\n"
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n"
            "typedef int Int;\n"
            "#define stdout_Unit_() printf(\"()\")\n"
            "#define stdout_Unit()  (stdout_Unit_(), puts(\"\"))\n"
            "#define stdout_Int_(x) printf(\"%d\",x)\n"
            "#define stdout_Int(x)  (stdout_Int_(x), puts(\"\"))\n"
            "void* move (void* x) { return x; }\n"
            "int main (void) {\n"
            "\n"
            "\n"
            "}\n";
        assert(!strcmp(out,xp));
    }
    {
        char out[8192] = "";
        all_init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "var a : () = () ; call _stdo a")
        );
        Stmt* s;
        assert(parser_stmts(TK_EOF,&s));
        assert(s->sub == STMT_SEQ);
        assert(s->Seq.s1->sub == STMT_VAR);
        assert(s->Seq.s2->sub == STMT_CALL);
        assert(env(s));
        code(s);
        fclose(ALL.out);
        char* ret =
            "#include <assert.h>\n"
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n"
            "typedef int Int;\n"
            "#define stdout_Unit_() printf(\"()\")\n"
            "#define stdout_Unit()  (stdout_Unit_(), puts(\"\"))\n"
            "#define stdout_Int_(x) printf(\"%d\",x)\n"
            "#define stdout_Int(x)  (stdout_Int_(x), puts(\"\"))\n"
            "void* move (void* x) { return x; }\n"
            "int main (void) {\n"
            "\n"
            "stdo();\n"
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
        Stmt* s;
        assert(parser_stmts(TK_EOF,&s));
        code(s);
        fclose(ALL.out);
        char* ret =
            "#include <assert.h>\n"
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n"
            "typedef int Int;\n"
            "#define stdout_Unit_() printf(\"()\")\n"
            "#define stdout_Unit()  (stdout_Unit_(), puts(\"\"))\n"
            "#define stdout_Int_(x) printf(\"%d\",x)\n"
            "#define stdout_Int(x)  (stdout_Int_(x), puts(\"\"))\n"
            "void* move (void* x) { return x; }\n"
            "int main (void) {\n"
            "\n"
            "struct Bool;\n"
            "typedef struct Bool Bool;\n"
            "auto void stdout_Bool_ (Bool v);\n"
            "\n"
            "typedef enum {\n"
            "    False,\n"
            "    True\n"
            "} BOOL;\n"
            "\n"
            "struct Bool {\n"
            "    BOOL sub;\n"
            "    union {\n"
            "    };\n"
            "};\n"
            "\n"
            "\n"
            "void stdout_Bool_ (Bool v) {\n"
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
            "void stdout_Bool (Bool v) {\n"
            "    stdout_Bool_(v);\n"
            "    puts(\"\");\n"
            "}\n"
            "\n"
            "}\n";
        assert(!strcmp(out,ret));
    }
}

void t_all (void) {
goto __XXX__;
__XXX__:
    // ERROR
    assert(all(
        "(ln 1, col 1): expected statement : have \"/\"",
        "//call std()\n"
    ));
    assert(all(
        "(ln 1, col 27): undeclared variable \"x\"",
        "func f: () -> () { return x }\n"
    ));
    assert(all(
        "(ln 1, col 9): undeclared type \"Nat\"",
        "func f: Nat -> () { return () }\n"
    ));
    // UNIT
    assert(all(
        "()\n",
        "output std()\n"
    ));
    assert(all(
        "()\n",
        "var x: () = ()\n"
        "output std x\n"
    ));
    assert(all(
        "10\n",
        "var x: Int = 10\n"
        "output std x\n"
    ));
    // NATIVE
    assert(all(
        "(ln 4, col 1): expected statement : have \"x\"",
        "native _{\n"
        "   putchar('A');\n"
        "}\n"
        "x\n"
    ));
    assert(all(
        "(ln 1, col 8): expected `_{...}´ : have `{´",
        "native {\n"
        "   putchar('A');\n"
        "}\n"
    ));
    assert(all(
        "A",
        "native _{\n"
        "   putchar('A');\n"
        "}\n"
    ));
    assert(all(
        "1\n",
        "var x: Int = _abs(1)\n"
        "output std x\n"
    ));
    assert(all(
        "1\n",
        "call _stdout_Int _abs(1)\n"
    ));
    assert(all(
        "A",
        "var x: _char = _65\n"
        "call _putchar x\n"
    ));
    assert(all(
        "()\n",
        "var x: ((),()) = ((),())\n"
        "var y: () = x.1\n"
        "call _stdout_Unit y\n"
    ));
    assert(all(
        "()\n",
        "output std (((),()).1)\n"
    ));
    assert(all(
        "()\n",
        "var v: ((),()) = ((),())\n"
        "var x: ((),((),())) = ((),v)\n"
        "var y: ((),()) = x.2\n"
        "var z: () = y.2\n"
        "call _stdout_Unit z\n"
    ));
    assert(all(
        "()\n",
        "var v: ((),()) = ((),())\n"
        "var x: ((),((),())) = ((),v)\n"
        "var y: ((),()) = x.2\n"
        "var z: () = y.2\n"
        "call _stdout_Unit z\n"
    ));
    assert(all(
        "a",
        "native _{\n"
        "   void f (void) { putchar('a'); }\n"
        "}\n"
        "call _f ()\n"
    ));
    assert(all(
        "hello\n",
        "var y: _(char*) = _(\"hello\")\n"
        "var x: (Int,_(char*)) = (10,y)\n"
        "call _puts x.2\n"
    ));
    // OUTPUT
    assert(all(
        "(ln 2, col 8): unexpected `()´ : wanted variable identifier",
        "var x: () = ()\n"
        "output () x\n"
    ));
    assert(all(
        "()\n",
        "var x: () = ()\n"
        "output std x\n"
    ));
    assert(all(
        "((),())\n",
        "var x: ((),()) = ((),())\n"
        "output std x\n"
    ));
    assert(all(
        "10\n",
        "func output_f: Int -> () { output std arg }\n"
        "output f 10\n"
    ));
    // TYPE
    assert(all(
        "False\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = False()\n"
        "output std b\n"
    ));
    assert(all(
        "Zz1\n",
        "type Zz { Zz1:() }\n"
        "type Yy { Yy1:Zz }\n"
        "type Xx { Xx1:Yy }\n"
        "var z : Zz = Zz1()\n"
        "var y : Yy = Yy1 z\n"
        "var x : Xx = Xx1 y\n"
        "var yy: Yy = x.Xx1!\n"
        "var zz: Zz = yy.Yy1!\n"
        "output std zz\n"
    ));
    assert(all(
        "(ln 6, col 12): undeclared subtype \"Set_\"",
        "type Set_ {\n"
        "    Size:     (_int,_int,_int,_int)\n"
        "    Color_BG: _int\n"
        "    Color_FG: _int\n"
        "}\n"
        "output std(Set_(_1,_2,_3,_4))\n"
    ));
    assert(all(
        "Size (?,?,?,?)\n",
        "type Set_ {\n"
        "    Size:     (_int,_int,_int,_int)\n"
        "    Color_BG: _int\n"
        "    Color_FG: _int\n"
        "}\n"
        "var x: _int = _1\n"
        "var y: _int = _2\n"
        "var w: _int = _3\n"
        "var z: _int = _4\n"
        "output std(Size (x,y,w,x))\n"
    ));
    assert(all(
        "Zz1 ((),())\n",
        "type Zz { Zz1:((),()) }\n"
        "var a : ((),()) = ((),())\n"
        "var x : Zz = Zz1 a\n"
        "output std x\n"
    ));
    assert(all(
        "(ln 1, col 8): undeclared type \"Bool\"",
        "var x: Bool = ()\n"
    ));
    assert(all(
        "Xx1 (Yy1,Zz1)\n",
        "type Zz { Zz1:() }\n"
        "type Yy { Yy1:() }\n"
        "type Xx { Xx1:(Yy,Zz) }\n"
        "var y: Yy = Yy1 ()\n"
        "var z: Zz = Zz1()\n"
        "var a: (Yy,Zz) = (y,z)\n"
        "var x: Xx = Xx1 a\n"
        "output std x\n"
    ));
    assert(all(
        "Xx1 (Yy1,Zz1)\n",
        "type Zz { Zz1:() }\n"
        "type Yy { Yy1:() }\n"
        "type Xx { Xx1:(Yy,Zz) }\n"
        "output std (Xx1 (Yy1,Zz1))\n"
    ));
    assert(all(
        "True\n",
        "type Bool { False: () ; True: () }\n"
        "type Input {\n"
        "    Event: ()\n"
        "}\n"
        "var x: Input = Event ()\n"
        "var x_: \\Input = \\x\n"
        "var y: Bool = x_\\.Event?\n"
        "output std y\n"
    ));
    assert(all(
        "(ln 7, col 18): invalid `.´ : expected user type",
        "type Bool { False: () ; True: () }\n"
        "type Input {\n"
        "    Event: ()\n"
        "}\n"
        "var x: Input = Event ()\n"
        "var x_: \\Input = \\x\n"
        "var y: Bool = x_.Event?\n"
        "output std y\n"
    ));
    assert(all(
        "True\n",
        "type Bool { False: () ; True: () }\n"
        "type Event_ {\n"
        "    Any: ()\n"
        "}\n"
        "type Input {\n"
        "    Event: \\Event_\n"
        "}\n"
        "var e: Event_ = Any\n"
        "var x: Input = Event \\e\n"
        "var y: Bool = x.Event!\\.Any?\n"
        "output std y\n"
    ));
    assert(all(
        "True\n",
        "type Bool { False: () ; True: () }\n"
        "type Event_ {\n"
        "    Any: ()\n"
        "}\n"
        "type Input {\n"
        "    Event: (\\Event_,())\n"
        "}\n"
        "var e: Event_ = Any\n"
        "var x: Input = Event (\\e,())\n"
        "var y: Bool = x.Event!.1\\.Any?\n"
        "output std y\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False:() ; True:() }\n"
        "type rec Aa {\n"
        "    Aa1: ()\n"
        "}\n"
        "type Xx {\n"
        "    Xx1: ()\n"
        "}\n"
        "var ret: Aa = Aa1 ()\n"
        "if True {\n"
        "    var y: () = ()\n"
        "    var x: Xx = Xx1 y\n"
        "}\n"
        "output std ()\n"
    ));

    // ALIAS / POINTER

    assert(all(
        "(ln 3, col 5): invalid assignment to \"y\" : type mismatch",
        "var x: Int = 10\n"
        "var y: \\Int = \\x\n"
        "set y = 10\n"
        "output std x\n"
    ));
    assert(all(
        "10\n",
        "var x: Int = 10\n"
        "var y: \\Int = \\x\n"
        "set y\\ = 10\n"
        "output std x\n"
    ));
    assert(all(
        "True\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "var x: Bool = False\n"
        "var y: \\Bool = \\x\n"
        "set y\\ = True\n"
        "output std x\n"
    ));

    // TUPLE
    assert(all(
        "((1,2),(3,4))\n",
        "var x: ((Int,Int),(Int,Int)) = ((1,2),(3,4))\n"
        "output std x\n"
    ));
    assert(all(
        "(1,2)\n",
        "var x0: Int = 1\n"
        "var tup: (Int,Int) = (x0,2)\n"
        "output std tup\n"
    ));

    assert(all(
        "(1,2)\n",
        "var x: (Int,Int) = (1,2)\n"
        "var y: \\(Int,Int) = \\x\n"
        "output std y\\\n"
    ));
    assert(all(
        "1\n",
        "type Tp {\n"
        "   Sub: \\(Int,Int)\n"
        "}\n"
        "output std 1\n"
    ));
    assert(all(
        "(ln 2, col 15): invalid `.´ : expected tuple type",
        "var x: Int = 1\n"
        "var y: () = x.1\n"
    ));
    assert(all(
        "(ln 2, col 15): invalid `.´ : index is out of range",
        "var x: (Int,Int) = (1,2)\n"
        "var y: () = x.3\n"
    ));
    assert(all(
        "(ln 2, col 5): invalid assignment to \"y\" : type mismatch",
        "var x: (Int,Int) = (1,2)\n"
        "var y: () = x.2\n"
    ));

    // IF
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = False()\n"
        "if b { } else { output std() }\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "if False { } else { output std() }\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = True()\n"
        "if b { output std() }\n"
    ));
    // PREDICATE
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = True()\n"
        "var tst: Bool = b.True?\n"
        "if tst { output std () }\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = True()\n"
        "var tst: Bool = b.False?\n"
        "if tst {} else { output std() }\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = True()\n"
        "if b.False? {} else { output std() }\n"
    ));
    assert(all(
        "(ln 1, col 18): undeclared subtype \"Event\"",
        "call _check _arg.Event?\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = True()\n"
        "var c : \\Bool = \\b\n"
        "if c\\.False? {} else { output std() }\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = True()\n"
        "var c : \\Bool = \\b\n"
        "var d : () = c\\.True!\n"
        "output std d\n"
    ));
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = True()\n"
        "var c : \\Bool = \\b\n"
        "output std c\\.True!\n"
    ));
    // DISCRIMINATOR
    assert(all(
        "()\n",
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = True()\n"
        "var u : () = b.True!\n"
        "output std u\n"
    ));
    assert(all(
        "",     // ERROR
        "type Bool { False: () ; True: () }\n"
        "var b : Bool = True()\n"
        "var u : () = b.False!\n"
        "output std ()\n"
    ));
    // FUNC
    assert(all(
        //"(ln 2, col 6): invalid call to \"f\" : missing return assignment",
        //"",     // ERROR
        "()\n",
        "func f : () -> _int { return _1 }\n"
        "call f ()\n"
        "output std ()\n"
    ));
    assert(all(
        "1\n",
        "func f : _int -> _int { return arg }\n"
        "var v: Int = f 1\n"
        "output std v\n"
    ));
    assert(all(
        "()\n",
        "func f : () -> () { return arg }\n"
        "var v: () = f()\n"
        "output std v\n"
    ));
    assert(all(
        "(ln 1, col 26): invalid return : type mismatch",
        "func f : ((),()) -> () { return arg }\n"
        "call f()\n"
    ));
    assert(all(
        "(ln 2, col 6): invalid call to \"f\" : type mismatch",
        "func f : ((),()) -> () { return }\n"
        "call f()\n"
    ));
    assert(all(
        "False\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "func inv : Bool -> Bool {\n"
        "    var tst: Bool = arg.True?\n"
        "    if tst {\n"
        "        var v: Bool = False()\n"
        "        return v\n"
        "    } else {\n"
        "        var v: Bool = True ()\n"
        "        return v\n"
        "    }\n"
        "}\n"
        "var a: Bool = True()\n"
        "var b: Bool = inv a\n"
        "output std b\n"
    ));
    assert(all(
        "2\n",
        "func f : Int -> Int {\n"
        "   set arg = _(arg+1)\n"
        "   return arg\n"
        "}\n"
        "output std f 1\n"
    ));
    assert(all(
        "2\n",
        "func f : \\Int -> () {\n"
        "   set arg\\ = _(*arg+1)\n"
        "   return \n"
        "}\n"
        "var x: Int = 1\n"
        "call f (\\x)\n"
        "output std x\n"
    ));

    // arg.1 > arg.2
    assert(all(
        //"(ln 3, col 4): invalid assignment : cannot hold pointer \"arg\" (ln 2) in outer scope",
        "1\n",
        "type Tp { Tp1: \\Int }\n"
        "func f : (\\Tp,\\Int) -> () {\n"
        "   set arg.1\\.Tp1! = arg.2\n"
        "   return \n"
        "}\n"
        "output std 1\n"
    ));
    assert(all(
        "(ln 6, col 24): invalid tuple : pointers with different scopes",
        //"(ln 3, col 4): invalid assignment : cannot hold pointer \"arg\" (ln 2) in outer scope",
        "type Tp { Tp1: \\Int }\n"
        "var i: Int = 10\n"
        "var tp: Tp = Tp1 \\i\n"
        "{\n"
        "   var j: Int = 100\n"
        "   var v: (\\Tp,\\Int) = (\\tp,\\j)\n"
        "   set v.1\\.Tp1! = v.2\n"
        "}\n"
        "{\n"
        "   var k: Int = 0\n"
        "}\n"
        "output std tp.Tp1!\\\n"
    ));
    assert(all(
        "(ln 10, col 11): invalid tuple : pointers with different scopes",
        //"10\n",
        "type Tp { Tp1: \\Int }\n"
        "func f : (\\Int,\\Tp) -> () {\n"
        "   set arg.2\\.Tp1! = arg.1\n"
        "   return \n"
        "}\n"
        "var i: Int = 10\n"
        "var j: Int = 0\n"
        "{\n"
        "   var tp: Tp = Tp1 \\j\n"
        "   call f (\\i,\\tp)\n"
        "   output std tp.Tp1!\\\n"
        "}\n"
    ));
    assert(all(
        "(ln 12, col 12): invalid tuple : pointers with different scopes",
        "type Tp {\n"
        "    Tp1: \\Int\n"
        "}\n"
        "func f : (\\Int,\\Tp) -> () {  -- 2nd argument (possibly pointing to wider scope)\n"
        "    set arg.2\\.Tp1! = arg.1   -- holds 1st argument (possibly pointing to narrower scope)\n"
        "    return                     -- this leads to a dangling reference\n"
        "}\n"
        "var i: Int = 10\n"
        "var tp: Tp = Tp1 \\i           -- wider scope\n"
        "{\n"
        "    var j: Int = 0             -- narrower scope\n"
        "    call f (\\j,\\tp)          -- ERROR: passing pointers with different scopes\n"
        "}\n"
        "output std tp.Tp1!\\           -- use of dangling reference\n"
    ));
    assert(all(
        "$\n",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var lst: List = Item $List\n"
        "{\n"
        "    var cur: \\List = \\lst\n"
        "    set cur = \\cur\\.Item!\n"
        "    output std cur\n"
        "}\n"
    ));
    assert(all(
        "(ln 1, col 1): invalid return : no enclosing function",
        "return \n"
    ));

    // list / tail
    assert(all(
        "",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "func new: () -> List {\n"
        "    return Item $List\n"
        "}\n"
    ));
#if TODO-HASREC
    assert(all(
        "LT_Type ($,@)\n",
        "type rec List {\n"
        "    Item: (Int, List)\n"
        "}\n"
        "\n"
        "type List_Tail {\n"
        "    LT_Type: (List, \\List)\n"
        "}\n"
        "\n"
        "func new_list_tail: () -> List_Tail {\n"
        "    var l: List = $List\n"
        "    var null: \\List = _NULL\n"
        "    var ret: List_Tail = LT_Type (move l, null)\n"
        "    set ret.LT_Type!.2 = \\ret.LT_Type!.1\n"
        "    return move ret\n"
        "}\n"
        "var l: List_Tail = call new_list_tail ()\n"
        "output std (\\l)\n"
    ));
    assert(all(
        "LT_Type ($,@)\n",
        "type rec List {\n"
        "    Item: (Int, List)\n"
        "}\n"
        "\n"
        "type List_Tail {\n"
        "    LT_Type: (List, \\List)\n"
        "}\n"
        "\n"
        "func new_list_tail: () -> List_Tail {\n"
        "    var l: List = $List\n"
        "    var null: \\List = _NULL\n"
        "    var ret: List_Tail = LT_Type (move l, null)\n"
        "    set ret.LT_Type!.2 = \\ret.LT_Type!.1\n"
        "    output std 111\n"
        "    output std (ret.LT_Type!.2)\n"
        "    return move ret\n"
        "}\n"
        "func append_list_tail: (\\List_Tail,Int) -> () {\n"
        "   --output std (\\arg.1\\.LT_Type!.1)\n"
        "   --output std (arg.1\\.LT_Type!.2)\n"
        "   set arg.1\\.LT_Type!.2\\ = Item (arg.2, $List)\n"
        "   --output std (\\arg.1\\.LT_Type!.1)\n"
        //"   set arg.1\\.LT_Type!.2 = \\arg.1\\.LT_Type!.2\\.Item!.2\n"
        "}\n"
        "var l: List_Tail = call new_list_tail ()\n"
        "output std 222\n"
        "output std (l.LT_Type!.2)\n"
        "call append_list_tail(\\l, 10)\n"
        //"call append_list_tail(\\l, 20)\n"
        "--output std (\\l)\n"
    ));
#endif

    assert(all(
        "10\n",
        "var x: Int = 10\n"
        "var px: \\Int = \\x\n"
        "{\n"
        "   var p: \\Int = px\n"
        "   set px = p\n"
        "   output std p\\\n"
        "}\n"
    ));
    assert(all(
        "10\n",
        "var x: Int = 10\n"
        "var px: \\Int = \\x\n"
        "{\n"
        "   var px2: \\Int = px\n"
        "   var p: \\Int = px2\n"
        "   set px = p\n"
        "   output std p\\\n"
        "}\n"
    ));
    assert(all(
        "(ln 7, col 4): invalid assignment : cannot hold pointer \"y\" (ln 4) in outer scope",
        "var x: Int = 10\n"
        "var px: \\Int = \\x\n"
        "{\n"
        "   var y: Int = 100\n"
        "   var py: \\Int = \\y\n"
        "   var p: \\Int = py\n"
        "   set px = p\n"
        "   output std p\\\n"
        "}\n"
    ));
    assert(all(
        "10\n",
        "func f: (\\Int,\\Int) -> () {\n"
        "    var e: \\Int = arg.2\n"
        "    call f (arg.1, e)\n"
        "    return ()\n"
        "}\n"
        "output std 10\n"
    ));

    assert(all(
        "True\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "func f : \\Bool -> () {\n"
        "   set arg = arg\n"
        "   return \n"
        "}\n"
        "var x: Bool = True\n"
        "call f (\\x)\n"
        "output std x\n"
    ));
    assert(all(
        "True\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "func not: Bool -> Bool {\n"
        "    if arg {\n"
        "        return False\n"
        "    } else {\n"
        "        return True\n"
        "    }\n"
        "}\n"
        "func f : \\Bool -> () {\n"
        "   set arg\\ = True\n"
        "   return ()\n"
        "}\n"
        "var x: Bool = False\n"
        "call f (\\x)\n"
        "output std x\n"
    ));
    assert(all(
        "True\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "func pre g: Bool -> Bool\n"
        "func f: Bool -> Bool {\n"
        "   return g arg\n"
        "}\n"
        "func g: Bool -> Bool {\n"
        "    if arg {\n"
        "        return False\n"
        "    } else {\n"
        "        return True\n"
        "    }\n"
        "}\n"
        "var x: Bool = call f (False)\n"
        "output std x\n"
    ));
    // ENV
    assert(all(
        "(ln 1, col 1): expected statement : have \"_stdout_Unit\"",
        "_stdout_Unit(x)\n"
    ));
    assert(all(
        "(ln 1, col 12): undeclared variable \"x\"",
        "output std x\n"
    ));
    assert(all(
        "(ln 2, col 12): undeclared variable \"x\"",
        "func f : ()->() { var x:()=(); return x }\n"
        "output std x\n"
    ));
    assert(all(
        "(ln 2, col 12): undeclared variable \"x\"",
        "if () { var x:()=() }\n"
        "output std x\n"
    ));
    assert(all(
        "(ln 1, col 8): undeclared type \"Xx\"",
        "var x: Xx = ()\n"
    ));
    // TYPE REC
    assert(all(
        "(ln 2, col 15): undeclared subtype \"Item\"",
        "type List { Cons:() }\n"
        "var l: List = Item ()\n"
        "output std l\n"
    ));
    assert(all(
        "(ln 1, col 12): undeclared type \"List\"",
        "output std $List\n"
    ));
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var n: Nat = $Nat\n"
        "var n_: \\Nat = \\n\n"
        "output std n_\n"
    ));
    assert(all(
        "Succ ($)\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var n: Nat = Succ $Nat\n"
        "var n_: \\Nat = \\n\n"
        "output std n_\n"
    ));
    assert(all(
        "Aa1 (Bb1 (Aa1 ($)))\n",
        "type pre Bb\n"
        "type rec Aa {\n"
        "   Aa1: Bb\n"
        "}\n"
        "type rec Bb {\n"
        "   Bb1: Aa\n"
        "}\n"
        "var n: Aa = Aa1 Bb1 Aa1 $Bb\n"
        "var n_: \\Aa = \\n\n"
        "output std n_\n"
    ));
    assert(all(
        "(ln 1, col 19): unexpected `move´ call : no ownership transfer",
        "var n: Int = move 1\n"
    ));
    assert(all(
        "XNat1 (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "type XNat {\n"
        "   XNat1: Nat\n"
        "}\n"
        "var n: Nat = Succ $Nat\n"
        "var x: XNat = XNat1 move n\n"
        "var x_: \\XNat = \\x\n"
        "output std x_\n"
    ));
    assert(all(
        "Succ ($)\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var c: Nat = Succ $Nat\n"
        "output std (\\c)\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var c: Nat = Succ Succ $Nat\n"
        "output std (\\c)\n"
    ));
    assert(all(
        "Succ ($)\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var c: Nat = Succ Succ $Nat\n"
        "output std (\\c.Succ!)\n"
    ));
    assert(all(
        "Succ ($)\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var d: Nat = Succ $Nat\n"
        "var c: Nat = Succ move d\n"
        "var b: \\Nat = \\c.Succ!\n"
        "output std b\n"
    ));
    assert(all(
        "(Succ ($),@)\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var d: Nat = Succ $Nat\n"
        "var c: Nat = $Nat\n"
        "var b: \\Nat = \\c\n"
        "var a: (Nat,\\Nat) = (move d,b)\n" // precisa desalocar o d
        "var a_: \\(Nat,\\Nat) = \\a\n"
        "output std a_\n"
    ));
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var a: (Nat,Nat) = ($Nat,$Nat)\n"
        "var b: \\Nat = \\a.2\n"
        "output std b\n"
    ));
    assert(all(
        "(ln 6, col 14): invalid access to \"c\" : ownership was transferred (ln 5)",
        //"$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var c: Nat = Succ $Nat\n"
        "var a: (Nat,Nat) = ($Nat, move c)\n" // precisa transferir o c
        //"var d: Nat = c\n"               // erro
        "output std (\\c)\n"
    ));
    assert(all(
        "(ln 7, col 16): invalid access to \"a\" : ownership was transferred (ln 6)",
        //"$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var c: Nat = Succ $Nat\n"
        "var a: (Nat,Nat) = ($Nat,move c)\n"
        "var b: (Nat,Nat) = move a\n"        // tx a
        "var d: \\Nat = \\a.2\n"          // no: a is txed
        "output std d\n"
    ));
    assert(all(
        //"$\n",
        "(ln 7, col 16): invalid access to \"a\" : ownership was transferred (ln 6)",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var c: Nat = Succ $Nat\n"
        "var a: (Nat,Nat) = ($Nat,move c)\n"
        "var b: Nat = move a.2\n"            // zera a.2 (a se mantem sem tx)
        "var d: \\Nat = \\a.2\n"
        "output std d\n"                 // ok: $Nat
    ));
    assert(all(
        //"Succ ($)\n",
// TODO-NO-MOVE
        "(ln 8, col 16): invalid access to \"a\" : ownership was transferred (ln 7)",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var c: Nat = Succ $Nat\n"
        "var e: Nat = Succ $Nat\n"
        "var a: (Nat,Nat) = move (move e,move c)\n"
        "var b: Nat = move a.2\n"
        "var d: \\Nat = \\a.1\n"
        "output std d\n"             // ok: e
    ));
    assert(all(
        "(ln 7, col 19): invalid transfer of \"c\" : active pointer in scope (ln 6)",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var d: Nat = Succ $Nat\n"
        "var c: Nat = Succ move d\n"
        "var b: \\Nat = \\c.Succ!\n"      // borrow de c
        "var e: Nat = move c\n"              // erro
    ));
    assert(all(
        "Succ ($)\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var c: Nat = Succ $Nat\n"
        "{\n"
        "   var b: \\Nat = \\c.Succ!\n"
        "   {\n"
        "   }\n"
        "}\n"
        //"output std 10\n"
        "var e: Nat = move c\n"
        "output std (\\e)\n"
    ));
    assert(all(
        //"Succ ($)\n",
        //"(ln 7, col 16): invalid access to \"c\" : ownership was transferred (ln 6)",
        "(ln 6, col 21): invalid ownership transfer : mode `growable´ only allows root transfers",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var d: Nat = Succ $Nat\n"
        "var c: Nat = Succ move d\n"
        "var b: Nat = move c.Succ!\n"
        "var e: \\Nat = \\c\n"
        "output std e\n"
    ));
    assert(all(
        //"XNat1 ($)\n",
        //"(ln 11, col 17): invalid access to \"i\" : ownership was transferred (ln 10)",
        "(ln 10, col 21): invalid ownership transfer : mode `growable´ only allows root transfers",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "type XNat {\n"
        "   XNat1: Nat\n"
        "}\n"
        "var d: Nat = Succ $Nat\n"
        "var c: Nat = Succ move d\n"
        "var i: XNat = XNat1 move c\n"
        "var j: Nat = move i.XNat1!\n"
        "var k: \\XNat = \\i\n"
        "output std k\n"
    ));
    assert(all(
        //"XNat1 (Succ (Succ ($)))\n",
        "(ln 14, col 22): invalid ownership transfer : mode `growable´ only allows root transfers",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "type XNat {\n"
        "   XNat1: Nat\n"
        "}\n"
        "type YNat {\n"
        "   YNat1: XNat\n"
        "}\n"
        "var d: Nat = Succ $Nat\n"
        "var c: Nat = Succ move d\n"
        "var i: XNat = XNat1 move c\n"
        "var j: YNat = YNat1 move i\n"
        "var k: XNat = move j.YNat1!\n"
        "var k_: \\XNat = \\k\n"
        "output std k_\n"
    ));
// TODO-NO-MOVE
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var a: (Nat,Nat) = ($Nat,$Nat)\n"
        "var c: Nat = move a.1\n"   // can I move a.1 from root a???
        "output std (\\c)\n"
    ));
    assert(all(
        "($,$)\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var a: (Nat,Nat) = ($Nat,$Nat)\n"
        "var c: (Nat,Nat) = move a\n"
        "output std (\\c)\n"
    ));
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var a: (Nat,Nat) = ($Nat,$Nat)\n"
        "var b: (Nat,(Nat,Nat)) = ($Nat,move a)\n"
        "var c: Nat = move b.1\n"
        "var c_: \\Nat = \\c\n"
        "output std c_\n"
    ));
    assert(all(
        "(ln 9, col 17): undeclared type \"Naty\"",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var n: Nat = $Nat\n"
        "var tst: () = n.$Naty?\n"
    ));
    assert(all(
        "()\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var n: Nat = $Nat\n"
        "output std n.$Nat!\n"
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
        "var n: Nat = $Nat\n"
        "var tst: Bool = n.$Nat?\n"
        "output std tst\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var a: Nat = Succ $Nat\n"
        "var n: Nat = Succ move a\n"
        "var n_: \\Nat = \\n\n"
        "output std n_\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var a: Nat = Succ $Nat\n"
        "var n: Nat = Succ move a\n"
        "var n_: \\Nat = \\n\n"
        "output std n_\n"
    ));
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var x: Nat = $Nat\n"
        "var x_: \\Nat = \\x\n"
        "output std x_\n"
    ));
    assert(all(
        "Succ ($)\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var n: Nat = Succ $Nat\n"
        "var n_: \\Nat = \\n\n"
        "output std n_\n"
    ));
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    var x: Nat = $Nat\n"
        "    return move x\n"
        "}\n"
        "var v: Nat = f()\n"
        "var v_: \\Nat = \\v\n"
        "output std v_\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    var a: Nat = Succ $Nat\n"
        "    var b: Nat = Succ move a\n"
        "    return move b\n"
        "}\n"
        "var y: Nat = f()\n"
        "var y_: \\Nat = \\y\n"
        "output std y_\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    var a: Nat = Succ $Nat\n"
        "    var b: Nat = Succ move a\n"
        "    return move b\n"
        "}\n"
        "var y: Nat = f()\n"
        "var y_: \\Nat = \\y\n"
        "output std y_\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    var a: Nat = Succ $Nat\n"
        "    var b: Nat = Succ move a\n"
        "    return move b\n"
        "}\n"
        "var y: Nat = f()\n"
        "var y_: \\Nat = \\y\n"
        "output std y_\n"
    ));
    assert(all(
        "Succ (Succ ($))\n",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {\n"
        "    var a: Nat = Succ $Nat\n"
        "    var b: Nat = Succ move a\n"
        "    return move b\n"
        "}\n"
        "var y: Nat = f()\n"
        "var y_: \\Nat = \\y\n"
        "output std y_\n"
    ));
    assert(all(
        "$\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "func len: Nat -> Nat {\n"
        "    return $Nat\n"
        "}\n"
        "var a: Nat = Succ $Nat\n"
        "var y: Nat = len move a\n"
        "var y_: \\Nat = \\y\n"
        "output std y_\n"
    ));
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "var n: Nat = $Nat\n"
        "output std (\\n)\n"
        "var z: Nat = move n\n"
    ));
    assert(all(
        "",     // ERROR
        "type Bool { False:() ; True:() }\n"
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var n: Nat = $Nat\n"
        "var n_: \\Nat = \\n\n"
        "output std n_\\.$Nat?\n"
        "output std n_\\.Succ!.$Nat?\n"
    ));
    assert(all(
        "Succ (Succ (Succ ($)))\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "func len: \\Nat -> Nat {\n"
        "    if arg\\.$Nat? {\n"
        "        return $Nat\n"
        "    } else {\n"
        "        var n: Nat = len (\\arg\\.Succ!)\n"
        "        return Succ move n\n"
        "    }\n"
        "}\n"
        "var x: Nat = Succ Succ Succ $Nat\n"
        "var y: Nat = len (\\x)\n"
        "output std (\\y)\n"
    ));
    assert(all(
        //"Succ (Succ ($))\n",
        "(ln 5, col 21): invalid ownership transfer : mode `growable´ only allows root transfers",
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "var x: Nat = Succ Succ Succ $Nat\n"
        "var y: Nat = move x.Succ!\n"
        "output std (\\y)\n"
    ));
    assert(all(
        //"Succ (Succ (Succ ($)))\n",
        "(ln 12, col 34): invalid ownership transfer : mode `growable´ only allows root transfers",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "func len: Nat -> Nat {\n"
        "    if arg.$Nat? {\n"
        "        return $Nat\n"
        "    } else {\n"
        "        return Succ len move arg.Succ!\n"
        "    }\n"
        "}\n"
        "var x: Nat = Succ Succ Succ $Nat\n"
        "var y: Nat = len move x\n"
        "output std (\\y)\n"
    ));
    assert(all(
        "Item (1,Item (2,$))\n",
        "type rec List {\n"
        "    Item: (Int,List)\n"
        "}\n"
        "var x: List = Item (1, Item (2, $List))\n"
        "output std (\\x)\n"
    ));
    assert(all(
        "10\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Type {\n"
        "    Type_Any: Bool\n"
        "}\n"
        "output std 10\n"
    ));

    // OWNERSHIP / BORROWING
    assert(all(
        "(ln 6, col 19): invalid access to \"i\" : ownership was transferred (ln 5)",
        //"$\n",
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "var i: Nat = Succ $Nat\n"
        "var j: Nat = move i  -- tx\n"
        "var k: Nat = move i  -- erro\n"
        "output std (\\i)\n"
    ));
    assert(all(
        "$\n",
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "var x: Nat = $Nat   -- owner\n"
        "var z: \\Nat = \\x    -- borrow\n"
        "output std z\n"
    ));
    assert(all(
        "(ln 6, col 19): invalid transfer of \"x\" : active pointer in scope (ln 5)",
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "var x: Nat = $Nat   -- owner\n"
        "var z: \\Nat = \\x    -- borrow\n"
        "var y: Nat = move x      -- error: transfer while borrow is active\n"
    ));
    assert(all(
        "Succ ($)\n",
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "var x: Nat = Succ $Nat    -- owner\n"
        "{\n"
        "    var z: \\Nat = \\x -- borrow\n"
        "}\n"
        "var y: Nat = move x       -- ok: borrow is inactive\n"
        "var y_: \\Nat = \\y\n"
        "output std y_\n"
    ));
    assert(all(
        "(ln 8, col 19): invalid access to \"x\" : ownership was transferred (ln 6)",
        //"$\n",
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "var x: Nat = Succ $Nat     -- owner\n"
        "{\n"
        "    var z: Nat = move x         -- tx: new owner\n"
        "}\n"
        "var y: Nat = move x             -- no: x->z\n"
        "var y_: \\Nat = \\y\n"
        "output std y_\n"
    ));
    assert(all(
        "(ln 8, col 5): invalid return : cannot return local pointer \"l\" (ln 5)",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "func f: () -> \\List {\n"
        "    var l: List = $List   -- `l` is the owner\n"
        "    var a: \\List = \\l\n"
        "    var b: \\List = a\n"
        "    return b             -- error: cannot return pointer to deallocated value\n"
        "}\n"
    ));
    assert(all(
        //"Item ($)\n",
        //"(ln 6, col 20): invalid access to \"a\" : ownership was transferred (ln 5)",
        "(ln 5, col 28): invalid ownership transfer : mode `growable´ only allows root transfers",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var a: List = Item Item $List\n"
        "var b: List = move a.Item!.Item!\n"
        "var c: List = move a.Item!\n"
        "output std (\\c)\n"
    ));
    assert(all(
        "Item (Item ($))\n",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var l: List = Item $List\n"
        "var t: \\List = \\l.Item!\n"
        "set t\\ = Item $List\n"
        "set t = \\t\\.Item!\n"
        "output std (\\l)\n"
    ));
    assert(all(
        //"(ln 8, col 13): invalid transfer of \"l\" : active pointer in scope (ln 7)",
        "()\n",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        //"native _{ typedef void* PTR; }\n"
        //"func asptr: _PTR -> _PTR { return arg }\n"
        "func f: List -> () {}\n"
        "var x: List = Item $List\n"
        "var ptr: \\List = _NULL\n"
        "var l: (List,\\List) = (move x, ptr)\n"
        "set l.2 = \\l.1.Item!\n"
        "call f move l.1\n"
        "output std ()\n"
    ));
    assert(all(
        "",     // ERROR
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var x: List = Item $List\n"
        "var ptr: \\List = \\x\n"
        "set ptr\\ = Item $List\n"
        "output std ()\n"
    ));
    assert(all(
        "(ln 6, col 34): invalid transfer of \"x\" : active pointer in scope (ln 6)",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "func f: List -> () {}\n"
        "var x: List = Item $List\n"
        "var l: (List,\\List) = move (move x, \\x)\n"
        "call f move l.1\n"
    ));
    assert(all(
        //"(ln 7, col 13): invalid transfer of \"l\" : active pointer in scope (ln 6)",
        "()\n",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "func g: (List,\\List) -> () {}\n"
        "var ptr: \\List = _(NULL)\n"
        "var l: (List,\\List) = (Item $List, ptr)\n"
        "set l.2 = \\l.1.Item!\n"
        "call g move l\n"
        "output std ()\n"
    ));
    assert(all(
        "Succ ($)\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "func add: (Nat,\\Nat) -> Nat {\n"
        "        var x: Nat = move arg.1\n"
        "        return move x\n"
        "}\n"
        "var x: Nat = Succ $Nat\n"
        "var y: Nat = $Nat\n"
        "var y_: \\Nat = \\y\n"
        "var a: (Nat,\\Nat) = (move x,y_)\n"
        "var z: Nat = add move a\n"
        "var z_: \\Nat = \\z\n"
        "output std z_\n"
    ));
    assert(all(
        //"Succ ($)\n",
        "(ln 12, col 31): invalid ownership transfer : mode `growable´ only allows root transfers",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "type XNat {\n"
        "    XNat1: Nat\n"
        "}\n"
        "func add: XNat -> Nat {\n"
        "        var x: Nat = move arg.XNat1!\n"
        "        return move x\n"
        "}\n"
        "var x: Nat = Succ $Nat\n"
        "var a: XNat = XNat1 move x\n"
        "var z: Nat = add move a\n"
        "var z_: \\Nat = \\z\n"
        "output std z_\n"
    ));

    // CLONE
    assert(all(
        "True\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "var x: Bool = True ()\n"
        "var y: Bool = clone x\n"
        "output std y\n"
    ));
    assert(all(
        "Succ ($)\n",
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "var x: Nat = Succ $Nat\n"
        "var x_: \\Nat = \\x\n"
        "var y: Nat = clone x_\n"
        "var y_: \\Nat = \\y\n"
        "output std y_\n"
    ));

    // EXPR_UNK
    assert(all(
        "10\n",
        "var x: Int = ?\n"
        "set x = 10\n"
        "output std x\n"
    ));

    // INT
    assert(all(
        "-10\n",
        "var x: Int = -10\n"
        "output std x\n"
    ));

    // SET
    assert(all(
        "20\n",
        "var v: Int = 10\n"
        "set v = 20\n"
        "output std v\n"
    ));
    assert(all(
        "Item (Item ($))\n",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var l1: List = Item Item $List\n"
        "var l2: List = Item $List\n"
        "set l2 = move l1\n"
        "output std (\\l2)\n"
    ));
    assert(all(
        "(ln 10, col 18): invalid access to \"l1\" : ownership was transferred (ln 9)",
        //"$\n",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "type Xx {\n"
        "    Xx1: \\List\n"
        "}\n"
        "var l1: List = $List\n"
        "var l2: List = $List\n"
        "set l2 = move l1\n"
        "var x: Xx = Xx1 \\l1\n"
        "output std x.Xx1!\n"
    ));
    assert(all(
        "(ln 7, col 14): invalid access to \"l1\" : ownership was transferred (ln 6)",
        //"$\n",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var l1: List = $List\n"
        "var l2: List = $List\n"
        "set l2 = move l1\n"
        "output std (\\l1)\n"
    ));
    assert(all(
        "(ln 8, col 5): invalid assignment : cannot hold pointer \"l2\" (ln 7) in outer scope",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var l1: List = $List\n"
        "var r: \\List = \\l1\n"
        "{\n"
        "    var l2: List = $List\n"
        "    set r = \\l2  -- error\n"
        "}\n"
    ));

    // POINTER
    assert(all(
        "10\n",
        "var x: \\Int = ?\n"
        "var y: Int = 10\n"
        "{\n"
        "    set x = \\y\n"
        "}\n"
        "output std (x\\)\n"
    ));
    assert(all(
        "(ln 4, col 5): invalid assignment : cannot hold pointer \"z\" (ln 3) in outer scope",
        "var x: \\Int = ?\n"
        "{\n"
        "    var z: Int = 10\n"
        "    set x = \\z\n"
        "}\n"
        "output std (x\\)\n"
    ));
    assert(all(
        "(ln 4, col 5): invalid assignment : cannot hold pointer \"y\" (ln 3) in outer scope",
        "var x: (Int,\\Int) = (10,?)\n"
        "{\n"
        "    var y: Int = 10\n"
        "    set x.2 = \\y\n"
        "}\n"
        "output std x.2\\\n"
    ));
    assert(all(
        "10\n",
        "var y: Int = 10\n"
        "var x: (Int,\\Int) = (10,\\y)\n"
        "{\n"
        "    set x.2 = \\y\n"
        "}\n"
        "output std x.2\\\n"
    ));
    assert(all(
        "(ln 4, col 5): invalid assignment : cannot hold pointer \"y\" (ln 3) in outer scope",
        "var x: (Int,\\Int) = (10,?)\n"
        "{\n"
        "    var y: Int = 10\n"
        "    set x = (10,\\y)\n"
        "}\n"
        "output std x.2\\\n"
    ));
    assert(all(
        "(ln 4, col 5): invalid assignment : cannot hold pointer \"y\" (ln 3) in outer scope",
        "var x: (Int,\\Int) = (10,?)\n"
        "{\n"
        "    var y: (Int,\\Int) = (10,?)\n"
        "    set x = (10,y.2)\n"
        "}\n"
        "output std x.2\\\n"
    ));
    assert(all(
        "(ln 4, col 5): invalid assignment : cannot hold pointer \"y\" (ln 3) in outer scope",
        "var x: (Int,\\Int) = (10,?)\n"
        "{\n"
        "    var y: (Int,Int) = (10,10)\n"
        "    set x = (10,\\y.2)\n"
        "}\n"
        "output std x.2\\\n"
    ));
    assert(all(
        "(ln 7, col 5): invalid assignment : cannot hold pointer \"y\" (ln 6) in outer scope",
        "type Xx {\n"
        "    Xx1: (Int,\\Int)\n"
        "}\n"
        "var x: Xx = ?\n"
        "{\n"
        "    var y: Int = 10\n"
        "    set x = Xx1(10,\\y)\n"
        "}\n"
    ));
    assert(all(
        "(ln 7, col 5): invalid assignment : cannot hold pointer \"y\" (ln 6) in outer scope",
        "type Xx {\n"
        "    Xx1: (Int,\\Int)\n"
        "}\n"
        "var x: Xx = ?\n"
        "{\n"
        "    var y: Xx = Xx1 (10,?)\n"
        "    set x = Xx1(10,y.Xx1!.2)\n"
        "}\n"
    ));
    assert(all(
        "(ln 7, col 5): invalid assignment : cannot hold pointer \"y\" (ln 6) in outer scope",
        "type Xx {\n"
        "    Xx1: (Int,\\Int)\n"
        "}\n"
        "var x: Xx = ?\n"
        "{\n"
        "    var y: Xx = Xx1 (10,?)\n"
        "    set x = Xx1(10,\\y.Xx1!.1)\n"
        "}\n"
    ));
    assert(all(
        "(ln 5, col 5): invalid assignment : cannot hold pointer \"z\" (ln 4) in outer scope",
        "var x: \\Int = ?\n"
        "func f: \\Int -> \\Int { return arg }\n"
        "{\n"
        "    var z: Int = 10\n"
        "    set x = call f(\\z)\n"
        "}\n"
        "output std (x\\)\n"
    ));
    assert(all(
        "1\n10\n",
        "var x: (\\Int,Int) = ?\n"
        "func f: \\Int -> Int { return arg\\ }\n"
        "var y: Int = 1\n"
        "{\n"
        "    var z: Int = 10\n"
        "    set x = (\\y, f(\\z))\n"
        "}\n"
        "output std x.1\\\n"
        "output std x.2\n"
    ));
    assert(all(
        "(ln 5, col 19): invalid dnref : cannot transfer value",
        "type rec Nat { Succ: Nat }\n"
        "var x: Nat = $Nat\n"
        "var y: \\Nat = \\x\n"
        "var z: \\Nat = y\n"
        "var p: Nat = move z\\\n"
    ));

    // CYCLE
    assert(all(
        "Item (Item ($))\n",            // ok: not cycle
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var l: List = Item $List\n"
        "var m: List = Item $List\n"
        "var p: \\List = \\l.Item!\n"
        "set p\\ = move m\n"
        "output std (\\l)\n"
    ));
    assert(all(
        "(ln 5, col 20): invalid assignment : cannot transfer ownsership to itself",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var l: List = Item $List\n"
        "set l.Item! = move l\n"
    ));
    assert(all(
        "(ln 6, col 15): invalid assignment : cannot transfer ownsership to itself",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "var l: List = Item $List\n"
        "var m: \\List = \\l.Item!\n"
        "set m\\ = move l\n"
    ));
    assert(all(
        "(ln 7, col 17): invalid assignment : cannot transfer ownsership to itself",
        "type rec List {\n"
        "    Item: List\n"
        "}\n"
        "func f: List->List { return move arg }\n"
        "var l: List = Item $List\n"
        "var m: \\List = \\l.Item!\n"
        "set m\\ = f move l\n"
    ));
    assert(all(
        "1\n2\n",
        "{\n"
        "   output std 1\n"
        "   output std 2\n"
        "}\n"
    ));
    assert(all(
        "(ln 6, col 39): invalid dnref : cannot transfer value",
        "type rec List {\n"
        "    Item: ((Int,Int),List)\n"
        "}\n"
        "var l: List = Item ((10,0),$List)\n"
        "var m: \\List = \\l.Item!.2\n"
        "{ set m\\.Item!.2 = Item ((10,10),move m\\) ; output std 10 }\n"
    ));

    // LOOP
    assert(all(
        "20\n",
        "loop {\n"
        "   break\n"
        "}\n"
        "output std 20\n"
    ));
    assert(all(
        "15\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "\n"
        "func bool: Int -> Bool {\n"
        "    return _(arg ? (Bool){True} : (Bool){False})\n"
        "}\n"
        "var i: Int = 1\n"
        "var n: Int = 0\n"
        "loop {\n"
        "    set n = _(n + i)\n"
        "    set i = _(i + 1)\n"
        "    if bool _(i > 5) {\n"
        "        break\n"
        "    }\n"
        "}\n"
        "output std n    -- shows 10\n"
    ));

#if TODO-deep_ptr
    assert(all(
        "10\n",
        "var x: Int = 10\n"
        "func f: () -> \\Int { return \\x }\n"
        "output std (f ()) \\\n"
    ));
#endif

#if TODO-what
    assert(all(
        "((1,2),(3,4))\n",
        "output std ((1,2),(3,4))\n"
    ));
    assert(all(
        "(ln 7, col 14): invalid transfer of \"c\" : active pointer in scope (ln 6)",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "var d: Nat = Succ $Nat\n"
        "var c: Nat = Succ d\n"
        "var b: \\Nat = ?\n"
        "{\n"
        "   set b = \\c.Succ!\n"      // borrow de c
        "}\n"
        "var e: Nat = c\n"              // erro
    ));
#endif

#if TODO-as
    assert(all(
        "a",
        "var x: ((Int,Int),_int) = ((1,1),_1 as _int)\n"
        "output std x.1\n"
    ));
    assert(all(
        "a",
        "var x: ((Int,Int),_{char*}) = ((1,1),_(\"oi\") as _(char*))\n"
        "output std x.1\n"
    ));
#endif

#if TODO-anon-tuples // infer from destiny
    assert(all(
        "Zz1\n",
        "type Set_ {\n"
        "    Size:     (_int,_int,_int,_int)\n"
        "    Color_BG: _int\n"
        "    Color_FG: _int\n"
        "}\n"
        "output std(Size(_1,_2,_3,_4))\n"
    ));
#endif

#if TODO-clone-tuples
    assert(all(
        "(Succ ($), False)\n",
        "type Bool {\n"
        "    False: ()\n"
        "    True:  ()\n"
        "}\n"
        "type rec Nat {\n"
        "    Succ: Nat\n"
        "}\n"
        "var x: Nat  = Succ $Nat\n"
        "var y: Bool = False ()\n"
        "var xy: (Nat,Bool) = (x,y)\n"
        "var z: (Nat,Bool) = clone xy\n"
        "var z_: \\(Nat,Bool) = \\z\n"
        "output std z_\n"
    ));
#endif

#if TODO-POOL
    // POOL
    assert(all(
        "(ln 4, col 23): missing pool for constructor",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "call _show_Nat(Succ(Succ($Nat)))\n"
    ));
    assert(all(
        "(ln 5, col 13): missing pool for call",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {}\n"
        "output std(f())\n"
    ));
    assert(all(
        "(ln 5, col 6): missing pool for return of \"f\"",
        "type rec Nat {\n"
        "   Succ: Nat\n"
        "}\n"
        "func f: () -> Nat {}\n"
        "call f()\n"
    ));
#endif
}

void t_parser (void) {
    t_parser_type();
    t_parser_expr();
    t_parser_stmt();
}

int main (void) {
    t_lexer();
    t_parser();
    t_env();
    t_code();
    t_all();
    puts("OK");
}
