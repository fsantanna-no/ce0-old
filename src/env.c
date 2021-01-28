#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

Type Type_Unit = { TYPE_UNIT, 0 };
Type Type_Bool = { TYPE_USER, 0, .User={TX_USER,{.s="Bool"},0,0} };

Stmt* env_id_to_stmt (Env* env, const char* id) {
    while (env != NULL) {
        const char* cur = NULL;
        switch (env->stmt->sub) {
            case STMT_USER:
                cur = env->stmt->User.tk.val.s;
                break;
            case STMT_VAR:
                cur = env->stmt->Var.tk.val.s;
                break;
            case STMT_FUNC:
                cur = env->stmt->Func.tk.val.s;
                break;
            case STMT_BLOCK:
                // ignore, just a block sentinel
                break;
            default:
                assert(0 && "bug found");
        }
        if (cur!=NULL && !strcmp(id,cur)) {
            return env->stmt;
        }
        env = env->prev;
    }
    return NULL;
}

Stmt* env_stmt_to_func (Stmt* s) {
    Stmt* arg = env_id_to_stmt(s->env, "arg");
    assert(arg != NULL);
    Env* env = arg->env;
    while (env != NULL) {
        if (env->stmt->sub == STMT_FUNC) {
            return env->stmt;
        }
        env = env->prev;
    }
    return NULL;
}

Stmt* env_sub_id_to_user_stmt (Env* env, const char* sub) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            for (int i=0; i<env->stmt->User.size; i++) {
                if (!strcmp(sub, env->stmt->User.vec[i].tk.val.s)) {
                    return env->stmt;
                }
            }
        }
        env = env->prev;
    }
    return NULL;
}

Type* env_sub_id_to_user_type (Env* env, const char* sub) {
    Stmt* user = env_sub_id_to_user_stmt(env, sub);
    assert(user!=NULL && user->sub==STMT_USER);
    for (int i=0; i<user->User.size; i++) {
        if (!strcmp(user->User.vec[i].tk.val.s, sub)) {
            return user->User.vec[i].type;
        }
    }
    return NULL;
}

Sub* env_find_sub (Env* env, const char* sub) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            for (int i=0; i<env->stmt->User.size; i++) {
                if (!strcmp(sub, env->stmt->User.vec[i].tk.val.s)) {
                    return &env->stmt->User.vec[i];
                }
            }
        }
        env = env->prev;
    }
    return NULL;
}

