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

void code_type (Type tp) {
    switch (tp.sub) {
        case TYPE_NONE:
            assert(0 && "bug found");
        case TYPE_UNIT:
            out("int");
            break;
        case TYPE_NATIVE:
            out(&tp.tk.val.s[1]);
            break;
        case TYPE_TYPE:
            out(tp.tk.val.s);
            break;
    }
}

void code_expr (Expr e) {
    switch (e.sub) {
        case EXPR_NONE:
            assert(0 && "bug found");
        case EXPR_UNIT:
            out("1");
            break;
        case EXPR_NATIVE:
            out(&e.tk.val.s[1]);
            break;
        case EXPR_VAR:
            out(e.tk.val.s);
            break;
        case EXPR_CALL:
            code_expr(*e.Call.func);
            out("(");
            code_expr(*e.Call.arg);
            out(")");
            break;
        case EXPR_CONS:
            // ((Bool) { Bool_False })
            fprintf(ALL.out,
                "((%s) { %s_%s, {} })",
                e.Cons.type.val.s, e.Cons.type.val.s, e.Cons.subtype.val.s);
            break;
        case EXPR_TUPLE: {
            char str[16];
            sprintf(str, "((TUPLE%d)", e.Tuple.size);
            out(str);
            out("{ ");
            for (int i=0; i<e.Tuple.size; i++) {
                //fprintf (ALL.out[OGLOB], "%c _%d=", ((i==0) ? ' ' : ','), i);
                if (i != 0) {
                    out(",");
                }
                out("(void*)");
                code_expr(e.Tuple.vec[i]);
            }
            out(" })");
            break;
        }
        case EXPR_INDEX:
            code_expr(*e.Index.tuple);
            fprintf(ALL.out, "._%d", e.Index.index);
            break;
    }
}

void code_stmt (Stmt s) {
    switch (s.sub) {
        case STMT_VAR:
            code_type(s.Var.type);
            fputs(" ", ALL.out);
            fputs(s.Var.id.val.s, ALL.out);
            fputs(" = ", ALL.out);
            code_expr(s.Var.init);
            out(";\n");
            break;

        case STMT_TYPE: {
            const char* sup = s.Type.id.val.s;
            const char* SUP = strupper(s.Type.id.val.s);

            // ENUM + STRUCT + UNION
            {
                // enum { Bool_False, Bool_True } BOOL;
                out("typedef enum {\n");
                    for (int i=0; i<s.Type.size; i++) {
                        out("    ");
                        out(sup);                       // Bool
                        out("_");
                        out(s.Type.vec[i].id.val.s);    // False
                        if (i < s.Type.size-1) {
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
                for (int i=0; i<s.Type.size; i++) {
                    Sub sub = s.Type.vec[i];
                    out("        ");
                    code_type(sub.type);
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
                // _show_Bool (Bool v)
                fprintf(ALL.out,
                    "void show_%s (%s v) {\n"
                    "    switch (v.sub) {\n",
                    sup, sup
                );
                for (int i=0; i<s.Type.size; i++) {
                    Sub sub = s.Type.vec[i];
                    fprintf(ALL.out,
                        "        case %s_%s:\n"
                        "            printf(\"%s.%s\");\n"
                        "            break;\n",
                        sup, sub.id.val.s, sup, sub.id.val.s
                    );

#if 0
                    if (cons.type.sub != TYPE_UNIT) {
                        out("putchar('(');\n");
                        void aux (Type type, char* arg, int first) {
                            switch (type.sub) {
                                case TYPE_TUPLE:
                                    if (!first) {
                                        out("putchar('(');\n");
                                    }
                                    for (int i=0; i<type.Tuple.size; i++) {
                                        if (i > 0) {
                                            out("printf(\",\");\n");
                                        }
                                        char arg_[256];
                                        sprintf(arg_, "%s._%d", arg, i);
                                        aux(type.Tuple.vec[i], arg_, 0);
                                    }
                                    if (!first) {
                                        out("putchar(')');\n");
                                    }
                                    break;
                                case TYPE_DATA:
                                    fprintf(ALL.out[OGLOB], "_show_%s(%s);\n", type.Data.tk.val.s, arg);
                                    break;
                                default:
                                    out("printf(\"%s\", \"???\");\n");
                            }
                        }
                        char arg_[256];
                        sprintf(arg_, "v%s_%s", (data.isrec?"->":"."), v);
                        aux(cons.type, arg_, 1);
                        out("putchar(')');\n");
                    }
#endif
                }
                out("    }\n");
                out("    puts(\"\");\n");
                out("}\n");
            }

            break;
        }

        case STMT_CALL:
            code_expr(s.call);
            out(";\n");
            break;

        case STMT_SEQ:
            for (int i=0; i<s.Seq.size; i++) {
                code_stmt(s.Seq.vec[i]);
            }
            break;
    }
}

void code (Stmt s) {
    out (
        "#include <assert.h>\n"
        "#include <stdio.h>\n"
        "typedef struct { void *_1, *_2;      } TUPLE2;\n"
        "typedef struct { void *_1, *_2, *_3; } TUPLE3;\n"
        "#define show_Unit(x) (assert(((long)(x))==1), puts(\"()\"))\n"
        "int main (void) {\n"
        "\n"
    );
    code_stmt(s);
    fprintf(ALL.out, "\n");
    out("}\n");
}
