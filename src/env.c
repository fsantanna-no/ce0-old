#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

Type Type_Bool = { TYPE_USER, 0, .User={TX_USER,{.s="Bool"},0,0} };

int err_message (Tk* tk, const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): %s", tk->lin, tk->col, v);
    return 0;
}

Stmt* env_id_to_stmt (Env* env, const char* id, int* scope) {
    if (scope != NULL) {
        *scope = 0;
    }
    while (env != NULL) {
        const char* cur = NULL;
        switch (env->stmt->sub) {
            case STMT_USER:
                cur = env->stmt->User.id.val.s;
                break;
            case STMT_VAR:
                cur = env->stmt->Var.id.val.s;
                break;
            case STMT_FUNC:
                cur = env->stmt->Func.id.val.s;
                break;
            default:
                assert(0 && "bug found");
        }
        if (scope!=NULL && !strcmp("arg",cur)) {
            *scope = *scope + 1;
        }
        if (cur!=NULL && !strcmp(id,cur)) {
            return env->stmt;
        }
        env = env->prev;
    }
    return NULL;
}

Stmt* env_stmt_to_func (Stmt* s) {
    Stmt* arg = env_id_to_stmt(s->env, "arg", NULL);
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
                if (!strcmp(sub, env->stmt->User.vec[i].id.val.s)) {
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
        if (!strcmp(user->User.vec[i].id.val.s, sub)) {
            return user->User.vec[i].type;
        }
    }
    return NULL;
}

Sub* env_find_sub (Env* env, const char* sub) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            for (int i=0; i<env->stmt->User.size; i++) {
                if (!strcmp(sub, env->stmt->User.vec[i].id.val.s)) {
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
        case EXPR_UNIT: {
            static Type tp = { TYPE_UNIT, 0 };
            return &tp;
        }

        case EXPR_NATIVE: {
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type) { TYPE_NATIVE, 0, .Native=e->Native };
            return tp;
        }

        case EXPR_VAR: {
            Stmt* s = env_id_to_stmt(env, e->Var.val.s, NULL);
            assert(s != NULL);
            return (s->sub == STMT_VAR) ? s->Var.type : s->Func.type;
        }

        case EXPR_NULL: {
            Stmt* user = env_id_to_stmt(env, e->Null.val.s, NULL);
            assert(user != NULL);
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type){ TYPE_USER, 0, .User=user->User.id };
            return tp;
        }

        case EXPR_INT: {
            static Type tp = { TYPE_USER, 0, .User={TX_USER,{.s="Int"},0,0} };
            return &tp;
        }

        case EXPR_ALIAS: {
            Type* tp = env_expr_to_type(env, e->Alias);
            assert(tp != NULL);
            Type* ret = malloc(sizeof(Expr));
            assert(ret != NULL);
            *ret = *tp;
            ret->isalias = 1;
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
                if (!strcmp(e->Call.func->Var.val.s,"clone")) {     // returns type of arg
                    Type* ret = malloc(sizeof(Type));
                    assert(ret != NULL);
                    *ret = *env_expr_to_type(env, e->Call.arg);
                    ret->isalias = 0;
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
                        sprintf(tp->Native.val.s, "%s()", e->Call.func->Native.val.s);
                        break;
                    case EXPR_VAR:
                    case EXPR_NATIVE:
                        sprintf(tp->Native.val.s, "%s(%s)", e->Call.func->Native.val.s, e->Call.arg->Var.val.s);
                        break;
                    case EXPR_NULL:
                        sprintf(tp->Native.val.s, "%s(NULL)", e->Call.func->Native.val.s);
                        break;
                    case EXPR_INT:
                        sprintf(tp->Native.val.s, "%s(0)", e->Call.func->Native.val.s);
                        break;
                    case EXPR_CALL:
                        // _f _g 1
                        break;
                    default:
                        assert(0);
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
            return tp->Tuple.vec[e->Index.index.val.n-1];
        }

        case EXPR_CONS: {
            Stmt* user = env_sub_id_to_user_stmt(env, e->Cons.subtype.val.s);
            assert(user != NULL);
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type){ TYPE_USER, 0, .User=user->User.id };
            return tp;
        }

        case EXPR_PRED:
            return &Type_Bool;

        case EXPR_DISC: {   // x.True
            Type* val = env_expr_to_type(env, e->Disc.val);
            assert(val->sub == TYPE_USER);
            Type* tp = env_sub_id_to_user_type(env, e->Disc.subtype.val.s);
            assert(!tp->isalias && "bug found : `&´ inside subtype");
            if (!val->isalias) {
                return tp;
            } else if (!env_type_hasalloc(env,tp)) {
                // only keep & if sub hasalloc:
                //  &x -> x.True
                return tp;
            } else if (env_type_hasalloc(env,tp)) {
                //  &x -> &x.Cons
                Type* ret = malloc(sizeof(Type));
                assert(ret != NULL);
                *ret = *tp;
                ret->isalias = 1;
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
        Stmt* s = env_id_to_stmt(env, tp->User.val.s, NULL);
        assert(s != NULL);
        return s;
    } else {
        return NULL;
    }
}