Type* env_expr_to_type (Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_UNK: {
            static Type tp = { TYPE_ANY, 0 };
            return &tp;
        }

        case EXPR_UNIT: {
            static Type tp = { TYPE_UNIT, 0 };
            return &tp;
        }

        case EXPR_NATIVE: {
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type) { TYPE_NATIVE, 0, .Native=e->tk };
            return tp;
        }

        case EXPR_VAR: {
            Stmt* s = env_id_to_stmt(env, e->tk.val.s);
            assert(s != NULL);
            return (s->sub == STMT_VAR) ? s->Var.type : s->Func.type;
        }

        case EXPR_NULL: {
            Stmt* user = env_id_to_stmt(env, e->tk.val.s);
            assert(user != NULL);
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type){ TYPE_USER, 0, .User=user->User.tk };
            return tp;
        }

        case EXPR_INT: {
            static Type tp = { TYPE_USER, 0, .User={TX_USER,{.s="Int"},0,0} };
            return &tp;
        }

        case EXPR_UPREF: {
            Type* tp = env_expr_to_type(env, e->Upref);
            assert(tp != NULL);
            assert(!tp->isptr);
            Type* ret = malloc(sizeof(Expr));
            assert(ret != NULL);
            *ret = *tp;
            ret->isptr = 1;
            return ret;
        }

        case EXPR_DNREF: {
            Type* tp = env_expr_to_type(env, e->Upref);
            assert(tp != NULL);
            assert(tp->isptr);
            Type* ret = malloc(sizeof(Expr));
            assert(ret != NULL);
            *ret = *tp;
            ret->isptr = 0;
            return ret;
        }

        case EXPR_TUPLE: {
            Type** vec = malloc(e->Tuple.size*sizeof(Type));
            assert(vec != NULL);
            for (int i=0; i<e->Tuple.size; i++) {
                vec[i] = env_expr_to_type(env, e->Tuple.vec[i]);
            }
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type) { TYPE_TUPLE, 0, {.Tuple={e->Tuple.size,vec}} };
            return tp;
        }

        case EXPR_CALL: {
            // special cases: clone(), any _f()
            Type* tp = env_expr_to_type(env, e->Call.func);
            if (tp->sub == TYPE_FUNC) {
                assert(e->Call.func->sub == EXPR_VAR);
                if (!strcmp(e->Call.func->tk.val.s,"clone")) {     // returns type of arg
                    Type* ret = malloc(sizeof(Type));
                    assert(ret != NULL);
                    *ret = *env_expr_to_type(env, e->Call.arg);
                    ret->isptr = 0;
                    return ret;
                } else {
                    return tp->Func.out;
                }
            } else {
                assert(e->Call.func->sub == EXPR_NATIVE);           // returns typeof _f(x)
                Type* tp = malloc(sizeof(Type));
                assert(tp != NULL);
                *tp = (Type) { TYPE_NATIVE, 0, .Native={TX_NATIVE,{.s={"TODO"}},0,0} };
                switch (e->Call.arg->sub) {
                    case EXPR_UNIT:
                        sprintf(tp->Native.val.s, "%s()", e->Call.func->tk.val.s);
                        break;
                    case EXPR_VAR:
                    case EXPR_NATIVE:
                        sprintf(tp->Native.val.s, "%s(%s)", e->Call.func->tk.val.s, e->Call.arg->tk.val.s);
                        break;
                    case EXPR_NULL:
                        sprintf(tp->Native.val.s, "%s(NULL)", e->Call.func->tk.val.s);
                        break;
                    case EXPR_INT:
                        sprintf(tp->Native.val.s, "%s(0)", e->Call.func->tk.val.s);
                        break;
                    case EXPR_CALL:
                        // _f _g 1
                        break;
                    default:
                        //assert(0);
                        break;
                }
                return tp;   // TODO: should be typeof(f(arg))
            }
        }

        case EXPR_INDEX: {  // x.1
            Type* tp = env_expr_to_type(env,e->Index.val);
            assert(tp != NULL);
            if (tp->sub == TYPE_NATIVE) {
                return tp;
            }
            assert(tp->sub == TYPE_TUPLE);
            return tp->Tuple.vec[e->tk.val.n-1];
        }

        case EXPR_CONS: {
            Stmt* user = env_sub_id_to_user_stmt(env, e->tk.val.s);
            assert(user != NULL);
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type){ TYPE_USER, 0, .User=user->User.tk };
            return tp;
        }

        case EXPR_PRED:
            return &Type_Bool;

        case EXPR_DISC: {   // x.True
            Type* val = env_expr_to_type(env, e->Disc.val);
            assert(val->sub == TYPE_USER);
            if (e->tk.enu == TX_NULL) {
                assert(!strcmp(e->tk.val.s, val->User.val.s));
                return &Type_Unit;
            }
            Type* tp = env_sub_id_to_user_type(env, e->tk.val.s);
            //assert(!tp->isptr && "bug found : `&´ inside subtype");
            if (!val->isptr) {
                return tp;
            } else if (!env_type_ishasrec(env,tp,0)) {
                // only keep & if sub hasalloc:
                //  &x -> x.True
                return tp;
            } else if (env_type_ishasrec(env,tp,0)) {
                //  &x -> &x.Cons
                Type* ret = malloc(sizeof(Type));
                assert(ret != NULL);
                *ret = *tp;
                ret->isptr = 1;
                return ret;
            }
            assert(0 && "bug found");
        }
    }
    assert(0 && "bug found");
}

