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
            strcat(out, &tp->tk.val.s[1]);
            break;
        case TYPE_NIL:
            strcat(out, "Nil");
            break;
        case TYPE_USER: {
            strcat(out, tp->tk.val.s);
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
            strcat(out, &tp->tk.val.s[1]);
            break;
        case TYPE_NIL:
            strcat(out, "void*");
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

        // create mock types TUPLE__NIL__*
        // TUPLE_NAT_NAT
        // TUPLE_NIL_NAT
        // TUPLE_NAT_NIL
        for (int i=0; i<tp->Tuple.size; i++) {
            Type* it = &tp->Tuple.vec[i];
            if (it->sub == TYPE_USER) {
                Stmt* s = env_find_decl(it->env, it->tk.val.s);
                assert(s!=NULL && s->sub==STMT_USER);
                if (s->User.isrec) {
                    Type tmp = *tp;
                    Type vec[tp->Tuple.size];
                    memcpy(vec, tp->Tuple.vec, sizeof(vec));
                    vec[i] = (Type) { TYPE_NIL, tmp.env };
                    tmp.Tuple.vec = vec;
                    //ft(&tmp);
                    fprintf(ALL.out, "#ifndef __%s__\n", to_ce(&tmp));
                    fprintf(ALL.out, "#define __%s__\n", to_ce(&tmp));
                    fprintf(ALL.out, "typedef %s %s;\n", tp_ce, to_ce(&tmp));
                    fprintf(ALL.out, "#endif\n");
                }
            }
        }

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
        case EXPR_CONS: {
            Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
            assert(user != NULL);
            fprintf(ALL.out, "_%d", e->N);
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

                int isrec = 0;

                if (!strcmp(e->Call.func->tk.val.s,"output")) {
                    out("output_");
                    code_to_ce(env_expr_type(e->Call.arg));
                } else {
                    visit_expr(e->Call.func, fe_1);

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
                visit_expr(e->Call.arg, fe_1);
                out(")");
            }
            return 0;
        }

        case EXPR_TUPLE:
            out("((");
            out(to_c(env_expr_type(e)));
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
            Type* tp = env_expr_type(e->Disc.val);          // Bool
            Stmt* s  = env_find_decl(e->env, tp->tk.val.s); // type Bool { ... }
            visit_expr(e->Disc.val, fe_1);
            fprintf(ALL.out, "%s_%s", (s->User.isrec ? "->" : "."), e->Disc.sub.val.s);
            return 0;
        }

        case EXPR_PRED: {
            Type* tp = env_expr_type(e->Pred.val);          // Bool
            Stmt* s  = env_find_decl(e->env, tp->tk.val.s); // type Bool { ... }
            int isnil = (e->Pred.sub.enu == TK_NIL);
            out("((");
            visit_expr(e->Pred.val, fe_1);
            fprintf(ALL.out, "%s == %s) ? (Bool){True,{._True=1}} : (Bool){False,{._False=1}})",
                (isnil ? "" : (s->User.isrec ? "->sub" : ".sub")),
                (isnil ? "NULL" : e->Pred.sub.val.s)
            );
            return 0;
        }
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

// tuple structs pre declarations
// cons structs pre allocations
// user structs pre declarations

