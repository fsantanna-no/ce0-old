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

void code_type__ (char* out, Type* tp) {
    switch (tp->sub) {
        case TYPE_NONE:
            assert(0 && "bug found");
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
                code_type__(out, &tp->Tuple.vec[i]);
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

char* code_type_ (Type* tp) {
    static char out[256];
    out[0] = '\0';
    code_type__(out, tp);
    return out;
}

void code_type (Type* tp) {
    out(code_type_(tp));
}

///////////////////////////////////////////////////////////////////////////////

void code_expr_0 (Expr* e) {
    switch (e->sub) {
        case EXPR_NONE:
            assert(0 && "bug found");
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
        case EXPR_CONS: {
            code_expr_0(e->Cons.arg);   // child "x2" is used by parent "x1"

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
            break;
        }
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

            Type* tp  = env_expr_type(e);
            char tp_[256];
            strcpy(tp_, code_type_(tp));
            out("#ifndef __");
            out(tp_);
            out("__\n");
            out("#define __");
            out(tp_);
            out("__\n");
            out("typedef struct {\n");
            for (int i=0; i<tp->Tuple.size; i++) {
                code_type(&tp->Tuple.vec[i]);
                fprintf(ALL.out, " _%d;\n", i+1);
            }
            out("} ");
            out(tp_);
            out(";\n");
            out("#endif\n");
            break;
        }
    }
}

void code_expr_1 (Expr* e) {
    switch (e->sub) {
        case EXPR_NONE:
            assert(0 && "bug found");
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
        case EXPR_CALL:
            code_expr_1(e->Call.func);
            out("(");
            code_expr_1(e->Call.arg);
            out(")");
            break;
        case EXPR_CONS: {
            Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
            assert(user != NULL);
            fprintf(ALL.out, "%s_%d", (user->User.isrec ? "&" : ""), e->N);
            break;
        }
        case EXPR_TUPLE:
            out("((");
            code_type(env_expr_type(e));
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
        case STMT_NONE:
            assert(0 && "bug found");
            break;

        case STMT_VAR:
            code_expr_0(&s->Var.init);
            code_type(&s->Var.type);
            fputs(" ", ALL.out);
            fputs(s->Var.id.val.s, ALL.out);
            fputs(" = ", ALL.out);
            code_expr_1(&s->Var.init);
            out(";\n");
            break;

        case STMT_USER: {
            const char* sup = s->User.id.val.s;
            const char* SUP = strupper(s->User.id.val.s);

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
                    code_type(&sub.type);
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

            // SHOW
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
                            sprintf(arg, "output_%s_(v%s_%s)", sub->type.tk.val.s, op, sub->id.val.s);
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

        case STMT_FUNC:
            assert(s->Func.type.sub == TYPE_FUNC);
            code_type(s->Func.type.Func.out);
            out(" ");
            out(s->Func.id.val.s);
            out(" (");
            code_type(s->Func.type.Func.inp);
            out(" arg) {\n");
            code_stmt(s->Func.body);
            out("}\n");
            break;

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
        "#define output_Unit(x) (output_Unit_(x), puts(\"\"))\n"
        "int main (void) {\n"
        "\n"
    );
    code_stmt(s);
    fprintf(ALL.out, "\n");
    out("}\n");
}