//  type Bool { ... }
//  var x: Bool
//  ... x ...           -- give "x" -> "var x" -> type Bool
Stmt* env_type_to_user_stmt (Env* env, Type* tp) {
    if (tp->sub == TYPE_USER) {
        Stmt* s = env_id_to_stmt(env, tp->User.val.s);
        assert(s != NULL);
        return s;
    } else {
        return NULL;
    }
}

int env_type_hasrec (Env* env, Type* tp, int okalias) {
    auto int aux (Env* env, Type* tp, int okalias);
    return (!env_type_isrec(env,tp,0) && aux(env,tp,okalias));

    int aux (Env* env, Type* tp, int okalias) {
        if (!okalias && tp->isptr) {
            return 0;
        }
        switch (tp->sub) {
            case TYPE_ANY:
            case TYPE_UNIT:
            case TYPE_NATIVE:
            case TYPE_FUNC:
                return 0;
            case TYPE_TUPLE:
                for (int i=0; i<tp->Tuple.size; i++) {
                    if (aux(env, tp->Tuple.vec[i], 0)) {
                        return 1;
                    }
                }
                return 0;
            case TYPE_USER: {
                Stmt* user = env_id_to_stmt(env, tp->User.val.s);
                assert(user!=NULL && user->sub==STMT_USER);
                if (user->User.isrec) {
                    return 1;
                }
                for (int i=0; i<user->User.size; i++) {
                    if (aux(env,user->User.vec[i].type, 0)) {
                        return 1;
                    }
                }
                return 0;
            }
        }
        assert(0);
    }

}

int env_type_isrec (Env* env, Type* tp, int okalias) {
    if (!okalias && tp->isptr) {
        return 0;
    }
    switch (tp->sub) {
        case TYPE_ANY:
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_FUNC:
        case TYPE_TUPLE:
            return 0;
        case TYPE_USER: {
            Stmt* user = env_id_to_stmt(env, tp->User.val.s);
            assert(user!=NULL && user->sub==STMT_USER);
            return user->User.isrec;
        }
    }
    assert(0);
}

int env_type_ishasrec (Env* env, Type* tp, int okalias) {
    return env_type_isrec(env,tp,okalias) || env_type_hasrec(env,tp,okalias);
}

int env_type_ishasptr (Env* env, Type* tp) {
    char* users[256];
    int users_n = 0;
    int aux (Env* env, Type* tp) {
        if (tp->isptr) {
            return 1;
        }
        switch (tp->sub) {
            case TYPE_ANY:
            case TYPE_UNIT:
            case TYPE_NATIVE:
            case TYPE_FUNC:
                return 0;
            case TYPE_TUPLE:
                for (int i=0; i<tp->Tuple.size; i++) {
                    if (aux(env,tp->Tuple.vec[i])) {
                        return 1;
                    }
                }
                return 0;
            case TYPE_USER: {
                Stmt* user = env_id_to_stmt(env, tp->User.val.s);
                assert(user!=NULL && user->sub==STMT_USER);
                if (user->User.isrec) {
                    for (int i=0; i<users_n; i++) {
                        if (!strcmp(user->tk.val.s,users[i])) {
                            return 0;
                        }
                    }
                    assert(users_n < 256);
                    users[users_n++] = user->tk.val.s;
                }
                for (int i=0; i<user->User.size; i++) {
                    if (aux(env,user->User.vec[i].type)) {
                        return 1;
                    }
                }
                return 0;
            }
        }
        assert(0);
    }
    return aux(env, tp);
}

Stmt* env_expr_leftmost_decl (Env* env, Expr* e) {
    Expr* left = expr_leftmost(e);
    assert(left != NULL);
    assert(left->sub == EXPR_VAR);
    Stmt* left_decl = env_id_to_stmt(env, left->tk.val.s);
    assert(left_decl != NULL);
    assert(left_decl->sub == STMT_VAR);
    return left_decl;
}

///////////////////////////////////////////////////////////////////////////////