int fe_0 (Expr* e) {
    if (e->sub == EXPR_TUPLE) {
        for (int i=0; i<e->Tuple.size; i++) {
            visit_expr(&e->Tuple.vec[i], fe_0); // first visit child
        }
        ft(env_expr_type(e));                   // second visit myself
        return 0;

    } else if (e->sub == EXPR_CONS) {
        visit_expr(e->Cons.arg, fe_0);          // first visit child

        Stmt* user = env_find_super(e->env, e->Cons.sub.val.s);
        assert(user != NULL);

        char* sup = user->User.id.val.s;
        char* sub = e->Cons.sub.val.s;

        // Bool __1 = (Bool) { False, {_False=1} };
        // Nat  __1 = (Nat)  { Succ,  {_Succ=&_2} };
        fprintf(ALL.out,
            "%s %s_%d = ((%s) { %s, { ._%s=",
            sup, (user->User.isrec ? "_" : ""), e->N, sup, sub, sub);
        visit_expr(e->Cons.arg, fe_1);
        out(" } });\n");

        if (e->Cons.ispool) {
            assert(user->User.isrec && "bug found");

            // Nat* _1 = (Nat*) pool_alloc(pool, sizeof(Nat));
            // *_1 = __1;
            fprintf(ALL.out,
                "%s* _%d = (%s*) pool_alloc(_pool, sizeof(%s));\n"
                "assert(_%d!=NULL && \"TODO\");\n"
                "*_%d = __%d;\n",
                    sup, e->N, sup, sup, e->N, e->N, e->N);
        } else if (user->User.isrec) {
            // Bool* _1 = &__1;
            fprintf(ALL.out, "%s* _%d = &__%d;\n", sup, e->N, e->N);
        } else {
            // plain cons: nothing else to do
        }

        return 0;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void code_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_USER:
            for (int i=0; i<s->User.size; i++) {        // first generate tuples
                visit_type(&s->User.vec[i].type, ft);
            }

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
                    out(to_c(&sub.type));
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

            // CLEAN
            if (s->User.isrec) {
                fprintf (ALL.out,
                    "void %s_free (struct %s** p) {\n"
                    "    if (*p == NULL) { return; }\n",
                    sup, sup
                );

                for (int i=0; i<s->User.size; i++) {
                    Sub sub = s->User.vec[i];
                    if (sub.type.sub==TYPE_USER && (!strcmp(sup,sub.type.tk.val.s))) {
                        fprintf (ALL.out,
                            "    %s_free(&(*p)->_%s);\n"
                            "    if (!BET((long)_STACK,(long)*p,(long)&p)) {\n"
                            "       free(*p);\n"
                            "    }\n",
                            sup, sub.id.val.s
                        );
                    }
                }

                out("}\n");
            }

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

        case STMT_VAR: {
            visit_type(&s->Var.type, ft);

            char sup[256];
            strcpy(sup, to_ce(&s->Var.type));

            if (s->Var.pool == 0) {
                // no pool
            } else if (s->Var.pool == -1) {
                out("Pool* _pool = NULL;\n");
            } else {
                fprintf (ALL.out,
                    "%s _buf[%d];\n"
                    "Pool _%d = { _buf,sizeof(_buf),0 };\n"
                    "Pool* _pool = &_%d;\n",
                    sup, s->Var.pool, s->N, s->N
                );
            }

            visit_expr(&s->Var.init, fe_0);

            out(to_c(&s->Var.type));
            fputs(" ", ALL.out);
            fputs(s->Var.id.val.s, ALL.out);
            if (s->Var.pool == -1) {
                fprintf (ALL.out,
                    " __attribute__ ((__cleanup__(%s_free))) ",
                    sup
                );
            }
            fputs(" = ", ALL.out);

            visit_expr(&s->Var.init, fe_1);
            out(";\n");
            break;
        }

        case STMT_CALL:
            visit_expr(&s->ret, fe_0);
            visit_expr(&s->ret, fe_1);
            out(";\n");
            break;

        case STMT_RETURN:
            visit_expr(&s->ret, fe_0);
            out("return ");
            visit_expr(&s->ret, fe_1);
            out(";\n");
            break;

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
                (isrec ? "Pool* _pool," : ""),
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
        "#define MIN(x,y)   (((x) < (y)) ? (x) : (y))\n"
        "#define MAX(x,y)   (((x) > (y)) ? (x) : (y))\n"
        "#define BET(x,y,z) (MIN(x,z)<y && y<MAX(x,z))\n"
        "#define output_Unit_(x) (assert(((long)(x))==1), printf(\"()\"))\n"
        "#define output_Unit(x)  (output_Unit_(x), puts(\"\"))\n"
        "#define output_Nil_(x)  assert(0 && \"bug found\")\n"
        "#define output_Nil(x)   assert(0 && \"bug found\")\n"
        "typedef struct {\n"
        "    void* buf;\n"      // stack-allocated buffer
        "    int max;\n"        // maximum size
        "    int cur;\n"        // current size
        "} Pool;\n"
        "void* pool_alloc (Pool* pool, int n) {\n"
        "    if (pool == NULL) {\n"                     // pool is unbounded
        "        return malloc(n);\n"                   // fall back to `malloc`
        "    } else {\n"
        "        void* ret = pool->buf + pool->cur;\n"
        "        pool->cur += n;\n"                     // nodes are allocated sequentially
        "        if (pool->cur <= pool->max) {\n"
        "            return ret;\n"
        "        } else {\n"
        "            return NULL;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "int main (void) {\n"
        "    void* _STACK = &_STACK;\n"
        "\n"
    );
    code_stmt(s);
    fprintf(ALL.out, "\n");
    out("}\n");
}
