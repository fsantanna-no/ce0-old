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
    if (tp->isalias) {
        strcat(out, "x");
    }
    switch (tp->sub) {
        case TYPE_AUTO:
            assert(0);
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
                to_ce_(out, tp->Tuple.vec[i]);
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
        case TYPE_AUTO:
        case TYPE_UNIT:
            assert(0);
        case TYPE_NATIVE:
            strcat(out, "typeof(");
            strcat(out, tp->Native.val.s);
            strcat(out, ")");
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
                to_ce_(out, tp->Tuple.vec[i]);
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

int env_user_hasalloc (Env* env, Stmt* user) {
    assert(user->sub == STMT_USER);
    if (user->User.isrec) {
        return 1;
    }
    for (int i=0; i<user->User.size; i++) {
        Sub sub = user->User.vec[i];
        if (env_type_hasalloc(env, sub.type)) {
            return 1;
        }
    }
    return 0;
}

void code_free_user (Env* env, Stmt* user) {
    assert(user->sub == STMT_USER);
    assert(env_user_hasalloc(env,user));

    // Nat_free

    const char* sup = user->User.id.val.s;
    int isrec = user->User.isrec;

    fprintf (ALL.out,
        "void %s_free (struct %s%s* p) {\n",
        sup, sup, (isrec ? "*" : "")
    );

    if (isrec) {
        out("    if (*p == NULL) { return; }\n");
    }

    for (int i=0; i<user->User.size; i++) {
        Sub sub = user->User.vec[i];
        if (env_type_hasalloc(env, sub.type)) {
            fprintf (ALL.out,
                "    %s_free(&(*p)%s_%s);\n",
                to_ce(sub.type), (isrec ? "->" : "."), sub.id.val.s
            );
        }
    }

    if (isrec) {
        out("    free(*p);\n");
    }

    out("}\n");
}

void code_free (Env* env, Type* tp) {
    switch (tp->sub) {
        case TYPE_TUPLE: {
            if (!env_type_hasalloc(env,tp)) {
                return;
            }

            char* tp_ = to_ce(tp);
            fprintf (ALL.out,
                "void %s_free (%s* p) {\n",
                tp_, tp_
            );
            for (int i=0; i<tp->Tuple.size; i++) {
                Type* sub = tp->Tuple.vec[i];
                if (env_type_hasalloc(env,sub)) {
                    fprintf (ALL.out,
                        "    %s_free(&p->_%d);\n",
                        to_ce(sub), i+1
                    );
                }
            }
            out("}\n");
            break;
        }
        case TYPE_USER: { // TODO-set-null
            Stmt* user = env_id_to_stmt(env, tp->User.val.s, NULL);
            assert(user != NULL);
            if (env_user_hasalloc(env,user)) {
                code_free_user(env, user);
            }
            break;
        }
        default:
            assert(0);
    }
}

///////////////////////////////////////////////////////////////////////////////

char* toptr (Env* env, Type* tp, char* op1, char* op2) {
    return (!env_type_isrec(env,tp) && env_type_hasalloc(env,tp)) ? op1 : op2;
}

int ftp_tuples (Env* env, Type* tp) {
    if (tp->sub != TYPE_TUPLE) {
        return VISIT_CONTINUE;
    }

    char tp_c [256];
    char tp_ce[256];
    strcpy(tp_c,  to_c (env,tp));
    strcpy(tp_ce, to_ce(tp));

    // IFDEF
    out("#ifndef __"); out(tp_ce); out("__\n");
    out("#define __"); out(tp_ce); out("__\n");

    // STRUCT
    out("typedef struct {\n");
    for (int i=0; i<tp->Tuple.size; i++) {
        Type* sub = tp->Tuple.vec[i];
        if (sub->sub == TYPE_UNIT) {
            // do not generate anything
        } else {
            out(to_c(env,tp->Tuple.vec[i]));
            fprintf(ALL.out, " _%d;\n", i+1);
        }
    }
    out("} ");
    out(tp_ce);
    out(";\n");

    // FREE
    code_free(env, tp);

    int hasalloc = env_type_hasalloc(env, tp);

    // SHOW
    {
        fprintf (ALL.out,
            "#ifndef __show_%s%s__\n"
            "#define __show_%s%s__\n",
            ((!tp->isalias && hasalloc) ? "x" : ""), tp_ce,
            ((!tp->isalias && hasalloc) ? "x" : ""), tp_ce
        );
        fprintf(ALL.out,
            "int show_%s%s_ (%s%s v) {\n"
            "    printf(\"(\");\n",
            ((!tp->isalias && hasalloc) ? "x" : ""),
            tp_ce, tp_c, toptr(env,tp,"*","")
        );
        for (int i=0; i<tp->Tuple.size; i++) {
            if (i > 0) {
                fprintf(ALL.out, "    printf(\",\");\n");
            }
            Type* sub = tp->Tuple.vec[i];
            if (sub->sub == TYPE_NATIVE) {
                fprintf(ALL.out, "    putchar('?');\n");
            } else if (sub->sub == TYPE_UNIT) {
                fprintf(ALL.out, "    show_Unit_();\n");
            } else {
                int hasalloc = env_type_hasalloc(env, sub);
                fprintf(ALL.out, "    show_%s%s_(%sv%s_%d);\n",
                    ((/*sub->isalias ||*/ hasalloc) ? "x" : ""),
                    to_ce(sub), toptr(env,sub,"&",""), toptr(env,tp,"->","."), i+1);
            }
        }
        fprintf(ALL.out,
            "    printf(\")\");\n"
            "    return 1;\n"
            "}\n"
            "int show_%s%s (%s%s v) {\n"
            "    show_%s%s_(v);\n"
            "    puts(\"\");\n"
            "    return 1;\n"
            "}\n",
            ((!tp->isalias && hasalloc) ? "x" : ""), tp_ce, tp_c, toptr(env,tp,"*",""),
            ((!tp->isalias && hasalloc) ? "x" : ""), tp_ce
        );
        out("#endif\n");
    }

    // IFDEF
    out("#endif\n");
    return VISIT_CONTINUE;
}
#if XXXXXX

char* ftk_ (Tk* tk) {
    static char decl[256];
    sprintf(decl, "_tmp_%ld_%ld", tk->lin, tk->col);
    return decl;
}

char* ftk (Env* env, Tk* tk, int istx) {
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
        case TX_NUM:
            fprintf(ALL.out, "int %s = %d;\n", decl, tk->val.n);
            break;
        default:
//printf(">>> %d\n", tk->enu);
            assert(0);
    }

    // Set EXPR_VAR.istx=1 for root recursive and not alias/alias type.
    //      f(nat)          -- istx=1       -- root, recursive
    //      return nat      -- istx=1       -- root, recursive
    //      show(&nat)    -- istx=0       -- root, recursive, alias
    //      f(alias_nat)    -- istx=0       -- root, recursive, alias type
    //      nat.xxx         -- istx=0       -- not root, recursive
    if (istx && tk->enu==TX_VAR) {
        Type* tp = env_tk_to_type(env,tk);
        assert(tp != NULL);
        if (env_type_hasalloc(env,tp) && !tp->isalias) {
            // this prevents "double free"

            if (env_type_isrec(env,tp)) {
                fprintf(ALL.out, "%s = NULL;\n", tk->val.s);
            } else {
                switch (tp->sub) {
                    case TYPE_TUPLE:
                        for (int i=0; i<tp->Tuple.size; i++) {
                            if (env_type_isrec(env,&tp->Tuple.vec[i])) {
                                fprintf(ALL.out, "%s._%d = NULL;\n", tk->val.s, i+1);
                            }
                        }
                        break;
                    case TYPE_USER: {
                        Stmt* decl = env_id_to_stmt(env, tp->User.val.s, NULL);
                        assert(decl!=NULL && decl->sub==STMT_USER);
                        for (int i=0; i<decl->User.size; i++) {
                            Sub* sub = &decl->User.vec[i];
                            if (env_type_isrec(env,&sub->type)) {
                                fprintf(ALL.out, "%s._%s = NULL;\n", tk->val.s, sub->id.val.s);
                            }
                        }
                        break;
                    }
                    default:
                        assert(0);
                }
            }
        }
    }

    return decl;
}

