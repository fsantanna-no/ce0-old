#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

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
    if (arg == NULL) {
        return NULL;
    }
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

///////////////////////////////////////////////////////////////////////////////

void env_expr_to_type_free (Type** tp) {
    switch ((*tp)->sub) {
        case TYPE_ANY:
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_USER:
            break;
        case TYPE_TUPLE:
            for (int i=0; i<(*tp)->Tuple.size; i++) {
                env_expr_to_type_free(&(*tp)->Tuple.vec[i]);
            }
            free((*tp)->Tuple.vec);
            break;
        case TYPE_FUNC:
            env_expr_to_type_free(&(*tp)->Func.inp);
            env_expr_to_type_free(&(*tp)->Func.out);
            break;
    }
    free(*tp);
}

Type* deep (Type* tp);

void deep_ (Type* ret, Type* tp) {
    *ret = *tp;
    switch (tp->sub) {
        case TYPE_ANY:
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_USER:
            break;
        case TYPE_TUPLE: {
            Type** vec = malloc(tp->Tuple.size*sizeof(Type));
            assert(vec != NULL);
            for (int i=0; i<tp->Tuple.size; i++) {
                vec[i] = deep(tp->Tuple.vec[i]);
            }
            ret->Tuple.vec = vec;
            break;
        }
        case TYPE_FUNC: {
            ret->Func.inp = deep(tp->Func.inp);
            ret->Func.out = deep(tp->Func.out);
            break;
        }
    }
}

Type* deep (Type* tp) {
    Type* ret = malloc(sizeof(Type));
    assert(ret != NULL);
    deep_(ret, tp);
    return ret;
}

void env_expr_to_type_ (Type* ret, Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_UNK:
            *ret = (Type) { TYPE_ANY, 0 };
            break;
        case EXPR_UNIT:
            *ret = (Type) { TYPE_UNIT, 0 };
            break;
        case EXPR_NATIVE:
            *ret = (Type) { TYPE_NATIVE, 0, .Native=e->tk };
            break;
        case EXPR_VAR: {
            Stmt* s = env_id_to_stmt(env, e->tk.val.s);
            assert(s != NULL);
            deep_(ret, (s->sub == STMT_VAR) ? s->Var.type : s->Func.type);
            break;
        }
        case EXPR_NULL: {
            Stmt* user = env_id_to_stmt(env, e->tk.val.s);
            assert(user != NULL);
            *ret = (Type) { TYPE_USER, 0, .User=user->User.tk };
            break;
        }
        case EXPR_INT:
            *ret = (Type) { TYPE_USER, 0, .User={TX_USER,{.s="Int"},0,0} };
            break;
        case EXPR_PRED:
            *ret = (Type) { TYPE_USER, 0, .User={TX_USER,{.s="Bool"},0,0} };
            break;
        case EXPR_UPREF:
            env_expr_to_type_(ret, env, e->Upref);
            assert(!ret->isptr);
            ret->isptr = 1;
            break;
        case EXPR_DNREF:
            env_expr_to_type_(ret, env, e->Upref);
            assert(ret->isptr);
            ret->isptr = 0;
            break;
        case EXPR_TUPLE: {
            Type** vec = malloc(e->Tuple.size*sizeof(Type));
            assert(vec != NULL);
            for (int i=0; i<e->Tuple.size; i++) {
                vec[i] = env_expr_to_type(env, e->Tuple.vec[i]);
            }
            *ret = (Type) { TYPE_TUPLE, 0, {.Tuple={e->Tuple.size,vec}} };
            break;
        }
        case EXPR_CONS: {
            Stmt* user = env_sub_id_to_user_stmt(env, e->tk.val.s);
            assert(user != NULL);
            *ret = (Type) { TYPE_USER, 0, .User=user->User.tk };
            break;
        }

        case EXPR_CALL: {
            // special cases: clone(), move(), any _f()
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e->Call.func);
            if (tp->sub == TYPE_FUNC) {
                assert(e->Call.func->sub == EXPR_VAR);
                if (!strcmp(e->Call.func->tk.val.s,"clone") ||
                    !strcmp(e->Call.func->tk.val.s,"move")) {   // returns type of arg
                    env_expr_to_type_(ret, env, e->Call.arg);
                    ret->isptr = 0;
                } else {
                    deep_(ret, tp->Func.out);
                }
            } else {
                assert(e->Call.func->sub == EXPR_NATIVE);       // returns typeof _f(x)
                *ret = (Type) { TYPE_NATIVE, 0, .Native={TX_NATIVE,{.s={"TODO"}},0,0} };
                switch (e->Call.arg->sub) {
                    case EXPR_UNIT:
                        sprintf(ret->Native.val.s, "%s()", e->Call.func->tk.val.s);
                        break;
                    case EXPR_VAR:
                    case EXPR_NATIVE:
                        sprintf(ret->Native.val.s, "%s(%s)", e->Call.func->tk.val.s, e->Call.arg->tk.val.s);
                        break;
                    case EXPR_NULL:
                        sprintf(ret->Native.val.s, "%s(NULL)", e->Call.func->tk.val.s);
                        break;
                    case EXPR_INT:
                        sprintf(ret->Native.val.s, "%s(0)", e->Call.func->tk.val.s);
                        break;
                    case EXPR_CALL:
                        // _f _g 1
                        break;
                    default:
                        //assert(0);
                        break;
                }
            }
            break;
        }

        case EXPR_INDEX: {  // x.1
            Type* tp = env_expr_to_type(env, e->Index.val);
            if (tp->sub == TYPE_NATIVE) {
                deep_(ret, tp);
                env_expr_to_type_free(&tp);
            } else {
                assert(tp->sub == TYPE_TUPLE);
                deep_(ret, tp->Tuple.vec[e->tk.val.n-1]);
                env_expr_to_type_free(&tp);
            }
            break;
        }

        case EXPR_DISC: {   // x.True
            Type* val __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e->Disc.val);
            assert(val->sub == TYPE_USER);
            if (e->tk.enu == TX_NULL) {
                assert(!strcmp(e->tk.val.s, val->User.val.s));
                *ret = (Type) { TYPE_UNIT, 0 };
                break;
            }

            deep_(ret, env_sub_id_to_user_type(env, e->tk.val.s));
            if (!val->isptr) {
                break;
            } else if (!env_type_ishasrec(env,ret)) {
                // only keep & if sub hasalloc:
                //  &x -> x.True
                break;
            } else {
                //  &x -> &x.Cons
                ret->isptr = 1;
                break;
            }
        }
    }
}