void env_held_vars (Env* env, Expr* e, int* N, Expr** vars, int* uprefs) {
    Type* TP = env_expr_to_type(env, e);
    assert((*N) < 255);

    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_UNK:
        case EXPR_NATIVE:
        case EXPR_NULL:
        case EXPR_INT:
        case EXPR_PRED:
            break;

        case EXPR_VAR: {
            Stmt* s = env_id_to_stmt(env, e->tk.val.s);
            assert(s != NULL);
            if (s->Var.type->isptr) {
                vars[*N] = e;
                uprefs[*N] = 0;
                (*N)++;
            }
            break;
        }

        case EXPR_UPREF: {
            Expr* var = expr_leftmost(e);
            assert(var != NULL);
            assert(var->sub == EXPR_VAR);
            vars[*N] = var;
            uprefs[*N] = 1;
            (*N)++;
            break;
        }

        case EXPR_DNREF: {
            assert(!TP->isptr); // TODO: pointer to pointer? need to hold it
            break;
        }

        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                env_held_vars(env, e->Tuple.vec[i], N, vars, uprefs);
            }
            break;

        case EXPR_DISC:
        case EXPR_INDEX:
            if (env_type_ishasptr(env,TP)) {
                Expr* var = expr_leftmost(e);
                assert(var != NULL);
                assert(var->sub == EXPR_VAR);
                vars[*N] = var;
                uprefs[*N] = 0;
                (*N)++;
            }
            break;

        case EXPR_CALL:
            // arg is held if call returns ishasptr
            if (env_type_ishasptr(env,TP)) {
                env_held_vars(env, e->Call.arg, N, vars, uprefs);
            }
            break;

        case EXPR_CONS: {
            env_held_vars(env, e->Cons, N, vars, uprefs);
            break;
        }

        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////

int set_seqs (Stmt* s) {
    switch (s->sub) {
        case STMT_BLOCK:
            s->seqs[1] = s->Block;
            break;

        case STMT_LOOP:
            s->seqs[1] = s->Loop;
            break;

        case STMT_SEQ:
            s->seqs[1] = s->Seq.s1;
            s->Seq.s1->seqs[0] = s->Seq.s2;

            void aux (Stmt* cur, Stmt* nxt) {
                cur->seqs[0] = nxt;
                switch (cur->sub) {
                    case STMT_BLOCK:
                        aux(cur->Block, nxt);
                        break;
                    case STMT_SEQ:
                        aux(cur->Seq.s2, nxt);
                        break;
                    case STMT_IF:
                        aux(cur->If.true,  nxt);
                        aux(cur->If.false, nxt);
                        break;
                    case STMT_LOOP:
                        aux(cur->Loop, nxt);
                        break;
                    case STMT_FUNC:
                        if (cur->Func.body == NULL) {
                            cur->seqs[1] = nxt;
                        } else {
                            aux(cur->Func.body, nxt);
                        }
                        break;
                    default:
                        cur->seqs[1] = nxt;
                        break;
                }
            }

            aux(s->Seq.s1, s->Seq.s2);
            break;

        case STMT_IF:
            s->seqs[1] = NULL;  // undetermined: left to user code
            break;

        case STMT_FUNC:
            if (s->Func.body != NULL) {
                s->seqs[1] = s->Func.body;
            }
            break;

        default:
            break;
    }
    return VISIT_CONTINUE;
}

///////////////////////////////////////////////////////////////////////////////