// prepares _tmp_N

void fe_tmp_set (Env* env, Expr* e, char* tp) {
    if (tp == NULL) {
        tp = to_c(env,env_expr_to_type(env,e));
    }
    fprintf(ALL.out, "%s _tmp_%d = ", tp, e->N);
}

int isaddr (Env* env, Expr* e) {
    return (e->isalias && !env_type_isrec(env,env_expr_to_type(env,e)));
}

void fe (Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_NULL:
        case EXPR_INT:
        case EXPR_NATIVE:
        case EXPR_VAR: {
            char* tk = ftk(env, &e->tk, (e->sub==EXPR_VAR && !e->isalias));
            char* tp = NULL;
            char tp_[256];
            if (e->sub == EXPR_NATIVE) {
                sprintf(tp_, "typeof(%s)", tk);
                tp = tp_;
            }
            fe_tmp_set(env, e, tp);
//printf(">>> %s -> %d\n", e->tk.val.s, env_type_isrec(env,env_expr_to_type(env,e)));
            if (isaddr(env,e)) {
                out("&");
            }
            out(tk);
            out(";\n");
            return;
        }

        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                ftk(env, &e->Tuple.vec[i], 1);
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
            char* val = ftk(env, &e->Index.val, 0);
            fe_tmp_set(env, e, NULL);
            fprintf(ALL.out, "%s%s._%d;\n", (isaddr(env,e) ? "&":""), val, e->Index.index.val.n);

            // transfer ownership
            Type* tp = env_expr_to_type(env,e);
            assert(tp != NULL);
            if (env_type_hasalloc(env,tp)) { // also checks isalias
                // this prevents "double free"
                fprintf(ALL.out, "%s._%d = NULL;\n", e->Index.val.val.s, e->Index.index.val.n);
            }
            return;
        }

        case EXPR_CALL: {
            char* arg = ftk(env, &e->Call.arg, 1);

            char* tp = NULL;
            char tp_[512];
            if (e->Call.func.enu == TX_NATIVE) {
                if (e->Call.arg.enu == TK_UNIT) {
                    sprintf(tp_, "typeof(%s())", e->Call.func.val.s);   // _f() instead of _f(1)
                } else {
                    sprintf(tp_, "typeof(%s(%s))", e->Call.func.val.s, arg);
                }
                tp = tp_;
            } else if (e->Call.func.enu==TX_VAR && !strcmp(e->Call.func.val.s,"clone")) {
                sprintf(tp_, "typeof(%s_%s(%s))", e->Call.func.val.s, to_ce(env_tk_to_type(env,&e->Call.arg)), arg);
                tp = tp_;
            }

            if (!e->Call.isstmt) {
                fe_tmp_set(env, e, tp);
            }

            if (e->Call.func.enu == TX_VAR) {
                if (!strcmp(e->Call.func.val.s,"clone")) {
                    out("clone_");
                    code_to_ce(env_tk_to_type(env, &e->Call.arg));
                } else if (!strcmp(e->Call.func.val.s,"show")) {
                    out("show_");
                    code_to_ce(env_tk_to_type(env, &e->Call.arg));
                } else {
                    out(e->Call.func.val.s);
                }
            } else {
                out(e->Call.func.val.s);
            }

            if (e->Call.func.enu==TX_NATIVE && e->Call.arg.enu==TK_UNIT) {
                out("();\n");   // _f() instead of _f(1)
            } else {
                fprintf(ALL.out, "(%s);\n", arg);
            }
            return;
        }

        default:
            assert(0);
    }
    assert(0);
}
#endif

