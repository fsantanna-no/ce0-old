#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "all.h"

char* strupper (const char* src) {
    static char dst[256];
    assert(strlen(src) < sizeof(dst));
    for (int i=0; i<strlen(src); i++) {
        dst[i] = toupper(src[i]);
    }
    dst[strlen(src)] = '\0';
    return dst;
}

void out (const char* v) {
    fputs(v, ALL.out);
}

///////////////////////////////////////////////////////////////////////////////

void to_c_ (char* out, Type* tp) {
    switch (tp->sub) {
        case TYPE_UNIT:
            strcat(out, "int");
            break;
        case TYPE_NATIVE:
            strcat(out, &tp->tk.val.s[1]);
            break;
        case TYPE_USER: {
            Stmt* s = env_find_decl(tp->env, tp->tk.val.s);
            if (s!=NULL && s->User.isrec) strcat(out, "struct ");
            strcat(out, tp->tk.val.s);
            if (s!=NULL && s->User.isrec) strcat(out, "*");
            break;
        }
        case TYPE_TUPLE:
            strcat(out, "TUPLE");
            for (int i=0; i<tp->Tuple.size; i++) {
                strcat(out, "__");
                to_c_(out, &tp->Tuple.vec[i]);
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

char* to_c (Type* tp) {
    static char out[256];
    out[0] = '\0';
    to_c_(out, tp);
    return out;
}

void code_to_c (Type* tp) {
    out(to_c(tp));
}

///////////////////////////////////////////////////////////////////////////////

void to_ce_ (char* out, Type* tp) {
    switch (tp->sub) {
        case TYPE_UNIT:
            strcat(out, "Unit");
            break;
        case TYPE_NATIVE:
            strcat(out, &tp->tk.val.s[1]);
            break;
        case TYPE_USER: {
            strcat(out, tp->tk.val.s);
            break;
        }
        case TYPE_TUPLE:
            strcat(out, "TUPLE");
            for (int i=0; i<tp->Tuple.size; i++) {
                strcat(out, "__");
                to_c_(out, &tp->Tuple.vec[i]);
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

char* to_ce (Type* tp) {
    static char out[256];
    out[0] = '\0';
    to_ce_(out, tp);
    return out;
}

void code_to_ce (Type* tp) {
    out(to_ce(tp));
}

///////////////////////////////////////////////////////////////////////////////

// tuple structs pre declarations

void code_tuple_0 (Type* tp) {
    assert(tp->sub == TYPE_TUPLE);

    char tp_c [256];
    char tp_ce[256];
    strcpy(tp_c,  to_c (tp));
    strcpy(tp_ce, to_ce(tp));

    // IFDEF
    out("#ifndef __");
    out(tp_ce);
    out("__\n");
    out("#define __");
    out(tp_ce);
    out("__\n");

    // STRUCT
    out("typedef struct {\n");
    for (int i=0; i<tp->Tuple.size; i++) {
        code_to_c(&tp->Tuple.vec[i]);
        fprintf(ALL.out, " _%d;\n", i+1);
    }
    out("} ");
    out(tp_ce);
    out(";\n");

    // OUTPUT
    fprintf(ALL.out,
        "void output_%s_ (%s v) {\n"
        "    printf(\"(\");\n",
        tp_ce, tp_c
    );
    for (int i=0; i<tp->Tuple.size; i++) {
        if (i > 0) {
            fprintf(ALL.out, "    printf(\",\");\n");
        }
        fprintf(ALL.out, "    output_%s_(v._%d);\n", to_ce(&tp->Tuple.vec[i]), i+1);
    }
    fprintf(ALL.out,
        "    printf(\")\");\n"
        "}\n"
        "void output_%s (%s v) {\n"
        "    output_%s_(v);\n"
        "    puts(\"\");\n"
        "}\n",
        tp_ce, tp_c, tp_ce
    );

    // IFDEF
    out("#endif\n");
}

// cons structs pre allocations

void code_expr_cons_0 (Expr* e) {
    assert(e->sub == EXPR_CONS);

    Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
    assert(user != NULL);

    char* sup = user->User.id.val.s;
    char* sub = e->Cons.sub.val.s;

    // Bool _1 = (Bool) { False, {_False=1} };
    // Nat  _1 = (Nat)  { Succ,  {_Succ=&_2} };
    fprintf(ALL.out,
        "%s _%d = ((%s) { %s, { ._%s=",
        sup, e->N, sup, sub, sub);
    code_expr_1(e->Cons.arg);
    out(" } });\n");
}

void code_expr_0 (Expr* e) {
    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_NIL:
        case EXPR_ARG:
        case EXPR_NATIVE:
        case EXPR_VAR:
            break;
        case EXPR_CALL:
            code_expr_0(e->Call.func);
            code_expr_0(e->Call.arg);
            break;
        case EXPR_CONS:
            code_expr_0(e->Cons.arg);   // child "x2" is used by parent "x1"
            code_expr_cons_0(e);
            break;
        case EXPR_INDEX:
            code_expr_0(e->Index.tuple);
            break;
        case EXPR_DISC:
            code_expr_0(e->Disc.cons);
            break;
        case EXPR_PRED:
            code_expr_0(e->Disc.cons);
            break;

        case EXPR_TUPLE: {
            for (int i=0; i<e->Tuple.size; i++) {
                code_expr_0(&e->Tuple.vec[i]);
            }
            code_tuple_0(env_expr_type(e));
            break;
        }
    }
}

void code_expr_1 (Expr* e) {
    switch (e->sub) {
        case EXPR_UNIT:
            out("1");
            break;
        case EXPR_NIL:
            out("NULL");
            break;
        case EXPR_ARG:
            out("arg");
            break;
        case EXPR_NATIVE:
            out(&e->tk.val.s[1]);
            break;
        case EXPR_VAR:
            out(e->tk.val.s);
            break;

        case EXPR_CALL: {
            if (e->Call.func->sub == EXPR_NATIVE) {
                code_expr_1(e->Call.func);
                out("(");
                code_expr_1(e->Call.arg);
                out(")");
            } else {
                assert(e->Call.func->sub == EXPR_VAR);

                int isrec = 0;

                if (!strcmp(e->Call.func->tk.val.s,"output")) {
                    out("output_");
                    code_to_ce(env_expr_type(e->Call.arg));
                } else {
                    code_expr_1(e->Call.func);

                    // f(x) -> f(pool,x)
                    // f(x) -> (x -> User)
                    Type* tp = env_expr_type(e->Call.func);
                    assert(tp->sub == TYPE_FUNC);
                    if (tp->Func.out->sub == TYPE_USER) {
                        // (x -> User) -> User
                        Stmt* s = env_find_decl(e->env, tp->Func.out->tk.val.s);
                        assert(s!=NULL && s->sub==STMT_USER);
                        isrec = s->User.isrec;
                    }
                }

                out("(");
                if (isrec) {
                    out("_pool, ");
                }
                code_expr_1(e->Call.arg);
                out(")");
            }
            break;
        }

        case EXPR_CONS: {
            Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
            assert(user != NULL);
            fprintf(ALL.out, "%s_%d", (user->User.isrec ? "&" : ""), e->N);
            break;
        }
        case EXPR_TUPLE:
            out("((");
            code_to_c(env_expr_type(e));
            out(") { ");
            for (int i=0; i<e->Tuple.size; i++) {
                //fprintf (ALL.out[OGLOB], "%c _%d=", ((i==0) ? ' ' : ','), i);
                if (i != 0) {
                    out(",");
                }
                code_expr_1(&e->Tuple.vec[i]);
            }
            out(" })");
            break;
        case EXPR_INDEX:
            code_expr_1(e->Index.tuple);
            fprintf(ALL.out, "._%d", e->Index.index);
            break;
        case EXPR_DISC:
            code_expr_1(e->Disc.cons);
            fprintf(ALL.out, "._%s", e->Disc.sub.val.s);
            break;
        case EXPR_PRED: {
            int isnil = (e->Pred.sub.enu == TK_NIL);
            out("((");
            code_expr_1(e->Pred.cons);
            fprintf(ALL.out, "%s == %s) ? (Bool){True,{._True=1}} : (Bool){False,{._False=1}})",
                (isnil ? "" : ".sub"),
                (isnil ? "NULL" : e->Pred.sub.val.s)
            );
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_VAR:
            if (s->Var.pool == 0) {
                // no pool
            } else if (s->Var.pool == -1) {
                out("Pool* _pool = NULL;\n");
            } else {
                fprintf (ALL.out,
                    "Pool _%d;\n"
                    "Pool* _pool = &_%d;\n",
                    s->N, s->N
                );
            }

            code_expr_0(&s->Var.init);
            code_to_c(&s->Var.type);
            fputs(" ", ALL.out);
            fputs(s->Var.id.val.s, ALL.out);
            fputs(" = ", ALL.out);
            code_expr_1(&s->Var.init);
            out(";\n");
            break;

        case STMT_USER: {
            const char* sup = s->User.id.val.s;
            const char* SUP = strupper(s->User.id.val.s);

            // tuple subtypes
            for (int i=0; i<s->User.size; i++) {
                Type* tp = &s->User.vec[i].type;
                if (tp->sub == TYPE_TUPLE) {
                    code_tuple_0(tp);
                }
            }

            // ENUM + STRUCT + UNION
            {
                // enum { False, True } BOOL;
                out("typedef enum {\n");
                    for (int i=0; i<s->User.size; i++) {
                        out("    ");
                        out(s->User.vec[i].id.val.s);    // False
                        if (i < s->User.size-1) {
                            out(",");
                        }
                        out("\n");
                    }
                out("} ");
                out(SUP);
                out(";\n\n");

                // struct { BOOL sub; union { ... } } Bool;
                fprintf(ALL.out,
                    "typedef struct %s {\n"
                    "    %s sub;\n"
                    "    union {\n",
                    sup, SUP
                );
                for (int i=0; i<s->User.size; i++) {
                    Sub sub = s->User.vec[i];
                    out("        ");
                    code_to_c(&sub.type);
                    out(" _");
                    out(sub.id.val.s);      // int _True
                    out(";\n");
                }
                fprintf(ALL.out,
                    "    };\n"
                    "} %s;\n",
                    sup
                );
            }
            out("\n");

            // OUTPUT
            {
                int isrec = s->User.isrec;
                char* op = (isrec ? "->" : ".");

                // _output_Bool (Bool v)
                fprintf(ALL.out,
                    "void output_%s_ (%s%s v) {\n",
                    sup, sup, (isrec ? "*" : "")
                );
                if (isrec) {
                    out (
                        "if (v == NULL) {\n"
                        "    printf(\"Nil\");\n"
                        "    return;\n"
                        "}\n"
                    );
                }
                fprintf(ALL.out, "    switch (v%ssub) {\n", op);
                for (int i=0; i<s->User.size; i++) {
                    Sub* sub = &s->User.vec[i];

                    char arg[1024] = "";
                    int yes = 0;
                    int par = 0;
                    switch (sub->type.sub) {
                        case TYPE_UNIT:
                            yes = par = 0;
                            break;
                        case TYPE_USER:
                            yes = par = 1;
                            sprintf(arg, "output_%s_(v%s_%s)", sub->type.tk.val.s, op, sub->id.val.s);
                            break;
                        case TYPE_TUPLE:
                            yes = 1;
                            par = 0;
                            sprintf(arg, "output_%s_(v%s_%s)", to_ce(&sub->type), op, sub->id.val.s);
                            break;
                        default:
                            assert(0 && "bug found");
                    }

                    fprintf(ALL.out,
                        "        case %s:\n"                  // True
                        "            printf(\"%s%s%s\");\n"   // True (
                        "            %s;\n"                   // ()
                        "            printf(\"%s\");\n"       // )
                        "            break;\n",
                        sub->id.val.s, sub->id.val.s, yes?" ":"", par?"(":"", arg, par?")":""
                    );
                }
                out("    }\n");
                out("}\n");
                fprintf(ALL.out,
                    "void output_%s (%s%s v) {\n"
                    "    output_%s_(v);\n"
                    "    puts(\"\");\n"
                    "}\n",
                    sup, sup, (isrec ? "*" : ""), sup
                );
            }

            break;
        }

        case STMT_CALL:
            code_expr_0(&s->call);
            code_expr_1(&s->call);
            out(";\n");
            break;

        case STMT_SEQ:
            for (int i=0; i<s->Seq.size; i++) {
                code_stmt(&s->Seq.vec[i]);
            }
            break;

        case STMT_IF:
            code_expr_0(&s->If.cond);
            out("if (");
            code_expr_1(&s->If.cond);
            out(".sub) {\n");           // Bool.sub returns 0 or 1
            code_stmt(s->If.true);
            out("} else {\n");
            code_stmt(s->If.false);
            out("}\n");
            break;

        case STMT_FUNC: {
            assert(s->Func.type.sub == TYPE_FUNC);

            // f: a -> User
            // f: (Pool,a) -> User
            int isrec = 0; {
                if (s->Func.type.Func.out->sub == TYPE_USER) {
                    Stmt* decl = env_find_decl(s->env, s->Func.type.Func.out->tk.val.s);
                    assert(decl!=NULL && decl->sub==STMT_USER);
                    isrec = decl->User.isrec;
                }
            }

            char tp_out[256] = "";
            to_c_(tp_out, s->Func.type.Func.out);

            char tp_inp[256] = "";
            to_c_(tp_inp, s->Func.type.Func.inp);

            fprintf (ALL.out,
                "%s %s (%s %s arg) {\n",
                tp_out,
                s->Func.id.val.s,
                (isrec ? "Pool* pool," : ""),
                tp_inp
            );
            code_stmt(s->Func.body);
            out("}\n");
            break;
        }

        case STMT_RETURN:
            code_expr_0(&s->ret);
            out("return ");
            code_expr_1(&s->ret);
            out(";\n");
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////

void code (Stmt* s) {
    out (
        "#include <assert.h>\n"
        "#include <stdio.h>\n"
        "#define output_Unit_(x) (assert(((long)(x))==1), printf(\"()\"))\n"
        "#define output_Unit(x)  (output_Unit_(x), puts(\"\"))\n"
        "typedef struct {\n"
        "    void* buf;\n"  // stack-allocated buffer
        "    int max;\n"    // maximum size
        "    int cur;\n"    // current size
        "} Pool;\n"
        "int main (void) {\n"
        "\n"
    );
    code_stmt(s);
    fprintf(ALL.out, "\n");
    out("}\n");
}