int set_envs (Stmt* s) {
    int DEPTH = 0;
    auto int set_envs_ (Stmt* s);
    return visit_stmt(0,s,set_envs_,NULL,NULL);

    int set_envs_ (Stmt* s) {
        s->env = ALL.env;
        switch (s->sub) {
            case STMT_NONE:
            case STMT_RETURN:
            case STMT_SEQ:
            case STMT_IF:
            case STMT_LOOP:
            case STMT_BREAK:
            case STMT_NATIVE:
            case STMT_CALL:
            case STMT_SET:
                break;

            case STMT_VAR: {
                Env* new = malloc(sizeof(Env));     // visit stmt after expr above
                *new = (Env) { s, ALL.env, DEPTH };
                ALL.env = new;
                return VISIT_BREAK;                 // do not visit expr again
            }

            case STMT_USER: {
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, ALL.env, DEPTH };
                ALL.env = new;
                if (s->User.isrec) {
                    s->env = ALL.env;
                }
                break;
            }

            case STMT_BLOCK: {
                Env* save = ALL.env;
                DEPTH++;
                { // dummy node to apply new depth
                    Env* new = malloc(sizeof(Env));     // visit stmt after expr above
                    *new = (Env) { s, ALL.env, DEPTH };
                    ALL.env = new;
                }
                visit_stmt(0,s->Block, set_envs_, NULL, NULL);
                DEPTH--;
                ALL.env = save;
                return VISIT_BREAK;                 // do not re-visit children, I just did them
            }

            case STMT_FUNC: {
                // body of recursive function depends on myself
                {
                    Env* new = malloc(sizeof(Env));
                    *new = (Env) { s, ALL.env, DEPTH };        // put myself
                    ALL.env = new;
                }

                if (s->Func.body != NULL) {
                    Env* save = ALL.env;
                    visit_stmt(0,s->Func.body, set_envs_, NULL, NULL);
                    ALL.env = save;
                }

                return VISIT_BREAK;                 // do not visit children, I just did that
            }
        }
        return 1;
    }
}

///////////////////////////////////////////////////////////////////////////////

int ftk (Env* env, Tk* tk, char* var_type) {
    switch (tk->enu) {
        case TK_UNIT:
        case TX_NATIVE:
        case TX_NUM:
            return 1;

        case TX_NULL:
            var_type = "type";
        case TX_USER:
        case TX_VAR: {
            Stmt* decl = env_id_to_stmt(env, tk->val.s);
            if (decl == NULL) {
                char err[TK_BUF+256];
                sprintf(err, "undeclared %s \"%s\"", var_type, tk->val.s);
                return err_message(tk, err);
            }
            return 1;
        }

        default:
            assert(0 && "TODO");
    }
}

int check_decls_type (Env* env, Type* tp) {
    if (tp->sub == TYPE_USER) {
        return ftk(env, &tp->User, "type");
    }
    return VISIT_CONTINUE;
}

int check_decls_expr (Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_NULL:
            return ftk(env, &e->tk, "type");
        case EXPR_VAR:
            return ftk(env, &e->tk, "variable");

        case EXPR_CALL:
            if (e->Call.func->sub != EXPR_VAR) {
                return VISIT_CONTINUE;
            }
            if (!ftk(env, &e->Call.func->tk, "function")) {
                return 0;
            }
            if (!visit_expr(1,env,e->Call.arg,check_decls_expr)) {
                return 0;
            }
            return VISIT_BREAK;

        case EXPR_CONS: {
            Stmt* user = env_sub_id_to_user_stmt(env, e->tk.val.s);
            if (user == NULL) {
                char err[TK_BUF+256];
                sprintf(err, "undeclared subtype \"%s\"", e->tk.val.s);
                return err_message(&e->tk, err);
            }
            return VISIT_CONTINUE;
        }

        case EXPR_DISC:
        case EXPR_PRED: {
            if (e->tk.enu == TX_NULL) {
                if (!ftk(env, &e->tk, "type")) {
                    return 0;
                }
            } else {
                Stmt* user = env_sub_id_to_user_stmt(env, e->tk.val.s);
                if (user == NULL) {
                    char err[TK_BUF+256];
                    sprintf(err, "undeclared subtype \"%s\"", e->tk.val.s);
                    return err_message(&e->tk, err);
                }
            }
            return VISIT_CONTINUE;
        }

        default:
            return VISIT_CONTINUE;
    }
}

///////////////////////////////////////////////////////////////////////////////