int code_expr_pre (Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_CONS: {
            Stmt* user = env_sub_id_to_user_stmt(env, e->Cons.subtype.val.s);
            assert(user != NULL);

            char* sup = user->User.id.val.s;
            char* sub = e->Cons.subtype.val.s;

            fprintf(ALL.out, "%s _tmp_%d = ", sup, e->N);

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
            if (tp->sub != TYPE_UNIT) {
                fprintf(ALL.out, ", ._%s=", sub);
                code_expr(env, e->Cons.arg, 0);
            }

            out(" });\n");
            break;
        }

        case EXPR_DISC: {
            Type* tp = env_expr_to_type(env, e->Disc.val);
            out("assert(");
            code_expr(env, e->Disc.val, 1);
            fprintf (ALL.out,
                ".sub == %s && \"discriminator failed\");\n",
                e->Disc.subtype.val.s
            );

#if XXXXXX
            int isptr = 0; // TODO: env_tk_isptr(env, &e->Disc.val);
            fe_tmp_set(env, e, NULL);
            fprintf(ALL.out, "%s%s%s_%s;\n", (isaddr(env,e) ? "&" : ""), val, (isptr ? "->" : "."), e->Disc.subtype.val.s);

            // transfer ownership
            Type* tp = env_expr_to_type(env,e);
            assert(tp != NULL);
            if (env_type_hasalloc(env,tp)) { // also checks isalias
                // this prevents "double free"
                fprintf(ALL.out, "%s%s_%s = NULL;\n", e->Disc.val.val.s, (isptr ? "->" : "."), e->Disc.subtype.val.s);
            }
#endif
            break;
        }

        default:
            break;
    }
    return VISIT_CONTINUE;
}