int env_type_hasalloc (Env* env, Type* tp) {
    if (tp->isalias) {
        return 0;
    }
    switch (tp->sub) {
        case TYPE_AUTO:
            assert(0);
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_FUNC:
            return 0;
        case TYPE_TUPLE:
            for (int i=0; i<tp->Tuple.size; i++) {
                if (env_type_hasalloc(env, tp->Tuple.vec[i])) {
                    return 1;
                }
            }
            return 0;
        case TYPE_USER: {
            Stmt* user = env_id_to_stmt(env, tp->User.val.s, NULL);
            assert(user!=NULL && user->sub==STMT_USER);
            if (user->User.isrec) {
                return 1;
            }
            for (int i=0; i<user->User.size; i++) {
                if (env_type_hasalloc(env,user->User.vec[i].type)) {
                    return 1;
                }
            }
            return 0;
        }
    }
    assert(0);
}

int env_type_isrec (Env* env, Type* tp) {
    switch (tp->sub) {
        case TYPE_AUTO:
            assert(0);
        case TYPE_UNIT:
        case TYPE_NATIVE:
        case TYPE_FUNC:
        case TYPE_TUPLE:
            return 0;
        case TYPE_USER: {
            Stmt* user = env_id_to_stmt(env, tp->User.val.s, NULL);
            assert(user!=NULL && user->sub==STMT_USER);
            return user->User.isrec;
        }
    }
    assert(0);
}

int env_type_isptr (Env* env, Type* tp) {
    Stmt* s = env_type_to_user_stmt(env, tp);
    return (tp->isalias || (s!=NULL && s->User.isrec));
}

///////////////////////////////////////////////////////////////////////////////

