void code_free_user (Stmt* s) {
    assert(s->sub == STMT_USER);
    assert(env_user_ishasrec(s->env,s));

    const char* sup = s->User.tk.val.s;

    fprintf (ALL.out,
        "void %s_free (struct %s** p) {\n"
        "   if (*p == NULL) { return; }\n",
        sup, sup
    );

    out("switch ((*p)->sub) {\n");
    for (int i=0; i<s->User.size; i++) {
        Sub sub = s->User.vec[i];
        if (env_type_ishasrec(s->env, sub.type)) {
            fprintf (ALL.out,
                "case %s:\n"
                "   %s_free(&(*p)->_%s);\n"
                "   break;\n",
                sub.tk.val.s,
                to_ce(sub.type), sub.tk.val.s
            );
        }
    }
    out (
        "       default:\n"
        "           break;\n"
        "   }\n"
        "   free(*p);\n"
        "}\n"
    );
}

void code_free_tuple (Env* env, Type* tp) {
    assert(tp->sub == TYPE_TUPLE);
    assert(env_type_ishasrec(env,tp));

    char* tp_ = to_ce(tp);
    fprintf (ALL.out,
        "void %s_free (%s** p) {\n"
        "   if (*p == NULL) { return; }\n",
        tp_, tp_
    );
    for (int i=0; i<tp->Tuple.size; i++) {
        Type* sub = tp->Tuple.vec[i];
        if (env_type_ishasrec(env,sub)) {
            fprintf (ALL.out,
                "    %s_free(&(*p)->_%d);\n",
                to_ce(sub), i+1
            );
        }
    }
    out("   free(*p);\n"
        "}\n");
}

///////////////////////////////////////////////////////////////////////////////

void code_clone_user (Stmt* s) {
    assert(s->sub == STMT_USER);
    assert(env_user_ishasrec(s->env,s));

    const char* sup = s->User.tk.val.s;

    fprintf(ALL.out,
        "%s* clone_%s (%s** v) {\n",
        sup, sup, sup
    );
    out (
        "if ((*v) == NULL) {\n"
        "   return NULL;\n"
        "}\n"
        "switch ((*v)->sub) {\n"
    );
    for (int i=0; i<s->User.size; i++) {
        Sub* sub = &s->User.vec[i];
        char* sub_id = sub->tk.val.s;
        char* sub_tp = to_ce(sub->type);
        int sub_ishasrec = env_type_ishasrec(s->env, sub->type);
        fprintf(ALL.out, "case %s:\n", sub_id);

        char clone[256];
        if (sub->type->sub == TYPE_UNIT) {
            assert(!sub->type->isptr);  // TODO: refuse \() or \((),()), etc
            strcpy(clone, "");
        } else if (sub_ishasrec) {
            sprintf(clone, "._%s=clone_%s(&(*v)->_%s)", sub_id, sub_tp, sub_id);
        } else {
            sprintf(clone, "._%s=(*v)->_%s", sub_id, sub_id);
        }

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
    }
    out("}\n");
    out("assert(0);\n");
    out("}\n");
}

void code_clone_tuple (Env* env, Type* tp) {
    assert(tp->sub == TYPE_TUPLE);
    assert(env_type_ishasrec(env,tp));

    char* tp_ = to_ce(tp);
    fprintf (ALL.out,
        "%s* clone_%s (%s** v) {\n"
        "   %s* ret = malloc(sizeof(%s));\n"
        "   assert(ret!=NULL && \"not enough memory\");\n"
        "   *ret = (%s) { ",
        tp_, tp_, tp_,
        tp_, tp_,
        tp_
    );
    for (int i=0; i<tp->Tuple.size; i++) {
        Type* sub = tp->Tuple.vec[i];
        if (env_type_ishasrec(env,sub)) {
            fprintf (ALL.out,
                "%s clone_%s(&(*v)->_%d)",
                (i != 0 ? "," : ""),
                to_ce(sub),
                i+1
            );
        } else {
            fprintf(ALL.out, "%s (*v)->_%d\n", (i != 0 ? "," : ""), i+1);
        }
    }
    out (
        " };\n"
        "   return ret;\n"
        "}\n"
    );
}

///////////////////////////////////////////////////////////////////////////////

void code_stdout_user (Stmt* s) {
    const char* sup = s->User.tk.val.s;
    int ishasrec = env_user_ishasrec(s->env, s);

    // _stdout_Bool (Bool v)
    fprintf(ALL.out,
        "void stdout_%s_ (%s%s v) {\n",
        sup, sup, (ishasrec ? "**" : "")
    );
    if (ishasrec) {
        out (
            "if ((*v) == NULL) {\n"
            "    printf(\"$\");\n"
            "    return;\n"
            "}\n"
        );
    }

    char* v = (ishasrec ? "(*v)->" : "v.");

    fprintf(ALL.out, "    switch (%ssub) {\n", v);
    for (int i=0; i<s->User.size; i++) {
        Sub* sub = &s->User.vec[i];

        char arg[TK_BUF*2+256] = "";
        int yes = 0;
        int par = 0;

        char* op2 = ""; {
            int ishasrec = env_type_ishasrec(s->env, sub->type);
            int isptr = sub->type->isptr;
            sub->type->isptr = 0;
            int ishasrec2 = env_type_ishasrec(s->env, sub->type);
            sub->type->isptr = isptr;
            if (ishasrec) {
                op2 = "&";
            } else if (!ishasrec && isptr && !ishasrec2) {
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
        sup, sup, (ishasrec ? "**" : ""),
        sup
    );
}

void code_stdout_tuple (Env* env, Type* tp) {
    int ishasrec = env_type_ishasrec(env, tp);

    char tp_c [256];
    char tp_ce[256];
    strcpy(tp_c,  to_c (env,tp));
    strcpy(tp_ce, to_ce(tp));

    fprintf (ALL.out,
        "#ifndef __stdout_%s__\n"
        "#define __stdout_%s__\n",
        tp_ce, tp_ce
    );
    fprintf(ALL.out,
        "void stdout_%s_ (%s%s v) {\n"
        "    printf(\"(\");\n",
        tp_ce, tp_c, (ishasrec ? "*" : "")
    );
    for (int i=0; i<tp->Tuple.size; i++) {
        if (i > 0) {
            fprintf(ALL.out, "printf(\",\");\n");
        }
        Type* sub = tp->Tuple.vec[i];

        char* op2 = ""; {
            int ishasrec = env_type_ishasrec(env, sub);
            if (ishasrec) {
                op2 = "&";
            } else if (!ishasrec && sub->isptr) {
                op2 = "*";
            }
        }

        if (sub->isptr) {
            fprintf(ALL.out, "putchar('@');\n");
        } else if (sub->sub == TYPE_NATIVE) {
            fprintf(ALL.out, "putchar('?');\n");
        } else if (sub->sub == TYPE_UNIT) {
            fprintf(ALL.out, "stdout_Unit_();\n");
        } else {
            fprintf(ALL.out, "stdout_%s_(%s%s_%d);\n",
                to_ce(sub), op2, (ishasrec ? "(*v)->" : "v."), i+1);
        }
    }
    fprintf(ALL.out,
        "    printf(\")\");\n"
        "}\n"
        "void stdout_%s (%s%s v) {\n"
        "    stdout_%s_(v);\n"
        "    puts(\"\");\n"
        "}\n",
        tp_ce, tp_c, (ishasrec ? "*" : ""),
        tp_ce
    );
    out("#endif\n");
}