int type_is_sup_sub (Type* sup, Type* sub, int isset) {
    if (sup->sub==TYPE_ANY || sub->sub==TYPE_ANY) {
        return 1;
    }
    if (sup->sub==TYPE_NATIVE || sub->sub==TYPE_NATIVE) {
        return 1;
    }
    if (sup->sub != sub->sub) {
        return 0;   // different TYPE_xxx
    }
    if (sup->isptr != sub->isptr) {
        return 0;
    }
    switch (sup->sub) {
        case TYPE_USER:
            //printf("%s vs %s\n", sup->User.val.s, sub->User.val.s);
            return (sub->User.enu==TK_ERR || !strcmp(sup->User.val.s,sub->User.val.s));
                // TODO: clone

        case TYPE_TUPLE:
            if (sup->Tuple.size != sub->Tuple.size) {
                return 0;
            }
            for (int i=0; i<sup->Tuple.size; i++) {
                if (!type_is_sup_sub(sup->Tuple.vec[i], sub->Tuple.vec[i], 0)) {
                    return 0;
                }
            }
            break;

        default:
            break;
    }
    return 1;
}

int check_types_expr (Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_UNK:
        case EXPR_NATIVE:
        case EXPR_VAR:
        case EXPR_TUPLE:
        case EXPR_NULL:
        case EXPR_INT:
        case EXPR_UPREF:
            break;

        case EXPR_DNREF: {
            Type* tp = env_expr_to_type(env, e->Dnref);
            if (!tp->isptr) {
                return err_message(&e->tk, "invalid `\\´ : expected pointer type");
            }
            break;
        }

        case EXPR_INDEX: {
            Type* tp = env_expr_to_type(env, e->Index.val);
            if (tp->sub!=TYPE_TUPLE || tp->isptr) {
                return err_message(&e->tk, "invalid `.´ : expected tuple type");
            }
            if (0>e->tk.val.n || e->tk.val.n>tp->Tuple.size) {
                return err_message(&e->tk, "invalid `.´ : index is out of range");
            }
            break;
        }

        case EXPR_DISC: {
            Type* tp = env_expr_to_type(env, e->Disc.val);
            if (tp->sub!=TYPE_USER || tp->isptr) {
                return err_message(&e->tk, "invalid `.´ : expected user type");
            }
// TODO: check if user type has subtype
            break;
        }

        case EXPR_PRED: {
            Type* tp = env_expr_to_type(env, e->Pred.val);
            if (tp->sub!=TYPE_USER || tp->isptr) {
                return err_message(&e->tk, "invalid `.´ : expected user type");
            }
// TODO: check if user type has subtype
            break;
        }

        case EXPR_CALL: {
            Type* func = env_expr_to_type(env, e->Call.func);
            Type* arg  = env_expr_to_type(env, e->Call.arg);

            if (e->Call.func->sub == EXPR_NATIVE) {
                TODO("TODO [check_types]: _f(...)\n");
            } else {
                assert(e->Call.func->sub == EXPR_VAR);
                if (!strcmp(e->Call.func->tk.val.s,"clone")) {
                    TODO("TODO [check_types]: clone(...)\n");
                } else if (!strcmp(e->Call.func->tk.val.s,"std")) {
                    TODO("TODO [check_types]: std(...)\n");
                } else if (!type_is_sup_sub(func->Func.inp, arg, 0)) {
                    char err[TK_BUF+256];
                    sprintf(err, "invalid call to \"%s\" : type mismatch", e->Call.func->tk.val.s);
                    return err_message(&e->Call.func->tk, err);
                }
            }
            break;
        }

        case EXPR_CONS: {
            Sub* sub = env_find_sub(env, e->tk.val.s);
            assert(sub != NULL);
            if (!type_is_sup_sub(sub->type, env_expr_to_type(env,e->Cons), 0)) {
                char err[TK_BUF+256];
                sprintf(err, "invalid constructor \"%s\" : type mismatch", e->tk.val.s);
                return err_message(&e->tk, err);
            }
        }
    }
    return 1;
}

