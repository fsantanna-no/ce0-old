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

void strcat_ (char* dst, const char* src) {
    int N = strlen(dst);
    for (int i=0; i<strlen(src); i++) {
        dst[N-1] = (src[i]=='*' ? '_' : src[i]);
        dst[N] = '\0';
        N++;
    }
}

void out (const char* v) {
    fputs(v, ALL.out);
}

///////////////////////////////////////////////////////////////////////////////

void to_ce_ (char* out, Type* tp) {
    switch (tp->sub) {
        case TYPE_ANY:
            assert(0);
        case TYPE_UNIT:
            strcat(out, "Unit");
            break;
        case TYPE_NATIVE:
            strcat_(out, tp->Native.val.s);
            break;
        case TYPE_USER: {
            strcat(out, tp->User.val.s);
            break;
        }
        case TYPE_TUPLE:
            strcat(out, "TUPLE");
            for (int i=0; i<tp->Tuple.size; i++) {
                strcat(out, "__");
                to_ce_(out, tp->Tuple.vec[i]);
                if (tp->Tuple.vec[i]->isptr) {
                    strcat(out,"_ptr");
                }
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
    switch (tp->sub) {
        case TYPE_ANY:
        case TYPE_UNIT:
            assert(0);
        case TYPE_NATIVE:
            //strcat(out, "typeof(");
            strcat(out, tp->Native.val.s);
            //strcat(out, ")");
            break;
        case TYPE_USER: {
            Stmt* s = env_id_to_stmt(env, tp->User.val.s);
            int isrec = (s!=NULL && s->User.isrec);
            if (isrec) strcat(out, "struct ");
            strcat(out, tp->User.val.s);
            if (isrec) strcat(out, "*");
            break;
        }
        case TYPE_TUPLE:
            to_ce_(out, tp);
            break;
        case TYPE_FUNC:
            assert(0 && "TODO");
    }
    if (tp->isptr) {
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

int env_user_ishasrec (Env* env, Stmt* user) {
    assert(user->sub == STMT_USER);
    if (user->User.isrec) {
        return 1;
    }
    for (int i=0; i<user->User.size; i++) {
        Sub sub = user->User.vec[i];
        if (env_type_ishasrec(env, sub.type, 0)) {
            return 1;
        }
    }
    return 0;
}

void code_free_user (Env* env, Stmt* user) {
    assert(user->sub == STMT_USER);
    assert(env_user_ishasrec(env,user));

    // Nat_free

    const char* sup = user->User.tk.val.s;
    int isrec = user->User.isrec;

    fprintf (ALL.out,
        "void %s_free (struct %s%s* p) {\n",
        sup, sup, (isrec ? "*" : "")
    );

    if (isrec) {
        out("if (*p == NULL) { return; }\n");
    }

    fprintf(ALL.out, "switch ((*p)%ssub) {\n", (isrec ? "->" : "."));
    for (int i=0; i<user->User.size; i++) {
        Sub sub = user->User.vec[i];
        if (env_type_ishasrec(env, sub.type, 0)) {
            fprintf (ALL.out,
                "case %s:\n"
                "   %s_free(&(*p)%s_%s);\n"
                "   break;\n",
                sub.tk.val.s,
                to_ce(sub.type), (isrec ? "->" : "."), sub.tk.val.s
            );
        }
    }
    out (
        "   default:\n"
        "       break;\n"
        "}\n"
    );

    if (isrec) {
        out("    free(*p);\n");
    }

    out("}\n");
}

void code_free_tuple (Env* env, Type* tp) {
    assert(tp->sub == TYPE_TUPLE);
    assert(env_type_ishasrec(env,tp,0));

    char* tp_ = to_ce(tp);
    fprintf (ALL.out,
        "void %s_free (%s* p) {\n",
        tp_, tp_
    );
    for (int i=0; i<tp->Tuple.size; i++) {
        Type* sub = tp->Tuple.vec[i];
        if (env_type_ishasrec(env,sub,0)) {
            fprintf (ALL.out,
                "    %s_free(&p->_%d);\n",
                to_ce(sub), i+1
            );
        }
    }
    out("}\n");
}

///////////////////////////////////////////////////////////////////////////////

void code_null_user (Env* env, Stmt* user) {
    assert(user->sub == STMT_USER);
    assert(env_user_ishasrec(env,user));

    const char* sup = user->User.tk.val.s;
    int isrec = user->User.isrec;

    fprintf (ALL.out,
        "void %s_null (struct %s%s* p) {\n",
        sup, sup, (isrec ? "*" : "")
    );

    if (user->User.isrec) {
        out("*p = NULL;\n");
    } else {
        for (int i=0; i<user->User.size; i++) {
            Sub sub = user->User.vec[i];
            if (env_type_ishasrec(env,sub.type,0)) {
                fprintf (ALL.out,
                    "    %s_null(&(*p)%s_%s);\n",
                    to_ce(sub.type), (isrec ? "->" : "."), sub.tk.val.s
                );
            }
        }
    }

    out("}\n");
}

void code_null_tuple (Env* env, Type* tp) {
    assert(tp->sub == TYPE_TUPLE);
    assert(env_type_ishasrec(env,tp,0));

    char* tp_ = to_ce(tp);
    fprintf (ALL.out,
        "void %s_null (%s* p) {\n",
        tp_, tp_
    );
    for (int i=0; i<tp->Tuple.size; i++) {
        Type* sub = tp->Tuple.vec[i];
        if (env_type_ishasrec(env,sub,0)) {
            fprintf (ALL.out,
                "    %s_null(&p->_%d);\n",
                to_ce(sub), i+1
            );
        }
    }
    out("}\n");
}

void code_clone_tuple (Env* env, Type* tp) {
    assert(tp->sub == TYPE_TUPLE);
    assert(env_type_ishasrec(env,tp,0));

    char* tp_ = to_ce(tp);
    fprintf (ALL.out,
        "%s clone_%s (%s* p) {\n"
        "   return (%s) {",
        tp_, tp_, tp_, tp_
    );
    for (int i=0; i<tp->Tuple.size; i++) {
        Type* sub = tp->Tuple.vec[i];
        if (env_type_ishasrec(env,sub,0)) {
            fprintf (ALL.out,
                "%s clone_%s(&p->_%d)\n",
                (i != 0 ? "," : ""),
                to_ce(sub), i+1
            );
        } else {
            fprintf(ALL.out, "%s p->_%d\n", (i != 0 ? "," : ""), i+1);
        }
    }
    out("   };\n}\n");
}

///////////////////////////////////////////////////////////////////////////////

int ftp_tuples (Env* env, Type* tp_) {
    if (tp_->sub != TYPE_TUPLE) {
        return VISIT_CONTINUE;
    }

    Type tp = *tp_;
    tp.isptr = 0;

    char tp_c [256];
    char tp_ce[256];
    strcpy(tp_c,  to_c (env,&tp));
    strcpy(tp_ce, to_ce(&tp));

    // IFDEF
    out("#ifndef __"); out(tp_ce); out("__\n");
    out("#define __"); out(tp_ce); out("__\n");

    // STRUCT
    out("typedef struct {\n");
    for (int i=0; i<tp.Tuple.size; i++) {
        Type* sub = tp.Tuple.vec[i];
        if (sub->sub == TYPE_UNIT) {
            // do not generate anything
        } else {
            out(to_c(env,tp.Tuple.vec[i]));
            fprintf(ALL.out, " _%d;\n", i+1);
        }
    }
    out("} ");
    out(tp_ce);
    out(";\n");

    int ishasrec = env_type_ishasrec(env, &tp, 0);

    // FREE, NULL
    if (ishasrec) {
        code_free_tuple(env, &tp);
        code_null_tuple(env, &tp);
        code_clone_tuple(env, &tp);
    }

    // STDO
    {
        int hasrec1 = env_type_hasrec(env, &tp, 0);
        fprintf (ALL.out,
            "#ifndef __stdout_%s__\n"
            "#define __stdout_%s__\n",
            tp_ce, tp_ce
        );
        fprintf(ALL.out,
            "void stdout_%s_ (%s%s v) {\n"
            "    printf(\"(\");\n",
            tp_ce, tp_c, (hasrec1 ? "*" : "")
        );
        for (int i=0; i<tp.Tuple.size; i++) {
            if (i > 0) {
                fprintf(ALL.out, "    printf(\",\");\n");
            }
            Type* sub = tp.Tuple.vec[i];

            char* op2 = ""; {
                int ishasrec = env_type_ishasrec(env, sub, 1);
                //int hasrec = env_type_hasrec(env, sub, 0);
                if (ishasrec && !sub->isptr) {
                    op2 = "&";
                } else if (!ishasrec && sub->isptr) {
                    op2 = "*";
                }
            }

            if (sub->sub == TYPE_NATIVE) {
                fprintf(ALL.out, "    putchar('?');\n");
            } else if (sub->sub == TYPE_UNIT) {
                fprintf(ALL.out, "    stdout_Unit_();\n");
            } else {
                fprintf(ALL.out, "    stdout_%s_(%sv%s_%d);\n",
                    to_ce(sub), op2, (hasrec1 ? "->" : "."), i+1);
            }
        }
        fprintf(ALL.out,
            "    printf(\")\");\n"
            "}\n"
            "void stdout_%s (%s%s v) {\n"
            "    stdout_%s_(v);\n"
            "    puts(\"\");\n"
            "}\n",
            tp_ce, tp_c, (hasrec1 ? "*" : ""),
            tp_ce
        );
        out("#endif\n");
    }

    // IFDEF
    out("#endif\n");
    return VISIT_CONTINUE;
}

int code_expr_pre (Env* env, Expr* e) {
    Type* TP = env_expr_to_type(env, e);
    assert(TP != NULL);

    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_UNK:
        case EXPR_NATIVE:
        case EXPR_NULL:
        case EXPR_INT:
        case EXPR_UPREF:
        case EXPR_DNREF:
        case EXPR_TUPLE:
        case EXPR_CALL:
        case EXPR_PRED:
            break;

        case EXPR_VAR: {
            // this prevents "double free"
            if (e->Var.tx_setnull) {
                char* id = e->Var.tk.val.s;
                fprintf(ALL.out, "typeof(%s) _tmp_%d = %s;\n", id, e->N, id);
                fprintf(ALL.out, "%s_null(&%s);\n", to_ce(TP), id);
            }
            break;
        }

        case EXPR_CONS: {
            Stmt* user = env_sub_id_to_user_stmt(env, e->Cons.subtype.val.s);
            assert(user != NULL);

            char* sup = user->User.tk.val.s;
            char* sub = e->Cons.subtype.val.s;

            fprintf (ALL.out,
                "%s%s _tmp_%d = ",
                sup,
                (user->User.isrec ? "*" : ""),
                e->N
            );

            if (user->User.isrec) {
                // Nat* _1 = (Nat*) malloc(sizeof(Nat));
                fprintf (ALL.out,
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

            fprintf(ALL.out, "((%s) { %s", sup, sub);

            Type* tp = env_sub_id_to_user_type(env, sub);
            assert(tp != NULL);
            if (tp->sub!=TYPE_UNIT && e->Cons.arg->sub!=EXPR_UNK) {
                fprintf(ALL.out, ", ._%s=", sub);
                code_expr(env, e->Cons.arg, 0);
            }

            out(" });\n");
            break;
        }

        case EXPR_INDEX: // TODO: f(x).1
            // transfer ownership, prevents "double free"
            if (e->Index.tx_setnull) {
                e->Index.tx_setnull = 0;
                out("typeof(");
                code_expr(env, e, 0);
                fprintf(ALL.out, ") _tmp_%d = ", e->N);
                code_expr(env, e, 0);
                out(";\n");

                out(to_ce(TP));
                out("_null(&");
                code_expr(env, e, 0);
                out(");\n");
                e->Index.tx_setnull = 1;
            }
            break;

        case EXPR_DISC: { // TODO: f(x).Succ
            if (e->Disc.subtype.enu == TX_NULL) {
                out("assert(");
                code_expr(env, e->Disc.val, 0);
                out(" == NULL && \"discriminator failed\");\n");
            } else {
                if (env_type_isrec(env,env_expr_to_type(env,e->Disc.val),0)) {
                    out("assert(");
                    code_expr(env, e->Disc.val, 0);
                    out(" != NULL && \"discriminator failed\");\n");
                }
                out("assert(");
                code_expr(env, e->Disc.val, 1);
                fprintf (ALL.out,
                    ".sub == %s && \"discriminator failed\");\n",
                     e->Disc.subtype.val.s
                 );
            }

            // transfer ownership, prevents "double free"
            if (e->Disc.tx_setnull) {
                e->Disc.tx_setnull = 0;
                out("typeof(");
                code_expr(env, e, 0);
                fprintf(ALL.out, ") _tmp_%d = ", e->N);
                code_expr(env, e, 0);
                out(";\n");

                out(to_ce(TP));
                out("_null(&");
                code_expr(env, e, 0);
                out(");\n");
                e->Disc.tx_setnull = 1;
            }
            break;
        }
    }
    return VISIT_CONTINUE;
}

void code_expr (Env* env, Expr* e, int deref_ishasrec) {
    Type* TP = env_expr_to_type(env, e);
    assert(TP != NULL);
    if (TP->sub==TYPE_UNIT && e->sub!=EXPR_CALL) {
        return;     // no code to generate
    }

    int isrec    = env_type_isrec(env,TP,1);
    //int ishasrec = env_type_ishasrec(env,TP,1);
    int deref = (deref_ishasrec && isrec);
    //int deref = (deref_ishasrec && (isrec || (TP->isptr && ishasrec))) ||
    //            (e->sub!=EXPR_UPREF && TP->isptr && !ishasrec);
    //int deref = (deref_ishasrec && isrec) || (TP->isptr && !ishasrec);
    //
    //int deref = (deref_ishasrec && (TP->isptr || env_type_isrec(env,TP,0)));
    if (deref) {
        out("(*(");
    }

    switch (e->sub) {
        case EXPR_UNK:
            assert(0);

        case EXPR_UNIT:
            break;

        case EXPR_INT:
            fprintf(ALL.out, "%d", e->Int.val.n);
            break;

        case EXPR_NULL:
            out("NULL");
            break;

        case EXPR_NATIVE:
            out(e->Native.val.s);
            break;

        case EXPR_UPREF: {
            out("(&(");
            code_expr(env, e->Upref, 0);
            out("))");
            break;
        }

        case EXPR_DNREF: {
            out("(*(");
            code_expr(env, e->Dnref, 0);
            out("))");
            break;
        }

        case EXPR_CONS:
            fprintf(ALL.out, "_tmp_%d", e->N);
            break;

        case EXPR_CALL:
            if (e->Call.func->sub == EXPR_NATIVE) {
                code_expr(env, e->Call.func, 1);
                out("(");
                code_expr(env, e->Call.arg, 1);
                out(")");
            } else {
                assert(e->Call.func->sub == EXPR_VAR);

                if (!strcmp(e->Call.func->Var.tk.val.s,"output_std")) {
                    out("stdout_");
                    code_to_ce(env_expr_to_type(env, e->Call.arg));
                } else if (!strcmp(e->Call.func->Var.tk.val.s,"clone")) {
                    Type* tp = env_expr_to_type(env, e->Call.arg);
                    if (env_type_ishasrec(env,tp,1)) {
                        out("clone_");
                        code_to_ce(tp);
                    }
                } else {
                    code_expr(env, e->Call.func, 1);
                }

                out("(");
                code_expr(env, e->Call.arg, 0);
                out(")");
            }
            break;

        case EXPR_TUPLE:
            out("((");
            out(to_c(env, TP));
            out(") { ");
            int comma = 0;
            for (int i=0; i<e->Tuple.size; i++) {
                Type* tp = env_expr_to_type(env, e->Tuple.vec[i]);
                if (tp->sub != TYPE_UNIT) {
                    if (comma) {
                        out(",");
                    }
                    comma = 1;
                }
                code_expr(env, e->Tuple.vec[i], 0);
            }
            out(" })");
            break;

        case EXPR_INDEX: {
            if (e->Index.tx_setnull) {
                fprintf(ALL.out, "_tmp_%d", e->N);
            } else {
                code_expr(env, e->Index.val, 1);
                fprintf(ALL.out, "._%d", e->Index.index.val.n);
            }
            break;
        }

        case EXPR_VAR:
            if (e->Var.tx_setnull) {
                fprintf(ALL.out, "_tmp_%d", e->N);
            } else {
                out(e->Var.tk.val.s);
            }
            break;

        case EXPR_DISC:
            if (e->Disc.tx_setnull) {
                fprintf(ALL.out, "_tmp_%d", e->N);
            } else {
                code_expr(env, e->Disc.val, 1);
                fprintf(ALL.out, "._%s", e->Disc.subtype.val.s);
            }
            break;

        case EXPR_PRED: {
            if (e->Pred.subtype.enu == TX_NULL) {
                out("((&");
                code_expr(env, e->Pred.val, 1);
                out(" == NULL)");
            } else {
                out("((");
                code_expr(env, e->Pred.val, 1);
                fprintf(ALL.out, ".sub == %s)", e->Pred.subtype.val.s);
            }
            out(" ? (Bool){True} : (Bool){False})\n");
            break;
        }
    }

    if (deref) {
        out("))");
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_user (Stmt* s) {
    const char* sup = s->User.tk.val.s;
    const char* SUP = strupper(s->User.tk.val.s);
    int isrec = s->User.isrec;

    // stdout must receive & from ishasrec, otherwise it would receive ownership
    int ishasrec = env_user_ishasrec(s->env, s);

    // struct Bool;
    // typedef struct Bool Bool;
    // void List_free (struct List** p);
    // void List_null (struct List** p);
    // void stdout_Bool_ (Bool v);
    // Bool clone_Bool_  (Bool v);
    {
        // struct { BOOL sub; union { ... } } Bool;
        fprintf(ALL.out,
            "struct %s;\n"
            "typedef struct %s %s;\n",
            sup, sup, sup
        );

        if (isrec) {
            fprintf (ALL.out,
                "auto void %s_free (struct %s** p);\n"
                "auto void %s_null (struct %s%s* p);\n",
                sup, sup,
                sup, sup, (isrec ? "*" : "")
            );
        }

        if (ishasrec) {
            fprintf(ALL.out,
                "auto %s%s clone_%s (%s%s v);\n",
                sup, (isrec ? "*" : ""),
                sup,
                sup, (isrec ? "**" : (ishasrec ? "*" : ""))
            );
        }

        fprintf(ALL.out,
            "auto void stdout_%s_ (%s%s v);\n", // auto: https://stackoverflow.com/a/7319417
            sup, sup, (isrec ? "**" : (ishasrec ? "*" : ""))
        );
    }
    out("\n");

    if (s->User.size == 0) {
        return;
    }

    // first generate tuples
    for (int i=0; i<s->User.size; i++) {
        visit_type(s->env, s->User.vec[i].type, ftp_tuples);
    }

    // ENUM + STRUCT + UNION
    {
        // enum { False, True } BOOL;
        out("typedef enum {\n");
            for (int i=0; i<s->User.size; i++) {
                out("    ");
                out(s->User.vec[i].tk.val.s);    // False
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
            Sub* sub = &s->User.vec[i];
            if (sub->type->sub == TYPE_UNIT) {
                // do not generate anything
            } else {
                out("        ");
                out(to_c(s->env, sub->type));
                out(" _");
                out(sub->tk.val.s);      // int _True
                out(";\n");
            }
        }
        fprintf(ALL.out,
            "    };\n"
            "};\n"
        );
    }
    out("\n");

    // FREE
    if (env_user_ishasrec(s->env,s)) {
        code_free_user(s->env, s);
        code_null_user(s->env, s);
    }

    // CLONE
    if (ishasrec) {
        char* v = (isrec ? "(*v)" : "v");
        fprintf(ALL.out,
            "%s%s clone_%s (%s%s v) {\n",
            sup, (isrec ? "*" : ""),
            sup,
            sup, (isrec ? "**" : (ishasrec ? "*" : ""))
        );
        if (!ishasrec) {
            out("return v;\n");
        } else {
            if (isrec) {
                fprintf (ALL.out,
                    "if (%s == NULL) {\n"
                    "   return NULL;\n"
                    "}\n",
                    v
                );
            }
            fprintf(ALL.out, "switch (%s->sub) {\n", v);
            for (int i=0; i<s->User.size; i++) {
                Sub* sub = &s->User.vec[i];
                char* sub_id = sub->tk.val.s;
                char* sub_tp = to_ce(sub->type);
                int sub_ishasrec = env_type_ishasrec(s->env, sub->type, 0);
                fprintf(ALL.out, "case %s:\n", sub_id);
                if (ishasrec) {
                    char clone[256];
                    if (sub->type->sub == TYPE_UNIT) {
                        assert(!sub->type->isptr);  // TODO: refuse \() or \((),()), etc
                        strcpy(clone, "");
                    } else if (sub_ishasrec) {
                        sprintf(clone, "._%s=clone_%s(&%s->_%s)", sub_id, sub_tp, v, sub_id);
                    } else {
                        sprintf(clone, "._%s=%s->_%s", sub_id, v, sub_id);
                    }

                    if (isrec) {
                        fprintf (ALL.out,
                            "{\n"
                            "   %s* ret = malloc(sizeof(%s));\n"
                            "   assert(ret!=NULL && \"not enough memory\");\n"
                            "   *ret = (%s) { %s, {%s} };\n"
                            "   return ret;\n"
                            "}\n",
                            sup, sup,
                            sup, sub_id, clone
                        );
                    } else {
                        fprintf (ALL.out,
                            "return (%s) { %s, {%s} };\n",
                            sup, sub_id, clone
                        );
                    }
                } else {
                    fprintf(ALL.out, "return %s;\n", v);
                }
            }
            out("}\n");
        }
        out("assert(0);\n");
        out("}\n");
    }
    out("\n");

    // STDO
    {
        // _stdout_Bool (Bool v)
        fprintf(ALL.out,
            "void stdout_%s_ (%s%s v) {\n",
            sup, sup, (isrec ? "**" : (ishasrec ? "*" : ""))
        );
        if (isrec) {
            out (
                "if (*v == NULL) {\n"
                "    printf(\"$\");\n"
                "    return;\n"
                "}\n"
            );
        }

        char* v = (isrec ? "(*v)->" : (ishasrec ? "v->" : "v."));

        fprintf(ALL.out, "    switch (%ssub) {\n", v);
        for (int i=0; i<s->User.size; i++) {
            Sub* sub = &s->User.vec[i];

            char arg[TK_BUF*2+256] = "";
            int yes = 0;
            int par = 0;

            char* op2 = ""; {
                int ishasrec = env_type_ishasrec(s->env, sub->type, 1);
                if (ishasrec && !sub->type->isptr) {
                    op2 = "&";
                } else if (!ishasrec && sub->type->isptr) {
                    op2 = "*";
                }
            }

            switch (sub->type->sub) {
                case TYPE_UNIT:
                    yes = par = 0;
                    break;
                case TYPE_NATIVE:
                    strcpy(arg,"putchar('?');");
                    break;
                case TYPE_USER:
                    yes = par = 1;
                    sprintf(arg, "stdout_%s_(%s%s_%s)",
                        sub->type->User.val.s, op2,
                        v, sub->tk.val.s);
                    break;
                case TYPE_TUPLE:
                    yes = 1;
                    par = 0;
                    sprintf(arg, "stdout_%s_(%s%s_%s)",
                        to_ce(sub->type), op2,
                        v, sub->tk.val.s);
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
                sub->tk.val.s, sub->tk.val.s, yes?" ":"", par?"(":"", arg, par?")":""
            );
        }
        out("    }\n");
        out("}\n");
        fprintf(ALL.out,
            "void stdout_%s (%s%s v) {\n"
            "    stdout_%s_(v);\n"
            "    puts(\"\");\n"
            "}\n",
            sup, sup, (isrec ? "**" : (ishasrec ? "*" : "")),
            sup
        );
    }
}

void code_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_NONE:
            break;

        case STMT_SEQ:
            code_stmt(s->Seq.s1);
            code_stmt(s->Seq.s2);
            break;

        case STMT_CALL:
            visit_expr(1, s->env, s->Call, code_expr_pre);
            code_expr(s->env, s->Call, 1);
            out(";\n");
            break;

        case STMT_USER: {
            if (!strcmp(s->User.tk.val.s,"Int")) {
                break;
            }
            code_user(s);
            break;
        }

        case STMT_VAR: {
            visit_type(s->env, s->Var.type, ftp_tuples);
            visit_expr(1, s->env, s->Var.init, code_expr_pre);

            if (s->Var.type->sub == TYPE_UNIT) {
                break;
            }

            fprintf(ALL.out, "%s %s", to_c(s->env,s->Var.type), s->Var.tk.val.s);

            if (env_type_ishasrec(s->env,s->Var.type,0)) {
                fprintf (ALL.out,
                    " __attribute__ ((__cleanup__(%s_free)))",
                    to_ce(s->Var.type)
                );
            }

            if (s->Var.init->sub != EXPR_UNK) {
                out(" = ");
                code_expr(s->env, s->Var.init, 0);
            }
            out(";\n");
            break;
        }

        case STMT_SET: {
            visit_expr(1, s->env, s->Set.src, code_expr_pre);
            visit_expr(1, s->env, s->Set.dst, code_expr_pre);

            Type* dst = env_expr_to_type(s->env, s->Set.dst);
            assert(dst != NULL);
            if (dst->sub == TYPE_UNIT) {
                break;
            }

            // TODO: if "dst" is ishasrec, need to free it
            if (env_type_ishasrec(s->env,dst,0)) {
                fprintf(ALL.out, "%s_free(&", to_ce(dst));
                code_expr(s->env, s->Set.dst, 0);
                out(");\n");
            }

            code_expr(s->env, s->Set.dst, 0);
            out(" = ");
            code_expr(s->env, s->Set.src, 0);
            out(";\n");
            break;
        }

        case STMT_RETURN: {
            visit_expr(1, s->env, s->Return, code_expr_pre);
            out("return ");
            code_expr(s->env, s->Return, 0);
            out(";\n");
            break;
        }

        case STMT_IF: {
            visit_expr(1, s->env, s->If.tst, code_expr_pre);
            out("if (");
            code_expr(s->env, s->If.tst, 1);
            out(".sub) {\n");
            code_stmt(s->If.true);
            out("} else {\n");
            code_stmt(s->If.false);
            out("}\n");
            break;
        }

        case STMT_BREAK:
            out("break;\n");
            break;
        case STMT_LOOP:
            out("while (1) {\n");
            code_stmt(s->Loop);
            out("}\n");
            break;

        case STMT_FUNC: {
            assert(s->Func.type->sub == TYPE_FUNC);

            if (!strcmp(s->Func.tk.val.s,"clone")) break;
            if (!strcmp(s->Func.tk.val.s,"output_std")) break;

            visit_type(s->env, s->Func.type, ftp_tuples);

            // f: a -> User

            int out_isunit = (s->Func.type->Func.out->sub == TYPE_UNIT);
            char tp_out[256] = "";
            if (!out_isunit) {
                to_c_(tp_out, s->env, s->Func.type->Func.out);
            }

            int inp_isunit = (s->Func.type->Func.inp->sub == TYPE_UNIT);
            char tp_inp[256] = "";
            if (!inp_isunit) {
                to_c_(tp_inp, s->env, s->Func.type->Func.inp);
            }

            fprintf (ALL.out,
                "auto %s %s (%s %s)",
                (out_isunit ? "void" : tp_out),
                s->Func.tk.val.s,
                (inp_isunit ? "void" : tp_inp),
                (inp_isunit ? "" : "_arg_")
            );

            if (s->Func.body == NULL) {
                out(";\n");     // pre-declaration
            } else {
                out("{\n");
                code_stmt(s->Func.body);
                out("}\n");
            }
            break;
        }

        case STMT_BLOCK:
            out("{\n");
            code_stmt(s->Block);
            out("}\n");
            break;

        case STMT_NATIVE:
            if (!s->Native.ispre) {
                out(s->Native.tk.val.s);
            }
            out("\n");
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////

int code_native_pre (Stmt* s) {
    if (s->sub==STMT_NATIVE && s->Native.ispre) {
        out(s->Native.tk.val.s);
    }
    return VISIT_CONTINUE;
}

void code (Stmt* s) {
    assert(visit_stmt(0,s,code_native_pre,NULL,NULL));
    out (
        "#include <assert.h>\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "typedef int Int;\n"
        "#define stdout_Unit_() printf(\"()\")\n"
        "#define stdout_Unit()  (stdout_Unit_(), puts(\"\"))\n"
        "#define stdout_Int_(x) printf(\"%d\",x)\n"
        "#define stdout_Int(x)  (stdout_Int_(x), puts(\"\"))\n"
        "int main (void) {\n"
        "\n"
    );
    code_stmt(s);
    fprintf(ALL.out, "\n");
    out("}\n");
}
