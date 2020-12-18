#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

Type Type_Unit  = { TYPE_UNIT, 0 };
Type Type_Bool  = { TYPE_USER, 0, .User={TX_USER,{.s="Bool"},0,0} };

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
//puts(cur);
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

Type* env_tk_to_type (Env* env, Tk* tk) { // static returns use env=NULL b/c no ids underneath
    switch (tk->enu) {
        case TK_UNIT: {
            static Type tp = { TYPE_UNIT, 0 };
            return &tp;
        }
        case TX_NATIVE: {
            static Type tp = { TYPE_NATIVE, 0 };
            return &tp;
        }
        case TX_VAR: {
            Stmt* s = env_id_to_stmt(env, tk->val.s, NULL);
            assert(s != NULL);
            return (s->sub == STMT_VAR) ? &s->Var.type : &s->Func.type;
        }
        case TX_NULL: {
            Stmt* user = env_id_to_stmt(env, tk->val.s, NULL);
            assert(user != NULL);
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type){ TYPE_USER, 0, .User=user->User.id };
            return tp;
        }
        default:
printf(">>> %d\n", tk->enu);
            assert(0);
    }
}

Type* env_expr_to_type (Env* env, Expr* e) {
    switch (e->sub) {
        case EXPR_UNIT:
        case EXPR_NATIVE:
        case EXPR_VAR:
        case EXPR_NULL:
            return env_tk_to_type(env, &e->tk);

        case EXPR_ALIAS: {
            Type* tp = env_tk_to_type(env, &e->Alias);
            assert(tp->sub==TYPE_USER && !tp->isalias);

            Type* ret = malloc(sizeof(Type));
            assert(ret != NULL);
            *ret = *tp;
            ret->isalias = 1;
            return ret;
        }

        case EXPR_TUPLE: {
            Type* vec = malloc(e->Tuple.size*sizeof(Type));
            assert(vec != NULL);
            for (int i=0; i<e->Tuple.size; i++) {
                vec[i] = *env_tk_to_type(env, &e->Tuple.vec[i]);
            }
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type) { TYPE_TUPLE, 0, {.Tuple={e->Tuple.size,vec}} };
            return tp;
        }

        case EXPR_CONS: {
            Stmt* user = env_sub_id_to_user_stmt(env, e->Cons.subtype.val.s);
            assert(user != NULL);
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type){ TYPE_USER, 0, .User=user->User.id };
            return tp;
        }

        case EXPR_CALL: {
            Type* tp = NULL;
            if (e->Call.func.enu == TX_VAR) {
                tp = env_tk_to_type(env, &e->Call.func);
            } else {
                assert(e->Call.func.enu == TX_NATIVE);
                static Type nat = { TYPE_NATIVE, 0 };
                static Type tp_ = { TYPE_FUNC, 0, .Func={&nat,&nat} };
                tp = &tp_;
            }
            assert(tp->sub == TYPE_FUNC);
            return tp->Func.out;
        }

        case EXPR_INDEX:    // x.1
            return &env_tk_to_type(env, &e->Index.val)->Tuple.vec[e->Index.index.val.n-1];

        case EXPR_DISC: {   // x.True
            Type* val = env_tk_to_type(env, &e->Disc.val);
            assert(val->sub == TYPE_USER);
            Stmt* decl = env_id_to_stmt(env, val->User.val.s, NULL);
            assert(decl!=NULL && decl->sub==STMT_USER);
            for (int i=0; i<decl->User.size; i++) {
                if (!strcmp(decl->User.vec[i].id.val.s, e->Disc.subtype.val.s)) {
                    Type* tp = &decl->User.vec[i].type;
                    assert(!tp->isalias && "bug found : `&´ inside subtype");
                    if (!val->isalias) {
                        return tp;
                    } else {
                        Type* ret = malloc(sizeof(Type));
                        assert(ret != NULL);
                        *ret = *tp;
                        ret->isalias = 1;
                        return ret;
                    }
                }
            }
            assert(0 && "bug found");
        }

        case EXPR_PRED:
            return &Type_Bool;
    }
    assert(0 && "bug found");
}

//  type Bool { ... }
//  var x: Bool
//  ... x ...           -- give "x" -> "var x" -> type Bool
Stmt* env_tk_to_type_to_user_stmt (Env* env, Tk* tk) {
    Type* tp = env_tk_to_type(env, tk);
    assert(tp != NULL);
    if (tp->sub == TYPE_USER) {
        Stmt* s = env_id_to_stmt(env, tp->User.val.s, NULL);
        assert(s != NULL);
        return s;
    } else {
        return NULL;
    }
}