int check_types_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_NONE:
        case STMT_USER:
        case STMT_FUNC:
        case STMT_SEQ:
        case STMT_LOOP:
        case STMT_BREAK:
        case STMT_BLOCK:
        case STMT_NATIVE:
        case STMT_CALL:
            return 1;

        case STMT_VAR:
            if (!type_is_sup_sub(s->Var.type, env_expr_to_type(s->env,s->Var.init), 0)) {
                char err[TK_BUF+256];
                sprintf(err, "invalid assignment to \"%s\" : type mismatch", s->Var.tk.val.s);
                return err_message(&s->Var.tk, err);
            }
            return 1;

        case STMT_SET:
            if (!type_is_sup_sub(env_expr_to_type(s->env,s->Set.dst), env_expr_to_type(s->env,s->Set.src), 1)) {
                char err[TK_BUF+256];
                if (s->Set.dst->sub == EXPR_VAR) {
                    sprintf(err, "invalid assignment to \"%s\" : type mismatch", s->Set.dst->tk.val.s);
                    return err_message(&s->Set.dst->tk, err);
                } else {
                    strcpy(err, "invalid assignment : type mismatch");
                    return err_message(&s->tk, err);
                }
            }
            return 1;

        case STMT_IF:
            if (!type_is_sup_sub(&Type_Bool, env_expr_to_type(s->env,s->If.tst), 0)) {
                return err_message(&s->tk, "invalid condition : type mismatch");
            }
            return 1;

        case STMT_RETURN: {
            Stmt* func = env_stmt_to_func(s);
            assert(func != NULL);
            assert(func->Func.type->sub == TYPE_FUNC);
            if (!type_is_sup_sub(func->Func.type->Func.out, env_expr_to_type(s->env,s->Return), 0)) {
                return err_message(&s->tk, "invalid return : type mismatch");
            }
            return 1;
        }
    }
    assert(0);
}

///////////////////////////////////////////////////////////////////////////////

// Rule 7:
// set DST = \SRC
// return \SRC
//  - check if scope of DST<=S

void ptr_or_pln (Stmt* dst, Stmt* src, int isup) {
    if (isup) {
        // acquired w/ =\x: points exactly to x which is in the deepest possible scope
        //assert(dst->Var.ptr_deepest==NULL || src->Var.ptr_deepest->env->depth>dst->Var.ptr_deepest->env->depth);
        dst->Var.ptr_deepest = src;
    } else {
        // acquired w/ =x: points to something that x points, take that scope, not the scope of x
        if (dst->Var.ptr_deepest==NULL || src->Var.ptr_deepest->env->depth>dst->Var.ptr_deepest->env->depth) {
            dst->Var.ptr_deepest = src->Var.ptr_deepest;
        }
    }
}