void code_expr (Env* env, Expr* e, int ctxplain) {
    Type* TP = env_expr_to_type(env, e);
    if (TP->sub==TYPE_UNIT && e->sub!=EXPR_CALL) {
        return;     // no code to generate
    }

    if (ctxplain && TP->isalias) {
        out("(*(");
    }

    switch (e->sub) {
        case EXPR_UNIT:
            break;

        case EXPR_INT:
            fprintf(ALL.out, "%d", e->Int.val.n);
            break;

        case EXPR_NATIVE:
            out(e->Native.val.s);
            break;

        case EXPR_VAR: {
            out(e->Var.val.s);
            break;
        }

        case EXPR_ALIAS: {
            out("&");
            code_expr(env, e->Alias, 0);
            break;
        }

        case EXPR_CONS:
            fprintf(ALL.out, "_tmp_%d", e->N);
            break;

        case EXPR_CALL: {
            if (e->Call.func->sub == EXPR_NATIVE) {
                code_expr(env, e->Call.func, 0);
                out("(");
                code_expr(env, e->Call.arg, 0);
                out(")");
            } else {
                assert(e->Call.func->sub == EXPR_VAR);

                if (!strcmp(e->Call.func->Var.val.s,"show")) {
                    out("show_");
                    code_to_ce(env_expr_to_type(env, e->Call.arg));
                } else {
                    code_expr(env, e->Call.func, 0);
                }

                out("(");
                code_expr(env, e->Call.arg, 0);
                out(")");
            }
            break;
        }

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

        case EXPR_INDEX:
            code_expr(env, e->Index.val, 1);
            fprintf(ALL.out, "._%d", e->Index.index.val.n);
            break;

        case EXPR_DISC: {
            code_expr(env, e->Disc.val, 1);
            fprintf (ALL.out, "._%s",
                e->Disc.subtype.val.s
            );
            break;
        }

        case EXPR_PRED: {
#if XXXXXX
            int isptr = env_tk_isptr(env, &e->Disc.val);
            char* val = ftk(env, &e->Disc.val, 0);
            fe_tmp_set(env, e, NULL);
#endif
            int isnil = (e->Pred.subtype.enu == TX_NULL);

            out("((");
            code_expr(env, e->Pred.val, 1);
            fprintf (ALL.out,
                ".sub == %s) ? (Bool){True} : (Bool){False})\n",
                (isnil ? "NULL" : e->Pred.subtype.val.s)
            );
            break;
        }

        default:
            assert(0);
    }
    if (ctxplain && TP->isalias) {
        out("))");
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_user (Stmt* s) {
    const char* sup = s->User.id.val.s;
    const char* SUP = strupper(s->User.id.val.s);
    int isrec = s->User.isrec;

    // show must receive & from hasalloc, otherwise it would receive ownership
    int hasalloc = env_user_hasalloc(s->env, s);

    // struct Bool;
    // typedef struct Bool Bool;
    // void show_Bool_ (Bool v);
    // Bool clone_Bool_  (Bool v);
    {
        // struct { BOOL sub; union { ... } } Bool;
        fprintf(ALL.out,
            "struct %s;\n"
            "typedef struct %s %s;\n",
            sup, sup, sup
        );

        fprintf(ALL.out,
            "auto %s%s clone_%s%s (%s%s v);\n",
            sup, (hasalloc ? "*" : ""),
            (hasalloc ? "x" : ""), sup,
            sup, (hasalloc ? "*" : "")
        );

        fprintf(ALL.out,
            "auto void show_%s%s_ (%s%s v);\n", // auto: https://stackoverflow.com/a/7319417
            (hasalloc ? "x" : ""),
            sup, sup, (hasalloc ? "*" : "")
        );
    }
    out("\n");

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
            Sub* sub = &s->User.vec[i];
            if (sub->type->sub == TYPE_UNIT) {
                // do not generate anything
            } else {
                out("        ");
                out(to_c(s->env, sub->type));
                out(" _");
                out(sub->id.val.s);      // int _True
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
    if (env_user_hasalloc(s->env,s)) {
        code_free_user(s->env, s);
    }

    // CLONE
    {
        fprintf(ALL.out,
            "%s%s clone_%s%s (%s%s v) {\n",
            sup, (hasalloc ? "*" : ""),
            (hasalloc ? "x" : ""), sup,
            sup, (hasalloc ? "*" : "")
        );
        if (!hasalloc) {
            out("return v;\n");
        } else {
            if (isrec) {
                out (
                    "if (v == NULL) {\n"
                    "   return v;\n"
                    "}\n"
                );
            }
            out("switch (v->sub) {\n");
            for (int i=0; i<s->User.size; i++) {
                Sub* sub = &s->User.vec[i];
                char* id = sub->id.val.s;
                fprintf (ALL.out,
                    "case %s: {\n"
                    "   %s* ret = malloc(sizeof(%s));\n"
                    "   assert(ret!=NULL && \"not enough memory\");\n"
                    "   *ret = (%s) { %s, {._%s=clone_%s%s(v->_%s)} };\n"
                    "   return ret;\n"
                    "}\n",
                    id,
                    sup, sup,
                    sup, id, id,
                    (hasalloc ? "x" : ""), to_ce(sub->type), id
                );
            }
            out("}\n");
        }
        out("assert(0);\n");
        out("}\n");
    }
    out("\n");

    // SHOW
    {
        char* op = (hasalloc ? "->" : ".");

        // _show_Bool (Bool v)
        fprintf(ALL.out,
            "void show_%s%s_ (%s%s v) {\n",
            (hasalloc ? "x" : ""),
            sup, sup, (hasalloc ? "*" : "")
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
            int sub_hasalloc = env_type_hasalloc(s->env, sub->type);

            char arg[1024] = "";
            int yes = 0;
            int par = 0;

            switch (sub->type->sub) {
                case TYPE_UNIT:
                    yes = par = 0;
                    break;
                case TYPE_NATIVE:
                    strcpy(arg,"putchar('?');");
                    break;
                case TYPE_USER:
                    yes = par = 1;
                    sprintf(arg, "show_%s%s_(%sv%s_%s)",
                        (sub->type->isalias || sub_hasalloc ? "x" : ""),
                        sub->type->User.val.s, toptr(s->env,sub->type,"&",""),
                        op, sub->id.val.s);
                    break;
                case TYPE_TUPLE:
                    yes = 1;
                    par = 0;
                    sprintf(arg, "show_%s%s_(%sv%s_%s)",
                        (sub->type->isalias || sub_hasalloc ? "x" : ""),
                        to_ce(sub->type), toptr(s->env,sub->type,"&",""),
                        op, sub->id.val.s);
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
            "void show_%s%s (%s%s v) {\n"
            "    show_%s%s_(v);\n"
            "    puts(\"\");\n"
            "}\n",
            (hasalloc ? "x" : ""), sup, sup, (hasalloc ? "*" : ""),
            (hasalloc ? "x" : ""), sup
        );
    }
}

void code_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_SET:
            assert(0);

        case STMT_NONE:
            break;

        case STMT_SEQ:
            code_stmt(s->Seq.s1);
            code_stmt(s->Seq.s2);
            break;

        case STMT_CALL:
            visit_expr(s->env, s->Call, code_expr_pre);
            code_expr(s->env, s->Call, 0);
            out(";\n");
            break;

        case STMT_USER: {
            if (strcmp(s->User.id.val.s,"Int")) {
                code_user(s);
            }
            break;
        }

        case STMT_VAR: {
            if (s->Var.type->sub == TYPE_UNIT) {
                break;
            }
            visit_type(s->env, s->Var.type, ftp_tuples);
            visit_expr(s->env, s->Var.init, code_expr_pre);

            fprintf(ALL.out, "%s %s", to_c(s->env,s->Var.type), s->Var.id.val.s);

            if (env_type_hasalloc(s->env,s->Var.type)) {
                fprintf (ALL.out,
                    " __attribute__ ((__cleanup__(%s_free)))",
                    to_ce(s->Var.type)
                );
            }

            out(" = ");
            code_expr(s->env, s->Var.init, 0);
            out(";\n");
            break;
        }

        case STMT_RETURN: {
            visit_expr(s->env, s->Return, code_expr_pre);
            out("return ");
            code_expr(s->env, s->Return, 0);
            out(";\n");
            break;
        }

        case STMT_IF: {
            code_expr_pre(s->env, s->If.tst);
            out("if (");
            code_expr(s->env, s->If.tst, 0);
            out(".sub) {\n");
            code_stmt(s->If.true);
            out("} else {\n");
            code_stmt(s->If.false);
            out("}\n");
            break;
        }

        case STMT_FUNC: {
            assert(s->Func.type->sub == TYPE_FUNC);

            if (!strcmp(s->Func.id.val.s,"clone")) break;
            if (!strcmp(s->Func.id.val.s,"show" )) break;

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
                "%s %s (%s %s) {\n",
                (out_isunit ? "void" : tp_out),
                s->Func.id.val.s,
                (inp_isunit ? "void" : tp_inp),
                (inp_isunit ? "" : "_arg_")
            );
            code_stmt(s->Func.body);
            out("}\n");
            break;
        }

        case STMT_BLOCK:
            code_stmt(s->Block);
            break;

        case STMT_NATIVE:
            if (!s->Native.ispre) {
                out(s->Native.tk.val.s);
            }
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
    assert(visit_stmt(s,code_native_pre,NULL,NULL));
    out (
        "#include <assert.h>\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "typedef int Int;\n"
        "#define show_Unit_() printf(\"()\")\n"
        "#define show_Unit()  (show_Unit_(), puts(\"\"))\n"
        "#define show_Int_(x) printf(\"%d\",x)\n"
        "#define show_Int(x)  (show_Int_(x), puts(\"\"))\n"
        "int main (void) {\n"
        "\n"
    );
    code_stmt(s);
    fprintf(ALL.out, "\n");
    out("}\n");
}