Type* env_expr_to_type (Env* env, Expr* e) {
    Type* ret = malloc(sizeof(Type));
    assert(ret != NULL);
    env_expr_to_type_(ret, env, e);
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

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

int env_type_ishasrec (Env* env, Type* tp) {
    if (tp->isptr) {
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
                if (env_type_ishasrec(env, tp->Tuple.vec[i])) {
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
                if (env_type_ishasrec(env,user->User.vec[i].type)) {
                    return 1;
                }
            }
            return 0;
        }
    }
    assert(0);
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
    Type* TP __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e);
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
            Type* tp __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env,var);
            uprefs[*N] = !tp->isptr; // \x (upref) vs \x\.y! (!upref)
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

void txs_txed_vars (Env* env, Expr* e, F_txed_vars f) {
    Type* TP __ENV_EXPR_TO_TYPE_FREE__ = env_expr_to_type(env, e);
    if (!env_type_ishasrec(env,TP)) {
        return;
    }

    Expr* e_ = e;
    if (e->sub==EXPR_CALL && e->Call.func->sub==EXPR_VAR && !strcmp(e->Call.func->tk.val.s,"move")) {
        e = e->Call.arg;
    }

    f(e_, e);

    switch (e->sub) {
        case EXPR_VAR:
        case EXPR_NULL:
        case EXPR_DNREF:
        case EXPR_DISC:
        case EXPR_INDEX:
            break;
        case EXPR_UNIT:
        case EXPR_UNK:
        case EXPR_NATIVE:
        case EXPR_INT:
        case EXPR_PRED:
        case EXPR_UPREF:
            assert(0);      // cannot be ishasrec
            break;

        case EXPR_TUPLE:
            for (int i=0; i<e->Tuple.size; i++) {
                txs_txed_vars(env, e->Tuple.vec[i], f);
            }
            break;

        case EXPR_CALL: // tx for args is checked elsewhere (here, only if return type is also ishasrec)
            // func may transfer its argument back
            // set x.2 = f(x) where f: T1->T2 { return arg.2 }
            txs_txed_vars(env, e->Call.arg, f);
            break;

        case EXPR_CONS: {
            txs_txed_vars(env, e->Cons, f);
            break;
        }

        default:
            break;
    }
}