int check_ptrs_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_VAR: {
            if (env_type_ishasptr(s->env,s->Var.type) && strcmp(s->Var.tk.val.s,"arg")) {
                s->Var.ptr_deepest = NULL;
            } else {
                s->Var.ptr_deepest = s;
            }

            int n=0; Expr* vars[256]; int uprefs[256];
            env_held_vars(s->env, s->Var.init, &n, vars, uprefs);
            for (int i=0; i<n; i++) {
                Stmt* src = env_id_to_stmt(s->env, vars[i]->tk.val.s);
                assert(src!=NULL && src->sub==STMT_VAR);
                ptr_or_pln(s, src, uprefs[i]);
            }

            // var x: \Int = ?
            if (s->Var.ptr_deepest == NULL) {
                s->Var.ptr_deepest = s; // assumes ? has the same scope of x
            }
            break;
        }

        case STMT_SET: {
            Type* tp = env_expr_to_type(s->env, s->Set.dst);
            if (!env_type_ishasptr(s->env,tp)) {
                break;
            }

            Stmt* dst = env_expr_leftmost_decl(s->env, s->Set.dst);
            assert(dst->sub == STMT_VAR);
            int dst_depth = dst->Var.ptr_deepest->env->depth;

            {
                int n=0; Expr* vars[256]; int uprefs[256];
                env_held_vars(s->env, s->Set.src, &n, vars, uprefs);
                for (int i=0; i<n; i++) {
                    Stmt* src = env_id_to_stmt(s->env, vars[i]->tk.val.s);
                    assert(src!=NULL && src->sub==STMT_VAR && src->Var.ptr_deepest!=NULL);
                    ptr_or_pln(dst, src, uprefs[i]);
                }
            }

            Tk* src_var = &dst->Var.ptr_deepest->Var.tk;

            if (dst_depth < dst->Var.ptr_deepest->env->depth) {
                char err[TK_BUF+256];
                sprintf(err, "invalid assignment : cannot hold pointer \"%s\" (ln %d) in outer scope",
                        src_var->val.s, src_var->lin);
                err_message(&s->tk, err);
                return EXEC_ERROR;
            }
            break;
        }

        case STMT_RETURN: {
            Type* tp = env_expr_to_type(s->env, s->Return);
            if (!env_type_ishasptr(s->env,tp)) {
                return EXEC_CONTINUE;
            }

            Expr* src      = expr_leftmost(s->Return);
            Stmt* src_decl = env_expr_leftmost_decl(s->env, s->Return);
            assert(src_decl->Var.ptr_deepest != NULL);

        //printf("ret=%d vs src=%d\n", s->env->depth, src_decl->Var.ptr_deepest->env->depth);
            if (s->env->depth <= src_decl->Var.ptr_deepest->env->depth) {
                char err[TK_BUF+256];
                sprintf(err, "invalid return : cannot return local pointer \"%s\" (ln %d)",
                        src->tk.val.s, src_decl->tk.lin);
                err_message(&src->tk, err);
                return EXEC_ERROR;
            }
            break;
        }

        default:
            break;
    }
    return EXEC_CONTINUE;
}

int check_ptrs_expr (Env* env, Expr* e) {
    if (e->sub != EXPR_TUPLE) {
        return EXEC_CONTINUE;
    }

    // EXPR_TUPLE

    int n=0; Expr* vars[256]; int uprefs[256];
    env_held_vars(env, e, &n, vars, uprefs);
    int depth = -1;
puts("-=-=-=-=-");
dump_expr(e); puts(" $$$");
    for (int i=0; i<n; i++) {
        Stmt* var = env_expr_leftmost_decl(env, vars[i]);
        assert(var!=NULL && var->sub==STMT_VAR);
#if 0
            Stmt* dcl = env_id_to_stmt(env, e->tk.val.s);
            assert(dcl != NULL);
            if (dcl->Var.type->isptr) {
                vars[(*vars_n)++] = e;
            }
#endif
dump_expr(vars[i]);
        int tmp = (uprefs[i] ? var->env->depth : var->Var.ptr_deepest->env->depth);
printf("  >>> %d  --  %s\n", depth, var->tk.val.s);
        if (depth!=-1 && depth!=tmp) {
dump_expr(vars[i]);
printf("  <<< %d  --  %s\n", tmp, var->tk.val.s);
            err_message(&e->tk, "invalid tuple : pointers with different scopes");
            return EXEC_ERROR;
        }
        depth = tmp;
dump_expr(vars[i]);
printf("  <<< %d  --  %s\n", depth, var->tk.val.s);
    }
    return EXEC_CONTINUE;
}

///////////////////////////////////////////////////////////////////////////////

int env (Stmt* s) {
    assert(visit_stmt(0,s,set_seqs,NULL,NULL));
    assert(set_envs(s));
    if (!visit_stmt(0,s,NULL,check_decls_expr,check_decls_type)) {
        return 0;
    }
    if (!visit_stmt(1,s,check_types_stmt,check_types_expr,NULL)) {
        return 0;
    }
    if (!exec(s,NULL,check_ptrs_stmt,check_ptrs_expr)) {
        return 0;
    }
    return 1;
}