int set_seqs (Stmt* s) {
    switch (s->sub) {
        case STMT_BLOCK:
            s->seqs[1] = s->Block;
            break;

        case STMT_SEQ:
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
            s->seqs[1] = s->Func.body;
            break;

        default:
            break;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int set_envs (Stmt* s) {
    s->env = ALL.env;
    switch (s->sub) {
        case STMT_SET:
            assert(0);

        case STMT_NONE:
        case STMT_RETURN:
        case STMT_SEQ:
        case STMT_IF:
        case STMT_NATIVE:
        case STMT_CALL:
            break;

        case STMT_VAR: {
            Env* new = malloc(sizeof(Env));     // visit stmt after expr above
            *new = (Env) { s, ALL.env };
            ALL.env = new;
            return VISIT_BREAK;                 // do not visit expr again
        }

        case STMT_USER: {
            Env* new = malloc(sizeof(Env));
            *new = (Env) { s, ALL.env };
            ALL.env = new;
            if (s->User.isrec) {
                s->env = ALL.env;
            }
            break;
        }

        case STMT_BLOCK: {
            Env* save = ALL.env;
            visit_stmt(s->Block, set_envs, NULL, NULL);
            ALL.env = save;
            return VISIT_BREAK;                 // do not re-visit children, I just did them
        }

        case STMT_FUNC: {
            // body of recursive function depends on myself
            {
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, ALL.env };        // put myself
                ALL.env = new;
            }

            if (s->Func.body != NULL) {
                Env* save = ALL.env;
                visit_stmt(s->Func.body, set_envs, NULL, NULL);
                ALL.env = save;
            }

            return VISIT_BREAK;                 // do not visit children, I just did that
        }
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int set_tmps (Stmt* s) {
    if (s->sub==STMT_VAR && s->Var.type->sub==TYPE_AUTO) {
        Type* tp = env_expr_to_type(s->env, s->Var.init);
        assert(tp != NULL);
        s->Var.type = tp;
    }
    return VISIT_CONTINUE;
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
            Stmt* decl = env_id_to_stmt(env, tk->val.s, NULL);
            if (decl == NULL) {
                char err[512];
                sprintf(err, "undeclared %s \"%s\"", var_type, tk->val.s);
                return err_message(tk, err);
            }
            return 1;
        }

        default:
printf(">>> %d\n", tk->enu);
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
            return ftk(env, &e->Null, "type");
        case EXPR_VAR:
            return ftk(env, &e->Var, "variable");

        case EXPR_CALL:
            if (e->Call.func->sub != EXPR_VAR) {
                return VISIT_CONTINUE;
            }
            if (!ftk(env, &e->Call.func->Var, "function")) {
                return 0;
            }
            if (!visit_expr(env,e->Call.arg,check_decls_expr)) {
                return 0;
            }
            return VISIT_BREAK;

        case EXPR_CONS: {
            Stmt* user = env_sub_id_to_user_stmt(env, e->Cons.subtype.val.s);
            if (user == NULL) {
                char err[512];
                sprintf(err, "undeclared subtype \"%s\"", e->Cons.subtype.val.s);
                return err_message(&e->Cons.subtype, err);
            }
            return VISIT_CONTINUE;
        }

        case EXPR_DISC:
        case EXPR_PRED: {
            Tk* subtype = (e->sub == EXPR_DISC ? &e->Disc.subtype : &e->Pred.subtype);
            if (subtype->enu == TX_NULL) {
                if (!ftk(env, subtype, "type")) {
                    return 0;
                }
            } else {
                Stmt* user = env_sub_id_to_user_stmt(env, subtype->val.s);
                if (user == NULL) {
                    char err[512];
                    sprintf(err, "undeclared subtype \"%s\"", subtype->val.s);
                    return err_message(subtype, err);
                }
            }
            return VISIT_CONTINUE;
        }

        default:
            return VISIT_CONTINUE;
    }
}

///////////////////////////////////////////////////////////////////////////////

int type_is_sup_sub (Type* sup, Type* sub) {
#if 1
    assert(sup->sub!=TYPE_AUTO && sub->sub!=TYPE_AUTO);
#else
    if (sup->sub==TYPE_AUTO || sub->sub==TYPE_AUTO) {
        return 1;
    }
#endif
    if (sup->sub==TYPE_NATIVE || sub->sub==TYPE_NATIVE) {
        return 1;
    }
    if (sup->sub != sub->sub) {
        return 0;   // different TYPE_xxx
    }
    if (sup->isalias != sub->isalias) {
        return 0;
    }
    switch (sup->sub) {
        case TYPE_USER:
            //printf("%s vs %s\n", sup->User.val.s, sub->User.val.s);
            return (
                (sup->isalias == sub->isalias) &&
                (sub->User.enu==TK_ERR || !strcmp(sup->User.val.s,sub->User.val.s))
                    // TODO: clone
            );

        case TYPE_TUPLE:
            if (sup->Tuple.size != sub->Tuple.size) {
                return 0;
            }
            for (int i=0; i<sup->Tuple.size; i++) {
                if (!type_is_sup_sub(sup->Tuple.vec[i], sub->Tuple.vec[i])) {
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
        case EXPR_NATIVE:
        case EXPR_VAR:
        case EXPR_TUPLE:
        case EXPR_NULL:
        case EXPR_INT:
        case EXPR_ALIAS:
            break;

        case EXPR_INDEX:
        case EXPR_DISC:
        case EXPR_PRED:
            TODO("TODO [check_types]: EXPR_INDEX/EXPR_DISC/EXPR_PRED\n");
            // TODO: check if e->tuple is really a tuple and that e->index is in range
            TODO("TODO [check_types]: (x,y,z).1\n");
            break;

        case EXPR_CALL: {
            Type* func = env_expr_to_type(env, e->Call.func);
            Type* arg  = env_expr_to_type(env, e->Call.arg);

            if (e->Call.func->sub == EXPR_NATIVE) {
                TODO("TODO [check_types]: _f(...)\n");
            } else {
                assert(e->Call.func->sub == EXPR_VAR);
                if (!strcmp(e->Call.func->Var.val.s,"clone")) {
                    TODO("TODO [check_types]: clone(...)\n");
                } else if (!strcmp(e->Call.func->Var.val.s,"show")) {
                    TODO("TODO [check_types]: show(...)\n");
                } else if (!type_is_sup_sub(func->Func.inp, arg)) {
                    char err[512];
                    sprintf(err, "invalid call to \"%s\" : type mismatch", e->Call.func->Var.val.s);
                    return err_message(&e->Call.func->Var, err);
                }
            }
            break;
        }

        case EXPR_CONS: {
            Sub* sub = env_find_sub(env, e->Cons.subtype.val.s);
            assert(sub != NULL);
            if (!type_is_sup_sub(sub->type, env_expr_to_type(env,e->Cons.arg))) {
                char err[512];
                sprintf(err, "invalid constructor \"%s\" : type mismatch", e->Cons.subtype.val.s);
                return err_message(&e->Cons.subtype, err);
            }
        }
    }
    return 1;
}

int check_types_stmt (Stmt* s) {
    switch (s->sub) {
        case STMT_SET:
            assert(0);

        case STMT_NONE:
        case STMT_USER:
        case STMT_FUNC:
        case STMT_SEQ:
        case STMT_BLOCK:
        case STMT_NATIVE:
        case STMT_CALL:
            return 1;

        case STMT_VAR:
            if (!type_is_sup_sub(s->Var.type, env_expr_to_type(s->env,s->Var.init))) {
                char err[512];
                sprintf(err, "invalid assignment to \"%s\" : type mismatch", s->Var.id.val.s);
                return err_message(&s->Var.id, err);
            }
            return 1;

        case STMT_IF:
            if (!type_is_sup_sub(&Type_Bool, env_expr_to_type(s->env,s->If.tst))) {
                return err_message(&s->tk, "invalid condition : type mismatch");
            }
            return 1;

        case STMT_RETURN: {
            Stmt* func = env_stmt_to_func(s);
            assert(func != NULL);
            assert(func->Func.type->sub == TYPE_FUNC);
            if (!type_is_sup_sub(func->Func.type->Func.out, env_expr_to_type(s->env,s->Return))) {
                return err_message(&s->tk, "invalid return : type mismatch");
            }
            return 1;
        }
    }
    assert(0);
}

///////////////////////////////////////////////////////////////////////////////

// How to detect ownership violations?
//  - Rule 6: transfer ownership and then access again:
//      var x: Nat = ...            -- owner
//      var z: Nat = add(x,...)     -- transfer
//      ... x or &x ...             -- error: access after ownership transfer
//  - Rule 5: save reference and then pass ownership:
//      var x: Nat = ...            -- owner
//      var z: Nat = &x             -- borrow
//      ... x ...                   -- error: transfer while borrow is active
//  - Rule 3: return alias pointing to local owner
//      func f: () -> &List {
//          var l: List = build()   -- `l` is the owner
//          return &l               -- error: cannot return alias to deallocated value
//      }
//
//  - Start from each STMT_VAR declaration: find all with `visit`.
//      - only STMT_VAR.isrec
//  - Call `exec` and stop when `env_id_to_stmt` fails.
//      - track EXPR_VAR acesses:
//          - transfer (istx)  // error if state=borrowed/moved // set state=moved
//          - borrow   (!istx) // error if state=moved          // set state=borrowed

int check_owner_alias (Stmt* S) {

    // visit all var declarations

    if (S->sub != STMT_VAR) {
        return 1;               // not var declaration
    }

    if (!env_type_hasalloc(S->env, S->Var.type)) {
        return 1;               // not recursive type
    }

    // STMT_VAR: recursive user type

    // var n: Nat = ...         // starting from each declaration

    typedef enum { NONE, TRANSFERRED, BORROWED } State;
    State state = NONE;
    Tk* tk1 = NULL;

    typedef struct { Stmt* stop; int state; int aliases_n; } Stack;
    Stack stack[256];
    int stack_n = 0;

    Stmt* aliases[256] = { S }; // list of declarations that reach me, start with myself
    int aliases_n = 1;          // if someneone points to theses, it also points to myself

    auto int fs (Stmt* s);

    void pre (void) {
        state = NONE;
        tk1 = NULL;
        stack_n = 0;
        aliases[0] = S;
        aliases_n = 1;
    }

    return exec(S->seqs[1], pre, fs);

    // ... n ...                // check all accesses to it

    int fs (Stmt* s)
    {
        // stop when tracked STMT_VAR is out of scope

        Stmt* decl = env_id_to_stmt(s->env, S->Var.id.val.s, NULL);
        if (decl!=NULL && decl==S) {
            // ok: S is still in scope
        } else {
            return EXEC_HALT;           // no: S is not in scope
        }

        // push/pop state/aliases stack

        if (s->sub == STMT_BLOCK) {        // enter block: push state
            stack[stack_n].stop      = s->seqs[0];
            stack[stack_n].state     = state;
            stack[stack_n].aliases_n = aliases_n;
            assert(stack_n < 256);
            stack_n++;
        }

        if (s == stack[stack_n-1].stop) {      // leave block: pop state
            stack_n--;
            aliases_n = stack[stack_n].aliases_n;
            if (state != TRANSFERRED) {
                state = stack[stack_n].state;
            }
        }

        //

        auto int rule_5_6 (void);
        auto int rule_3 (void);

        int ret = rule_5_6();
        if (ret != EXEC_CONTINUE) {
            return ret;
        }
        return rule_3();

        int rule_5_6 (void)
        {
            int ftk (Env* env, Tk* tk, int isalias, int istx) {
                if (strcmp(S->Var.id.val.s,tk->val.s)) {
                    return 1;
                }

                // ensure that EXPR_VAR is really same as STMT_VAR
                Stmt* decl = env_id_to_stmt(env, tk->val.s, NULL);
                assert(decl!=NULL && decl==S);

                //isalias = isalias || env_tk_to_type(env,tk)->isalias;

                // if already moved, it doesn't matter, any access is invalid

                switch (state) {
                    case NONE:
                        break;
                    case TRANSFERRED: {  // Rule 6
                        assert(tk1 != NULL);
                        char err[1024];
                        sprintf(err, "invalid access to \"%s\" : ownership was transferred (ln %ld)",
                                tk->val.s, tk1->lin);
                        err_message(tk, err);
                        return 0;
                    }
                    case BORROWED: {    // Rule 5
                        assert(tk1 != NULL);
                        if (!isalias) {
                            char err[1024];
                            sprintf(err, "invalid transfer of \"%s\" : active alias in scope (ln %ld)",
                                    tk->val.s, tk1->lin);
                            err_message(tk, err);
                            return 0;
                        }
                    }
                }
                tk1 = tk;
                if (isalias) {
                    assert(state != TRANSFERRED && "bug found");
                    state = BORROWED;
                } else if (istx) {
                    state = TRANSFERRED;
                }
                return 1;
            }

return EXEC_CONTINUE;
            switch (s->sub) {
                case STMT_VAR: {
                    Expr* e = s->Var.init;
                    switch (e->sub) {
                        case EXPR_UNIT:
                        case EXPR_NATIVE:
                        case EXPR_NULL:
                        case EXPR_INT:
                        case EXPR_PRED:
                            return EXEC_CONTINUE;

                        case EXPR_VAR:
                            assert(0);
#if XXXXXX
                            return ftk(s->env, &e->tk, e->isalias, 1);

                        case EXPR_TUPLE:
                            for (int i=0; i<e->Tuple.size; i++) {
                                int ret = ftk(s->env, &e->Tuple.vec[i], 0, 1);
                                if (ret != EXEC_CONTINUE) {
                                    return ret;
                                }
                            }
                            return EXEC_CONTINUE;

                        case EXPR_CONS:
                            return ftk(s->env, &e->Cons.arg, 0, 1);

                        case EXPR_CALL:
                            return ftk(s->env, &e->Call.arg, 0, 1);

                        case EXPR_INDEX:
                            return ftk(s->env, &e->Index.val, e->isalias, 0);

                        case EXPR_DISC:
                            return ftk(s->env, &e->Disc.val, e->isalias, 0);
#endif
                    }
                    assert(0 && "bug found");
                }

#if XXXXXX
                case STMT_RETURN:
                    return ftk(s->env, &s->Return, 0, 1);
#endif

                default:
                    return EXEC_CONTINUE;
            }
        }

        int rule_3 (void)
        {
return EXEC_CONTINUE;
#if XXXXXX
            switch (s->sub) {
                case STMT_VAR: {
                    // var S: T = ...
                    // var s: &T = &y;      <-- if y reaches S, so does s
                    if (s->Var.type.isalias) {
                        // get "var" being aliased
                        Tk* var = NULL;
                        Type* tp = env_expr_to_type(s->env, &s->Var.init);
                        assert(tp->isalias);
                        switch (s->Var.init.sub) {
                            case EXPR_VAR:
                                var = &s->Var.init.tk;
                                break;
                            case EXPR_INDEX:
                                var = &s->Var.init.Index.val;
                                break;
                            case EXPR_DISC:
                                var = &s->Var.init.Disc.val;
                                break;
                            case EXPR_CALL:
                                break;          // TODO?
                            case EXPR_TUPLE:
                                break;          // TODO?
                            default:
                                assert(0);
                        }
                        if (var != NULL) {
                            assert(var!=NULL && var->enu==TX_VAR);
                            Stmt* decl = env_id_to_stmt(s->env, var->val.s, NULL);
                            assert(decl != NULL);

                            // check if y reaches S
                            // if reaches, include s in the list
                            for (int i=0; i<aliases_n; i++) {
                                if (aliases[i] == decl) {
                                    assert(aliases_n < 256);
                                    aliases[aliases_n++] = s;
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
                case STMT_RETURN: {
                    Type* tp = env_tk_to_type(s->env, &s->Return);
                    if (tp->isalias) {
                        assert(s->Return.enu == TX_VAR);
                        int scope;
                        Stmt* decl = env_id_to_stmt(s->env, s->Return.val.s, &scope);
                        assert(decl != NULL);
                        if (scope == 0) {
                            for (int i=0; i<aliases_n; i++) {
                                if (aliases[i] == decl) {
                                    char err[1024];
                                    sprintf(err, "invalid return : cannot return alias to local \"%s\" (ln %ld)",
                                            S->Var.id.val.s, S->Var.id.lin);
                                    err_message(&s->Return, err);
                                    return EXEC_ERROR;
                                }
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            return EXEC_CONTINUE;
#endif
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

int env (Stmt* s) {
    assert(visit_stmt(s,set_seqs,NULL,NULL));
    assert(visit_stmt(s,set_envs,NULL,NULL));
//dump_stmt(s);
    if (!visit_stmt(s,NULL,check_decls_expr,check_decls_type)) {
        return 0;
    }
    assert(visit_stmt(s,set_tmps,NULL,NULL));
    if (!visit_stmt(s,check_types_stmt,check_types_expr,NULL)) {
        return 0;
    }
    if (!visit_stmt(s,check_owner_alias,NULL,NULL)) {
        return 0;
    }
    return 1;
}
