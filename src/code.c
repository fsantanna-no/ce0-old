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
        case TYPE_UNIT:
            out("int");
        case TYPE_NATIVE:
            out(&tp.tk.val.s[1]);
    }
}

void code_expr (Expr e) {
    switch (e.sub) {
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
