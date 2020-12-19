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
            strcat(out, tp->Native.val.s);
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

void to_c_ (char* out, Env* env, Type* tp) {
    int isrec = 0;
    switch (tp->sub) {
        case TYPE_UNIT:
            strcat(out, "int");
            break;
        case TYPE_NATIVE:
            strcat(out, tp->Native.val.s);
            break;
        case TYPE_USER: {
            Stmt* s = env_id_to_stmt(env, tp->User.val.s, NULL);
            isrec = (s!=NULL && s->User.isrec);
            if (isrec) strcat(out, "struct ");
            strcat(out, tp->User.val.s);
            if (isrec) strcat(out, "*");
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
    if (tp->isalias && !isrec) {
        strcat(out, "*");
    }
}

char* to_c (Env* env, Type* tp) {
    static char out[256];
    out[0] = '\0';
    to_c_(out, env, tp);
    return out;
}

///////////////////////////////////////////////////////////////////////////////

int ftp (Type* tp, void* env_) {
    Env* env = (Env*) env_;

    if (tp->sub == TYPE_TUPLE) {
        char tp_c [256];
        char tp_ce[256];
        strcpy(tp_c,  to_c (env,tp));
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
            out(to_c(env,&tp->Tuple.vec[i]));
            fprintf(ALL.out, " _%d;\n", i+1);
        }
        out("} ");
        out(tp_ce);
        out(";\n");

        // OUTPUT
        fprintf(ALL.out,
            "int output_%s_ (%s v) {\n"
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
            "    return 1;\n"
            "}\n"
            "int output_%s (%s v) {\n"
            "    output_%s_(v);\n"
            "    puts(\"\");\n"
            "    return 1;\n"
            "}\n",
            tp_ce, tp_c, tp_ce
        );

        // IFDEF
        out("#endif\n");
    }
    return 1;
}

char* ftk_ (Tk* tk) {
    static char decl[256];
    sprintf(decl, "_tmp_%ld_%ld", tk->lin, tk->col);
    return decl;
}

char* ftk (Env* env, Tk* tk) {
    char* decl = ftk_(tk);

    switch (tk->enu) {
        case TK_UNIT:
            fprintf(ALL.out, "int %s = 1;\n", decl);
            break;
        case TX_NATIVE:
        case TX_VAR:
            fprintf(ALL.out, "typeof(%s) %s = %s;\n", tk->val.s, decl, tk->val.s);
            break;
        case TX_NULL:
            fprintf(ALL.out, "%s* %s = NULL;\n", tk->val.s, decl);
            break;
        default:
//printf(">>> %d\n", tk->enu);
            assert(0);
    }

    // this prevents "double free"
    if (env_tk_istx(env, tk)) {
        fprintf(ALL.out, "%s = NULL;\n", tk->val.s);
            // Set EXPR_VAR.istx=1 for root recursive and not alias/alias type.
            //      f(nat)          -- istx=1       -- root, recursive
            //      return nat      -- istx=1       -- root, recursive
            //      output(&nat)    -- istx=0       -- root, recursive, alias
            //      f(alias_nat)    -- istx=0       -- root, recursive, alias type
            //      nat.xxx         -- istx=0       -- not root, recursive
    }

    return decl;
}

// prepares _tmp_N

void fe_tmp_set (Env* env, Expr* e, char* tp) {
    fprintf(ALL.out, "%s _tmp_%d = ", (tp!=NULL ? tp : to_c(env,env_expr_to_type(env,e))), e->N);
}

int isaddr (Env* env, Expr* e) {
    return (e->isalias && !env_type_isrec(env,env_expr_to_type(env,e)));
}

void fe (Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_NULL:
        case EXPR_NATIVE:
        case EXPR_VAR: {
            char* tk = ftk(env, &e->tk);
            char* tp = NULL;
            char tp_[256];
            if (e->sub == EXPR_NATIVE) {
                sprintf(tp_, "typeof(%s)", tk);
                tp = tp_;
            }
            fe_tmp_set(env, e, tp);
//printf(">>> %s -> %d\n", e->tk.val.s, env_type_isrec(env,env_expr_to_type(env,e)));
            if (isaddr(env,e)) {
                out(" /* aqui */ &");
            }
            out(tk);
            out(";\n");
            return;
        }

        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                ftk(env, &e->Tuple.vec[i]);
            }
            fe_tmp_set(env, e, NULL);
            fprintf(ALL.out, "((%s) {", to_c(env, env_expr_to_type(env,e)));
            for (int i=0; i<e->Tuple.size; i++) {
                if (i != 0) {
                    out(",");
                }
                fprintf(ALL.out, "_tmp_%ld_%ld", e->Tuple.vec[i].lin, e->Tuple.vec[i].col);
            }
            out(" });\n");
            return;

        case EXPR_INDEX: {
            char* val = ftk(env, &e->Index.val);
            fe_tmp_set(env, e, NULL);
            fprintf(ALL.out, "%s%s._%d;\n", (isaddr(env,e) ? "&":""), val, e->Index.index.val.n);
            return;
        }

        case EXPR_CALL: {
            char* arg = ftk(env, &e->Call.arg);

            char* tp = NULL;
            char tp_[512];
            if (e->Call.func.enu == TX_NATIVE) {
                sprintf(tp_, "typeof(%s(%s))", e->Call.func.val.s, arg);
                tp = tp_;
            }

            fe_tmp_set(env, e, tp);

            if (e->Call.func.enu == TX_VAR) {
                if (!strcmp(e->Call.func.val.s,"clone")) {
                    out("clone_");
                    code_to_ce(env_tk_to_type(env, &e->Call.arg));
                } else if (!strcmp(e->Call.func.val.s,"output")) {
                    out("output_");
                    code_to_ce(env_tk_to_type(env, &e->Call.arg));
                } else {
                    out(e->Call.func.val.s);
                }
            } else {
                out(e->Call.func.val.s);
            }

            fprintf(ALL.out, "(%s);\n", arg);
            return;
        }

        case EXPR_CONS: {
            char* arg = ftk(env, &e->Cons.arg);
            fe_tmp_set(env, e, NULL);

            Stmt* user = env_sub_id_to_user_stmt(env, e->Cons.subtype.val.s);
            assert(user != NULL);

            char* sup = user->User.id.val.s;
            char* sub = e->Cons.subtype.val.s;

            if (user->User.isrec) {
                // Nat* _1 = (Nat*) malloc(sizeof(Nat));
                fprintf( ALL.out,
                    "(%s*) malloc(sizeof(%s));\n"
                    "assert(_tmp_%d!=NULL && \"not enough memory\");\n"
                    "*_tmp_%d = ",
                    sup, sup, e->N, e->N
                );
            } else {
                // plain cons: nothing else to do
            }

            // Bool __1 = (Bool) { False, {_False=1} };
            // Nat  __1 = (Nat)  { Succ,  {_Succ=&_2} };
            fprintf(ALL.out,
                "((%s) { %s, ._%s=%s });\n",
                sup, sub, sub, arg
            );
            return;
        }

        case EXPR_DISC: {
            Stmt* s = env_tk_to_type_to_user_stmt(env, &e->Disc.val);
            assert(s != NULL);

            char* val = ftk(env, &e->Disc.val);

            fprintf (ALL.out,
                "assert(%s%ssub == %s && \"discriminator failed\");\n",
                val, (s->User.isrec ? "->" : "."), e->Disc.subtype.val.s
            );

            fe_tmp_set(env, e, NULL);
            fprintf(ALL.out, "%s%s%s_%s\n;", (isaddr(env,e) ? "&" : ""), val, (s->User.isrec ? "->" : "."), e->Disc.subtype.val.s);
            return;
        }

        case EXPR_PRED: {
            Stmt* s = env_tk_to_type_to_user_stmt(env, &e->Pred.val);
            assert(s != NULL);

            char* val = ftk(env, &e->Disc.val);

            fe_tmp_set(env, e, NULL);

            int isnil = (e->Pred.subtype.enu == TX_NULL);
            fprintf(ALL.out, "((%s%s == %s) ? (Bool){True,{._True=1}} : (Bool){False,{._False=1}});\n",
                val,
                (isnil ? "" : (s->User.isrec ? "->sub" : ".sub")),
                (isnil ? "NULL" : e->Pred.subtype.val.s)
            );
            return;
        }

    }
    assert(0);
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
                    "auto int output_%s_ (%s%s v);\n", // auto: https://stackoverflow.com/a/7319417
                    sup, sup, (isrec ? "*" : "")
                );
            }

            // first generate tuples
            for (int i=0; i<s->User.size; i++) {
                visit_type(&s->User.vec[i].type, ftp, s->env);
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
                    out(to_c(s->env, &sub.type));
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
                    "int output_%s_ (%s%s v) {\n",
                    sup, sup, (isrec ? "*" : "")
                );
                if (isrec) {
                    out (
                        "if (v == NULL) {\n"
                        "    printf(\"$\");\n"
                        "    return 1;\n"
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
                out("    return 1;\n");
                out("}\n");
                fprintf(ALL.out,
                    "int output_%s (%s%s v) {\n"
                    "    output_%s_(v);\n"
                    "    puts(\"\");\n"
                    "    return 1;\n"
                    "}\n",
                    sup, sup, (isrec ? "*" : ""), sup
                );
            }
            break;
        }

        case STMT_VAR: {
            visit_type(&s->Var.type, ftp, s->env);
            fe(s->env, &s->Var.init);

            char* id = s->Var.id.val.s;
            char sup[256];
            strcpy(sup, to_ce(&s->Var.type));

            out(to_c(s->env, &s->Var.type));
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

            fprintf(ALL.out, " = _tmp_%d;\n", s->Var.init.N);
            break;
        }

        case STMT_CALL:
            fe(s->env, &s->Call);
            break;

        case STMT_RETURN: {
            char* ret = ftk(s->env, &s->Return);
            fprintf(ALL.out, "return %s;\n", ret);
            break;
        }

        case STMT_SEQ:
            for (int i=0; i<s->Seq.size; i++) {
                code_stmt(&s->Seq.vec[i]);
            }
            break;

        case STMT_IF: {
            char* tst = ftk(s->env, &s->If.cond); // Bool.sub returns 0 or 1
            fprintf(ALL.out, "if (%s.sub) {\n", tst);
            code_stmt(s->If.true);
            out("} else {\n");
            code_stmt(s->If.false);
            out("}\n");
            break;
        }

        case STMT_FUNC: {
            assert(s->Func.type.sub == TYPE_FUNC);
            visit_type(&s->Func.type, ftp, s->env);

            // f: a -> User

            char tp_out[256] = "";
            to_c_(tp_out, s->env, s->Func.type.Func.out);

            char tp_inp[256] = "";
            to_c_(tp_inp, s->env, s->Func.type.Func.inp);

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

        case STMT_BLOCK:
            code_stmt(s->Block);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_expr (Expr* e) {
    fe(NULL, e);
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
