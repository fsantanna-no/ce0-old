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

void to_ce_ (char* out, Type* tp) {
    switch (tp->sub) {
        case TYPE_UNIT:
            strcat(out, "Unit");
            break;
        case TYPE_NATIVE:
            strcat(out, &tp->Nat.val.s[1]);
            break;
        case TYPE_USER: {
            strcat(out, tp->User.val.s);
            break;
        }
        case TYPE_TUPLE:
            strcat(out, "TUPLE");
            for (int i=0; i<tp->Tuple.size; i++) {
                strcat(out, "__");
                to_ce_(out, &tp->Tuple.vec[i]);
            }
            break;
        case TYPE_FUNC:
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

void to_c_ (char* out, Type* tp) {
    switch (tp->sub) {
        case TYPE_UNIT:
            strcat(out, "int");
            break;
        case TYPE_NATIVE:
            strcat(out, &tp->Nat.val.s[1]);
            break;
        case TYPE_USER: {
            Stmt* s = env_id_to_stmt(tp->env, tp->User.val.s, NULL);
            if (s!=NULL && s->User.isrec) strcat(out, "struct ");
            strcat(out, tp->User.val.s);
            if (s!=NULL && s->User.isrec) strcat(out, "*");
            break;
        }
        case TYPE_TUPLE:
            strcat(out, "TUPLE");
            for (int i=0; i<tp->Tuple.size; i++) {
                strcat(out, "__");
                to_ce_(out, &tp->Tuple.vec[i]);
            }
            break;
        case TYPE_FUNC:
            assert(0 && "TODO");
    }
}

char* to_c (Type* tp) {
    static char out[256];
    out[0] = '\0';
    to_c_(out, tp);
    return out;
}

///////////////////////////////////////////////////////////////////////////////

int ft (Type* tp) {
    if (tp->sub == TYPE_TUPLE) {
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
            out(to_c(&tp->Tuple.vec[i]));
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
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int fe_1 (Expr* e) {
    switch (e->sub) {
        case EXPR_UNIT:
            out("1");
            break;
        case EXPR_ARG:
            out("arg");
            break;
        case EXPR_NATIVE:
            out(&e->Nat.val.s[1]);
            break;
        case EXPR_VAR:
            if (e->Var.istx) {
                out("_");
            }
            out(e->Var.id.val.s);
            if (e->Var.istx) {
                fprintf(ALL.out, "_%d", e->N);
            }
            break;
        case EXPR_ALIAS:
            // same as e->alias
            break;
        case EXPR_CONS: {
            if (e->Cons.subtype.enu == TX_NIL) {
                out("NULL");
            } else {
                fprintf(ALL.out, "_%d", e->N);
            }
            return 0;   // do not generate arg (already generated at fe_0)
        }

        case EXPR_CALL: {
            if (e->Call.func->sub == EXPR_NATIVE) {
                visit_expr(e->Call.func, fe_1);
                out("(");
                visit_expr(e->Call.arg, fe_1);
                out(")");
            } else {
                assert(e->Call.func->sub == EXPR_VAR);

                if (!strcmp(e->Call.func->Var.id.val.s,"output")) {
                    out("output_");
                    code_to_ce(env_expr_to_type(e->Call.arg));
                } else {
                    visit_expr(e->Call.func, fe_1);
                }

                out("(");
                visit_expr(e->Call.arg, fe_1);
                out(")");
            }
            return 0;
        }

        case EXPR_TUPLE:
            out("((");
            out(to_c(env_expr_to_type(e)));
            out(") { ");
            for (int i=0; i<e->Tuple.size; i++) {
                //fprintf (ALL.out[OGLOB], "%c _%d=", ((i==0) ? ' ' : ','), i);
                if (i != 0) {
                    out(",");
                }
                visit_expr(&e->Tuple.vec[i], fe_1);
            }
            out(" })");
            return 0;

        case EXPR_INDEX:
            visit_expr(e->Index.tuple, fe_1);
            fprintf(ALL.out, "._%d", e->Index.index);
            return 0;

        case EXPR_DISC: {
            Stmt* s = env_expr_to_type_to_user_stmt(e->Disc.val);
            assert(s != NULL);
            visit_expr(e->Disc.val, fe_1);
            fprintf(ALL.out, "%s_%s", (s->User.isrec ? "->" : "."), e->Disc.subtype.val.s);
            return 0;
        }

        case EXPR_PRED: {
            Stmt* s = env_expr_to_type_to_user_stmt(e->Pred.val);
            assert(s != NULL);
            int isnil = (e->Pred.subtype.enu == TX_NIL);
            out("((");
            visit_expr(e->Pred.val, fe_1);
            fprintf(ALL.out, "%s == %s) ? (Bool){True,{._True=1}} : (Bool){False,{._False=1}})",
                (isnil ? "" : (s->User.isrec ? "->sub" : ".sub")),
                (isnil ? "NULL" : e->Pred.subtype.val.s)
            );
            return 0;
        }
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

// var transfer ownership
// tuple structs pre declarations
// cons structs pre allocations
// user structs pre declarations

int fe_0 (Expr* e) {
    switch (e->sub) {
        case EXPR_VAR:
            if (e->Var.istx) {
                char* id = e->Var.id.val.s;
                fprintf (ALL.out,
                    "typeof(%s) _%s_%d = %s;\n"
                    "%s = NULL;\n",
                    id, id, e->N, id, id
                );
            }
            return 0;

        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                visit_expr(&e->Tuple.vec[i], fe_0); // first visit child
            }
            ft(env_expr_to_type(e));                // second visit myself
            return 0;

        case EXPR_CONS: {
            visit_expr(e->Cons.arg, fe_0);          // first visit child

            if (e->Cons.subtype.enu == TX_NIL) {
                return 0;                           // out(NULL) in fe_1
            }

            Stmt* user = env_sub_id_to_user_stmt(e->env, e->Cons.subtype.val.s);
            assert(user != NULL);

            char* sup = user->User.id.val.s;
            char* sub = e->Cons.subtype.val.s;

            // Bool __1 = (Bool) { False, {_False=1} };
            // Nat  __1 = (Nat)  { Succ,  {_Succ=&_2} };
            fprintf(ALL.out,
                "%s %s_%d = ((%s) { %s, { ._%s=",
                sup, (user->User.isrec ? "_" : ""), e->N, sup, sub, sub);
            visit_expr(e->Cons.arg, fe_1);
            out(" } });\n");

            if (user->User.isrec) {
                // Nat* _1 = (Nat*) malloc(sizeof(Nat));
                // *_1 = __1;
                fprintf(ALL.out,
                    "%s* _%d = (%s*) malloc(sizeof(%s));\n"
                    "assert(_%d!=NULL && \"not enough memory\");\n"
                    "*_%d = __%d;\n",
                        sup, e->N, sup, sup, e->N, e->N, e->N);
            } else {
                // plain cons: nothing else to do
            }
            return 0;
        }

        case EXPR_DISC: {
            Stmt* s = env_expr_to_type_to_user_stmt(e->Disc.val);
            assert(s != NULL);
            visit_expr(e->Disc.val, fe_0);
            out("assert(");
            visit_expr(e->Disc.val, fe_1);
            fprintf (ALL.out,
                "%ssub == %s && \"discriminator failed\");\n",
                (s->User.isrec ? "->" : "."), e->Disc.subtype.val.s
            );
            return 0;
        }

        default:
            break;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void code_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_USER: {
            const char* sup = s->User.id.val.s;
            const char* SUP = strupper(s->User.id.val.s);
            int isrec = s->User.isrec;

            // struct Bool;
            // typedef struct Bool Bool;
            // void output_Bool_ (Bool v);
            {
                // struct { BOOL sub; union { ... } } Bool;
                fprintf(ALL.out,
                    "struct %s;\n"
                    "typedef struct %s %s;\n",
                    sup, sup, sup
                );

                fprintf(ALL.out,
                    "auto void output_%s_ (%s%s v);\n", // auto: https://stackoverflow.com/a/7319417
                    sup, sup, (isrec ? "*" : "")
                );
            }

            // first generate tuples
            for (int i=0; i<s->User.size; i++) {
                visit_type(&s->User.vec[i].type, ft);
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
                    "struct %s {\n"
                    "    %s sub;\n"
                    "    union {\n",
                    sup, SUP
                );
                for (int i=0; i<s->User.size; i++) {
                    Sub sub = s->User.vec[i];
                    out("        ");
                    out(to_c(&sub.type));
                    out(" _");
                    out(sub.id.val.s);      // int _True
                    out(";\n");
                }
                fprintf(ALL.out,
                    "    };\n"
                    "};\n"
                );
            }
            out("\n");

            // CLEAN
            if (isrec) {
                // Nat_free
                fprintf (ALL.out,
                    "void %s_free (struct %s** p) {\n"
                    "    if (*p == NULL) { return; }\n",
                    sup, sup
                );
                for (int i=0; i<s->User.size; i++) {
                    Sub sub = s->User.vec[i];
                    if (sub.type.sub==TYPE_USER && (!strcmp(sup,sub.type.User.val.s))) {
                        fprintf (ALL.out,
                            "    %s_free(&(*p)->_%s);\n"
                            "    free(*p);\n",
                            sup, sub.id.val.s
                        );
                    }
                }
                out("}\n");
            }

            // OUTPUT
            {
                char* op = (isrec ? "->" : ".");

                // _output_Bool (Bool v)
                fprintf(ALL.out,
                    "void output_%s_ (%s%s v) {\n",
                    sup, sup, (isrec ? "*" : "")
                );
                if (isrec) {
                    out (
                        "if (v == NULL) {\n"
                        "    printf(\"$\");\n"
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
                            sprintf(arg, "output_%s_(v%s_%s)", sub->type.User.val.s, op, sub->id.val.s);
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

        case STMT_VAR: {
            visit_type(&s->Var.type, ft);
            visit_expr(&s->Var.init, fe_0);

            char* id = s->Var.id.val.s;
            char sup[256];
            strcpy(sup, to_ce(&s->Var.type));

            out(to_c(&s->Var.type));
            out(" ");
            out(id);

            if (s->Var.type.sub==TYPE_USER && !s->Var.type.isalias) {
                Stmt* user = env_id_to_stmt(s->env, s->Var.type.User.val.s, NULL);
                assert(user!=NULL && user->sub==STMT_USER);
                if (user->User.isrec) {
                    fprintf (ALL.out,
                        " __attribute__ ((__cleanup__(%s_free)))",
                        sup
                    );
                }
            }

            fputs(" = ", ALL.out);
            visit_expr(&s->Var.init, fe_1);
            out(";\n");

            break;
        }

        case STMT_CALL:
            visit_expr(&s->Call, fe_0);
            visit_expr(&s->Call, fe_1);
            out(";\n");
            break;

        case STMT_RETURN: {
            visit_expr(&s->Ret, fe_0);
            out("return ");
            visit_expr(&s->Ret, fe_1);
            out(";\n");
            break;
        }

        case STMT_SEQ:
            for (int i=0; i<s->Seq.size; i++) {
                code_stmt(&s->Seq.vec[i]);
            }
            break;

        case STMT_IF:
            visit_expr(&s->If.cond, fe_0);
            out("if (");
            visit_expr(&s->If.cond, fe_1);
            out(".sub) {\n");           // Bool.sub returns 0 or 1
            code_stmt(s->If.true);
            out("} else {\n");
            code_stmt(s->If.false);
            out("}\n");
            break;

        case STMT_FUNC: {
            assert(s->Func.type.sub == TYPE_FUNC);
            visit_type(&s->Func.type, ft);

            // f: a -> User

            char tp_out[256] = "";
            to_c_(tp_out, s->Func.type.Func.out);

            char tp_inp[256] = "";
            to_c_(tp_inp, s->Func.type.Func.inp);

            fprintf (ALL.out,
                "%s %s (%s arg) {\n",
                tp_out,
                s->Func.id.val.s,
                tp_inp
            );
            code_stmt(s->Func.body);
            out("}\n");
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_expr_1 (Expr* e) {
    visit_expr(e, fe_1);
}

void code (Stmt* s) {
    out (
        "#include <assert.h>\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#define output_Unit_(x) (assert(((long)(x))==1), printf(\"()\"))\n"
        "#define output_Unit(x)  (output_Unit_(x), puts(\"\"))\n"
        "int main (void) {\n"
        "\n"
    );
    code_stmt(s);
    fprintf(ALL.out, "\n");
    out("}\n");
}