#if 0
void env_dump (Env* env) {
    while (env != NULL) {
        if (env->stmt->sub == STMT_USER) {
            puts(env->stmt->User.id.val.s);
        } else {
            assert(env->stmt->sub == STMT_VAR);
            puts(env->stmt->Var.id.val.s);
        }
        env = env->prev;
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////

void set_envs (Stmt* S) {
    // TODO: _N_
    // predeclare functions `clone`, `output`
    static Type user  = { TYPE_USER, 0, .User={TK_ERR,{},0,0} };
    static Type alias = { TYPE_USER, 1, .User={TK_ERR,{},0,0} };
    static Env clone, output;
    {
        static Stmt s_out = {
            0, STMT_VAR, NULL, {NULL,NULL},
            .Var={ {TX_VAR,{.s="clone"},0,0},
                   {TYPE_FUNC,.Func={&alias,&user}},{EXPR_UNIT} }
        };
        clone = (Env) { &s_out, NULL };
    }
    {
        static Type alias = { TYPE_USER, 0, .User={TK_ERR,{},0,0} };
        static Stmt s_out = {
            0, STMT_VAR, NULL, {NULL,NULL},
            .Var={ {TX_VAR,{.s="output"},0,0},
                   {TYPE_FUNC,.Func={&alias,&Type_Unit}},{EXPR_UNIT} }
        };
        output = (Env) { &s_out, &clone };
    }

    Env* env = &output;

    int fs (Stmt* s) {
        s->env = env;
        switch (s->sub) {
            case STMT_RETURN:
            case STMT_CALL:
            case STMT_SEQ:
            case STMT_IF:
                break;

            case STMT_VAR: {
                Env* new = malloc(sizeof(Env));     // visit stmt after expr above
                *new = (Env) { s, env };
                env = new;
                return VISIT_BREAK;                 // do not visit expr again
            }

            case STMT_USER: {
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, env };
                env = new;
                if (s->User.isrec) {
                    s->env = env;
                }
                break;
            }

            case STMT_BLOCK: {
                Env* save = env;
                visit_stmt(s->Block, fs);
                env = save;
                return VISIT_BREAK;                 // do not re-visit children, I just did them
            }

            case STMT_FUNC: {
                // body of recursive function depends on myself
                {
                    Env* new = malloc(sizeof(Env));
                    *new = (Env) { s, env };        // put myself
                    env = new;
                }

                Env* save = env;

                // body depends on arg
                {
                    Stmt* arg = malloc(sizeof(Stmt));
                    *arg = (Stmt) {
                        0, STMT_VAR, env, {s->Func.body,s->Func.body},
                        .Var={ {TX_VAR,{.s="arg"},0,0},*s->Func.type.Func.inp,{0,EXPR_UNIT,{}} }
                    };
                    Env* new = malloc(sizeof(Env));
                    *new = (Env) { arg, env };
                    env = new;
                }

                visit_stmt(s->Func.body, fs);

                env = save;

                return VISIT_BREAK;                 // do not visit children, I just did that
            }
        }
        return 1;
    }

    visit_stmt(S, fs);
}

///////////////////////////////////////////////////////////////////////////////

int check_undeclareds (Stmt* s) {

    int ftk (Env* env, Tk* tk, char* var_type) {
        switch (tk->enu) {
            case TK_UNIT:
            case TX_NATIVE:
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
//printf(">>> %d\n", tk->enu);
                assert(0 && "TODO");
        }
    }

    int fe (Env* env, Expr* e) {
        switch (e->sub) {
            case EXPR_UNIT:
            case EXPR_NATIVE:
                return 1;

            case EXPR_NULL:
                return ftk(env, &e->tk, "type");

            case EXPR_VAR:
                return ftk(env, &e->tk, "variable");

            case EXPR_ALIAS:
                return ftk(env, &e->Alias, "variable");

            case EXPR_TUPLE:
                for (int i=0; i<e->Tuple.size; i++) {
                    if (!ftk(env, &e->Tuple.vec[i], "variable")) {
                        return 0;
                    }
                }
                return 1;

            case EXPR_CALL:
                return ftk(env, &e->Call.func, "function") &&
                       ftk(env, &e->Call.arg,  "variable");

            case EXPR_INDEX:
                return ftk(env, &e->Index.val, "variable");

            case EXPR_CONS: {
                Stmt* user = env_sub_id_to_user_stmt(env, e->Cons.subtype.val.s);
                if (user == NULL) {
                    char err[512];
                    sprintf(err, "undeclared subtype \"%s\"", e->Cons.subtype.val.s);
                    return err_message(&e->Cons.subtype, err);
                }
                return ftk(env, &e->Cons.arg, "variable");
            }

            case EXPR_DISC:
            case EXPR_PRED: {
                Tk* val     = (e->sub == EXPR_DISC ? &e->Disc.val     : &e->Pred.val);
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
                return ftk(env, val, "variable");
            }
        }
        assert(0);
    }

    int ftp (Type* tp, void* env) {
        if (tp->sub == TYPE_USER) {
            return ftk((Env*) env, &tp->User, "type");
        }
        return 1;
    }

    switch (s->sub) {
        case STMT_SEQ:
        case STMT_BLOCK:
            return 1;

        case STMT_VAR:
            return (visit_type(&s->Var.type,ftp,s->env) && fe(s->env, &s->Var.init));

        case STMT_USER:
            for (int i=0; i<s->User.size; i++) {
                if (!visit_type(&s->User.vec[i].type,ftp,s->env)) {
                    return 0;
                }
            }
            return 1;

        case STMT_FUNC:
            return visit_type(&s->Func.type, ftp, s->env);

        case STMT_CALL:
            return fe(s->env, &s->Call);

        case STMT_IF:
            return ftk(s->env, &s->If.cond, "variable");

        case STMT_RETURN:
            return ftk(s->env, &s->Return, "variable");
    }
    assert(0);
}

///////////////////////////////////////////////////////////////////////////////

int check_types (Stmt* s) {
    int type_is_sup_sub (Type* sup, Type* sub) {
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
                    if (!type_is_sup_sub(&sup->Tuple.vec[i], &sub->Tuple.vec[i])) {
                        return 0;
                    }
                }
                break;

            default:
                break;
        }
        return 1;
    }

    int fe (Env* env, Expr* e) {
        switch (e->sub) {
            case EXPR_UNIT:
            case EXPR_NATIVE:
            case EXPR_VAR:
            case EXPR_TUPLE:
            case EXPR_NULL:
                break;

            case EXPR_ALIAS:
            case EXPR_DISC:
            case EXPR_PRED:
                TODO("TODO [check_types]: EXPR_INDEX/EXPR_ALIAS/EXPR_DISC/EXPR_PRED\n");
                break;

            case EXPR_INDEX:
                // TODO: check if e->tuple is really a tuple and that e->index is in range
                TODO("TODO [check_types]: (x,y,z).1\n");
                break;

            case EXPR_CALL: {
                Type* func = env_tk_to_type(env, &e->Call.func);
                Type* arg  = env_tk_to_type(env, &e->Call.arg);

                if (e->Call.func.enu == TX_NATIVE) {
                    TODO("TODO [check_types]: _f(...)\n");
                } else {
                    assert(e->Call.func.enu == TX_VAR);
                    if (!strcmp(e->Call.func.val.s,"clone")) {
                        TODO("TODO [check_types]: clone(...)\n");
                    } else if (!strcmp(e->Call.func.val.s,"output")) {
                        TODO("TODO [check_types]: output(...)\n");
                    } else if (!type_is_sup_sub(func->Func.inp, arg)) {
                        char err[512];
                        sprintf(err, "invalid call to \"%s\" : type mismatch", e->Call.func.val.s);
                        return err_message(&e->Call.func, err);
                    }
                }
                break;
            }

            case EXPR_CONS: {
                Sub* sub = env_find_sub(env, e->Cons.subtype.val.s);
                assert(sub != NULL);
                if (!type_is_sup_sub(&sub->type, env_tk_to_type(env, &e->Cons.arg))) {
                    char err[512];
                    sprintf(err, "invalid constructor \"%s\" : type mismatch", e->Cons.subtype.val.s);
                    return err_message(&e->Cons.subtype, err);
                }
            }
        }
        return 1;
    }

    switch (s->sub) {
        case STMT_USER:
        case STMT_FUNC:
        case STMT_SEQ:
        case STMT_BLOCK:
            return 1;

        case STMT_VAR:
            if (!fe(s->env, &s->Var.init)) {
                return 0;
            }
            if (!type_is_sup_sub(&s->Var.type, env_expr_to_type(s->env, &s->Var.init))) {
                char err[512];
                sprintf(err, "invalid assignment to \"%s\" : type mismatch", s->Var.id.val.s);
                return err_message(&s->Var.id, err);
            }
            return 1;

        case STMT_CALL: {
            Type* tp = env_expr_to_type(s->env, &s->Call);
            if (tp->sub != TYPE_UNIT) {
                assert(s->Call.sub == EXPR_CALL);
                Tk* func = &s->Call.Call.func;
                assert(func->enu==TX_VAR || func->enu==TX_NATIVE);
                if (func->enu == TX_VAR) {
                    char err[512];
                    sprintf(err, "invalid call to \"%s\" : missing return assignment",
                            func->val.s);
                    return err_message(&s->tk, err);
                }
            }
            return fe(s->env, &s->Call);
        }

        case STMT_IF:
            if (!type_is_sup_sub(&Type_Bool, env_tk_to_type(s->env, &s->If.cond))) {
                return err_message(&s->tk, "invalid condition : type mismatch");
            }
            return 1;

        case STMT_RETURN: {
            Stmt* func = env_stmt_to_func(s);
            assert(func != NULL);
            assert(func->Func.type.sub == TYPE_FUNC);
            if (!type_is_sup_sub(func->Func.type.Func.out, env_tk_to_type(s->env, &s->Return))) {
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
    if (S->Var.type.sub != TYPE_USER) {
        return 1;               // not user type
    }

    Stmt* user = env_id_to_stmt(S->env, S->Var.type.User.val.s, NULL);
    assert(user!=NULL && user->sub==STMT_USER);
    if (!user->User.isrec) {
        return 1;               // not recursive type
    }

    // STMT_VAR: recursive user type

    // var n: Nat = ...         // starting from each declaration

    typedef enum { NONE, TRANSFERRED, BORROWED } State;
    State state = NONE;
    Tk* tk1 = NULL;

    typedef struct { Stmt* s; int state; } Stack;
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

    int fs (Stmt* s) {
        Stmt* decl = env_id_to_stmt(s->env, S->Var.id.val.s, NULL);
        if (decl!=NULL && decl==S) {
            // ok: S is still in scope
        } else {
            return EXEC_HALT;           // no: S is not in scope
        }

        auto int rule_5_6 (void);
        auto int rule_3 (void);

        int ret = rule_5_6();
        if (ret != EXEC_CONTINUE) {
            return ret;
        }
        return rule_3();

        int rule_5_6 (void)
        {
            int ftk (Env* env, Tk* tk, int isalias) {
                if (strcmp(S->Var.id.val.s,tk->val.s)) {
                    return 1;
                }

                // ensure that EXPR_VAR is really same as STMT_VAR
                Stmt* decl = env_id_to_stmt(env, tk->val.s, NULL);
                assert(decl!=NULL && decl==S);

                Type* tp = env_tk_to_type(env, tk);
                isalias = isalias || tp->isalias;

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
                } else {
                    state = TRANSFERRED;
                }
                return 1;
            }

            switch (s->sub) {
                case STMT_VAR: {
                    Expr* e = &s->Var.init;
                    switch (e->sub) {
                        case EXPR_UNIT:
                        case EXPR_NATIVE:
                        case EXPR_NULL:
                        case EXPR_PRED:
                            return EXEC_CONTINUE;

                        case EXPR_VAR:
                            return ftk(s->env, &e->Alias,
                                       env_expr_to_type(s->env, e)->isalias);

                        case EXPR_ALIAS:
                            return ftk(s->env, &e->Alias, 1);

                        case EXPR_TUPLE:
                            for (int i=0; i<e->Tuple.size; i++) {
                                int ret = ftk(s->env, &e->Tuple.vec[i], 1);
                                if (ret != EXEC_CONTINUE) {
                                    return ret;
                                }
                            }
                            return EXEC_CONTINUE;

                        case EXPR_CONS:
                            return ftk(s->env, &e->Cons.arg, 0);

                        case EXPR_CALL:
                            return ftk(s->env, &e->Call.arg, 0);

                        case EXPR_INDEX:
                        case EXPR_DISC:
                            TODO("check if x.field is rec");
                            return EXEC_CONTINUE;
                    }
                    assert(0 && "bug found");
                }

                case STMT_CALL:
                    return ftk(s->env, &s->Call.Call.arg, 0);

                case STMT_RETURN:
                    return ftk(s->env, &s->Return, 0);

                default:
                    return EXEC_CONTINUE;
            }
        }

        int rule_3 (void)
        {
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
                            case EXPR_ALIAS:
                                var = &s->Var.init.Alias;
                                break;
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
                                break;
                            case EXPR_TUPLE: // (&x,&y)  // must put both
                                break;
                            default:
                                break;  // assert below
                        }
                        assert(var!=NULL && var->enu==TX_VAR);

                        decl = env_id_to_stmt(s->env, var->val.s, NULL);
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

            if (s->sub == STMT_BLOCK) {        // enter block: push state
                stack[stack_n].s = s->seqs[0];
                stack[stack_n].state = state;
                assert(stack_n < 256);
                stack_n++;
            }

            if (s == stack[stack_n-1].s) {
                stack_n--;
                if (state != TRANSFERRED) {
                    state = stack[stack_n].state;   // leave block: pop state
                }
            }

            return EXEC_CONTINUE;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

int env (Stmt* s) {
    set_envs(s);
    if (!visit_stmt(s,check_undeclareds)) {
        return 0;
    }
    if (!visit_stmt(s,check_types)) {
        return 0;
    }
    if (!visit_stmt(s,check_owner_alias)) {
        return 0;
    }
    return 1;
}
