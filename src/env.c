#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "all.h"

Type Type_Unit  = { TYPE_UNIT, NULL, 0 };
Type Type_Bool  = { TYPE_USER, NULL, 0, .User={TX_UPPER,{.s="Bool"},0,0} };

int err_message (Tk tk, const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): %s", tk.lin, tk.col, v);
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

Type* env_expr_to_type (Expr* e) { // static returns use env=NULL b/c no ids underneath
    switch (e->sub) {
        case EXPR_UNIT: {
            static Type tp = { TYPE_UNIT, NULL, 0 };
            return &tp;
        }

        case EXPR_NATIVE: {
            static Type tp = { TYPE_NATIVE, NULL, 0 };
            return &tp;
        }

        case EXPR_VAR: {
            Stmt* s = env_id_to_stmt(e->env, e->Var.id.val.s, NULL);
            assert(s != NULL);
            return (s->sub == STMT_VAR) ? &s->Var.type : &s->Func.type;
        }

        case EXPR_CALL: {
            Type* tp = NULL;
            if (e->Call.func->sub == EXPR_VAR) {
                tp = env_expr_to_type(e->Call.func);
            } else {
                static Type nat = { TYPE_NATIVE, NULL, 0 };
                static Type tp_ = { TYPE_FUNC, NULL, 0, .Func={&nat,&nat} };
                tp = &tp_;
            }
            assert(tp->sub == TYPE_FUNC);
            return tp->Func.out;
        }

        case EXPR_ALIAS: {
            Type* tp = env_expr_to_type(e->Alias);
            assert(tp->sub==TYPE_USER && !tp->isalias);

            Type* ret = malloc(sizeof(Type));
            assert(ret != NULL);
            *ret = *tp;
            ret->isalias = 1;
            return ret;
        }

        case EXPR_CONS: {
            Tk tk;
            if (e->Cons.subtype.enu == TX_NIL) {
                Stmt* user = env_id_to_stmt(e->env, e->Cons.subtype.val.s, NULL);
                assert(user != NULL);
                tk = user->User.id;
            } else {
                Stmt* user = env_sub_id_to_user_stmt(e->env, e->Cons.subtype.val.s);
                assert(user != NULL);
                tk = user->User.id;
            }
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type){ TYPE_USER, e->env, 0, .User=tk };
            return tp;
        }

        case EXPR_TUPLE: {
            Type* vec = malloc(e->Tuple.size*sizeof(Type));
            assert(vec != NULL);
            for (int i=0; i<e->Tuple.size; i++) {
                vec[i] = *env_expr_to_type(&e->Tuple.vec[i]);
            }
            Type* tp = malloc(sizeof(Type));
            assert(tp != NULL);
            *tp = (Type) { TYPE_TUPLE, e->env, 0, {.Tuple={e->Tuple.size,vec}} };
            return tp;
        }

        case EXPR_INDEX:    // x.1
            return &env_expr_to_type(e->Index.tuple)->Tuple.vec[e->Index.index-1];

        case EXPR_DISC: {   // x.True
            Type* val = env_expr_to_type(e->Disc.val);
            assert(val->sub == TYPE_USER);
            Stmt* decl = env_id_to_stmt(e->env, val->User.val.s, NULL);
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
Stmt* env_expr_to_type_to_user_stmt (Expr* e) {
    Type* tp = env_expr_to_type(e);
    assert(tp != NULL);
    if (tp->sub == TYPE_USER) {
        Stmt* s = env_id_to_stmt(e->env, tp->User.val.s, NULL);
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
    // TODO: _N_=0
    // predeclare function `output`
    static Env env_;
    {
        static Type alias = { TYPE_USER, NULL, 0, .User={TK_ERR,{},0,0} };
        static Stmt s_out = {
            0, STMT_VAR, NULL, {NULL,NULL},
            .Var={ {TX_LOWER,{.s="output"},0,0},
                   {TYPE_FUNC,NULL,.Func={&alias,&Type_Unit}},{EXPR_UNIT} }
        };
        env_ = (Env) { &s_out, NULL };
    }

    Env* env = &env_;

    int ft (Type* tp) {
        tp->env = env;
        return 1;
    }

    int fe (Expr* e) {
        e->env = env;
        return 1;
    }

    int fs (Stmt* s) {
        s->env = env;
        switch (s->sub) {
            case STMT_RETURN:
            case STMT_CALL:
            case STMT_SEQ:
            case STMT_IF:
                break;

            case STMT_VAR: {
                visit_type(&s->Var.type, ft);
                visit_expr(&s->Var.init, fe);       // visit expr before stmt below
                Env* new = malloc(sizeof(Env));     // visit stmt after expr above
                *new = (Env) { s, env };
                env = new;
                return VISIT_BREAK;                 // do not visit expr again
            }

            case STMT_USER: {
                Env* new = malloc(sizeof(Env));
                *new = (Env) { s, env };
                env = new;
                break;
            }

            case STMT_BLOCK: {
                Env* save = env;
                visit_stmt(s->Block, fs, fe, ft);
                env = save;
                return VISIT_BREAK;                 // do not re-visit children, I just did them
            }

            case STMT_FUNC: {
                visit_type(&s->Func.type, ft);

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
                        .Var={ {TX_LOWER,{.s="arg"},0,0},*s->Func.type.Func.inp,{0,EXPR_UNIT,env,{}} }
                    };
                    Env* new = malloc(sizeof(Env));
                    *new = (Env) { arg, env };
                    env = new;
                }

                visit_stmt(s->Func.body, fs, fe, ft);

                env = save;

                return VISIT_BREAK;                 // do not visit children, I just did that
            }
        }
        return 1;
    }

    visit_stmt(S, fs, fe, ft);
}

///////////////////////////////////////////////////////////////////////////////

int check_undeclareds (Stmt* s) {
    int ft (Type* tp) {
        if (tp->sub == TYPE_USER) {
            if (env_id_to_stmt(tp->env, tp->User.val.s, NULL) == NULL) {
                char err[512];
                sprintf(err, "undeclared type \"%s\"", tp->User.val.s);
                return err_message(tp->User, err);
            } else {
            }
        }
        return 1;
    }

    int fe (Expr* e) {
        switch (e->sub) {
            case EXPR_VAR: {
                Stmt* decl = env_id_to_stmt(e->env, e->Var.id.val.s, NULL);
                if (decl == NULL) {
                    char err[512];
                    sprintf(err, "undeclared variable \"%s\"", e->Var.id.val.s);
                    return err_message(e->Var.id, err);
                }
                break;
            }
            case EXPR_DISC:
            case EXPR_PRED:
            case EXPR_CONS: {
                Tk* sub = (e->sub==EXPR_DISC ? &e->Disc.subtype : (e->sub==EXPR_PRED ? &e->Pred.subtype : &e->Cons.subtype));
                if (sub->enu == TX_NIL) {
                    Stmt* decl = env_id_to_stmt(e->env, sub->val.s, NULL);
                    if (decl == NULL) {
                        char err[512];
                        sprintf(err, "undeclared type \"%s\"", sub->val.s);
                        return err_message(*sub, err);
                    }
                } else {
                    Stmt* user = env_sub_id_to_user_stmt(e->env, sub->val.s);
                    if (user == NULL) {
                        char err[512];
                        sprintf(err, "undeclared subtype \"%s\"", sub->val.s);
                        return err_message(*sub, err);
                    }
                }
                break;
            }
            default:
                break;
        }
        return 1;
    }

    return visit_stmt(s, NULL, fe, ft);
}

///////////////////////////////////////////////////////////////////////////////

int check_types (Stmt* S) {
    int type_is_sup_sub (Type* sup, Type* sub) {
        if (sup->sub != sub->sub) {
            return 0;   // different TYPE_xxx
        }
        if (sup->isalias != sub->isalias) {
            return 0;
        }
        switch (sup->sub) {
            case TYPE_USER:
                return (!strcmp(sup->User.val.s,sub->User.val.s) &&
                        sup->isalias == sub->isalias);

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

    int fe (Expr* e) {
        switch (e->sub) {
            case EXPR_UNIT:
            case EXPR_NATIVE:
            case EXPR_VAR:
            case EXPR_TUPLE:
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
                Type* func = env_expr_to_type(e->Call.func);
                Type* arg  = env_expr_to_type(e->Call.arg);

                if (e->Call.func->sub == EXPR_NATIVE) {
                    TODO("TODO [check_types]: _f(...)\n");
                } else if (!strcmp(e->Call.func->Var.id.val.s,"output")) {
                    TODO("TODO [check_types]: output(...)\n");
                } else if (!type_is_sup_sub(func->Func.inp, arg)) {
                    assert(e->Call.func->sub == EXPR_VAR);
                    char err[512];
                    sprintf(err, "invalid call to \"%s\" : type mismatch", e->Call.func->Var.id.val.s);
                    return err_message(e->Call.func->Var.id, err);
                }
                break;
            }

            case EXPR_CONS: {
                Type* subtype;
                if (e->Cons.subtype.enu == TX_NIL) {
                    subtype = &Type_Unit;
                } else {
                    Sub* sub = env_find_sub(e->env, e->Cons.subtype.val.s);
                    assert(sub != NULL);
                    subtype = &sub->type;
                }
                if (!type_is_sup_sub(subtype, env_expr_to_type(e->Cons.arg))) {
                    char err[512];
                    sprintf(err, "invalid constructor \"%s\" : type mismatch", e->Cons.subtype.val.s);
                    return err_message(e->Cons.subtype, err);
                }
            }
        }
        return 1;
    }

    int fs (Stmt* s) {
        switch (s->sub) {
            case STMT_FUNC:
            case STMT_SEQ:
            case STMT_BLOCK:
                break;

            case STMT_VAR:
                if (!type_is_sup_sub(&s->Var.type, env_expr_to_type(&s->Var.init))) {
                    char err[512];
                    sprintf(err, "invalid assignment to \"%s\" : type mismatch", s->Var.id.val.s);
                    return err_message(s->Var.id, err);
                }
                break;

            case STMT_USER: // no & in subtypes
                TODO("TODO [check_types]: STMT_USER\n");
                break;

            case STMT_CALL: {
                Type* tp = env_expr_to_type(&s->Call);
                if (tp->sub != TYPE_UNIT) {
                    assert(s->Call.sub == EXPR_CALL);
                    Expr* func = s->Call.Call.func;
                    assert(func->sub==EXPR_VAR || func->sub==EXPR_NATIVE);
                    if (func->sub == EXPR_VAR) {
                        char err[512];
                        sprintf(err, "invalid call to \"%s\" : missing return assignment",
                                func->Var.id.val.s);
                        return err_message(s->tk, err);
                    }
                }
                break;
            }

            case STMT_IF:
                if (!type_is_sup_sub(&Type_Bool, env_expr_to_type(&s->If.cond))) {
                    return err_message(s->tk, "invalid condition : type mismatch");
                }
                break;

            case STMT_RETURN: {
                Stmt* func = env_stmt_to_func(s);
                assert(func != NULL);
                assert(func->Func.type.sub == TYPE_FUNC);
                if (!type_is_sup_sub(func->Func.type.Func.out, env_expr_to_type(&s->Return))) {
                    return err_message(s->tk, "invalid return : type mismatch");
                }
                break;
            }
        }
        return 1;
    }

    return visit_stmt(S, fs, fe, NULL);
}

///////////////////////////////////////////////////////////////////////////////

// Set EXPR_VAR.istx=1 for root recursive and not alias/alias type.
//      f(nat)          -- istx=1       -- root, recursive
//      return nat      -- istx=1       -- root, recursive
//      output(&nat)    -- istx=0       -- root, recursive, alias
//      f(alias_nat)    -- istx=0       -- root, recursive, alias type
//      nat.xxx         -- istx=0       -- not root, recursive

void set_vars_istx (Stmt* s) {
    auto int fe (Expr* e);
    visit_stmt(s, NULL, fe, NULL);

    int fe (Expr* e) {
        switch (e->sub) {
            case EXPR_ALIAS:
            case EXPR_DISC:
            case EXPR_PRED:
                // keep istx=0
                return VISIT_BREAK;
            case EXPR_VAR: {
                Type* tp = env_expr_to_type(e);
                if (tp->sub==TYPE_USER && !tp->isalias) {
                    Stmt* user = env_id_to_stmt(e->env, tp->User.val.s, NULL);
                    if (user->User.isrec) {
                        e->Var.istx = 1;
                    }
                }
                break;
            }
            default:
                break;
        }
        return 1;
    }
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
    auto int fs1 (Stmt* s1);
    return visit_stmt(S, fs1, NULL, NULL);

    int fs1 (Stmt* s1) {
        if (s1->sub != STMT_VAR) {
            return 1;               // not var declaration
        }
        if (s1->Var.type.sub != TYPE_USER) {
            return 1;               // not user type
        }

        Stmt* user = env_id_to_stmt(s1->env, s1->Var.type.User.val.s, NULL);
        assert(user!=NULL && user->sub==STMT_USER);
        if (!user->User.isrec) {
            return 1;               // not recursive type
        }

        // var n: Nat = ...         // starting from each declaration

        typedef enum { NONE, TRANSFERRED, BORROWED } State;
        State state = NONE;
        Expr* e1 = NULL;

        typedef struct { Stmt* s; int state; } Stack;
        Stack stack[256];
        int stack_n = 0;

        Stmt* aliases[256] = { s1 }; // list of declarations that reach me, start with myself
        int aliases_n = 1;           // if someneone points to theses, it also points to myself

        auto int fs2 (Stmt* s2);
        auto int fe2 (Expr* e2);

        return exec(s1->seqs[1], fs2, fe2);

        // ... n ...                // check all accesses to it

        int fs2 (Stmt* s2) {
            Stmt* decl = env_id_to_stmt(s2->env, s1->Var.id.val.s, NULL);
            if (decl!=NULL && decl==s1) {
                // ok: s1 is still in scope
            } else {
                return EXEC_HALT;           // no: s1 is not in scope
            }

            // Rule 3
            switch (s2->sub) {
                case STMT_VAR: {
                    // var s1: T = ...
                    // var s2: &T = &y;      <-- if y reaches s1, so does s2
                    if (s2->Var.type.isalias) {
                        // get "var" being aliased
                        Expr* var = NULL;
                        if (s2->Var.init.sub == EXPR_ALIAS) {
                            assert(s2->Var.init.Alias->sub == EXPR_VAR);
                            var = s2->Var.init.Alias;
                        } else if (s2->Var.init.sub == EXPR_VAR) {
                            Type* tp = env_expr_to_type(&s2->Var.init);
                            assert(tp->isalias);
                            var = &s2->Var.init;
                        }
                        assert(var != NULL);

                        decl = env_id_to_stmt(s2->env, var->Var.id.val.s, NULL);
                        assert(decl != NULL);

                        // check if y reaches s1
                        // if reaches, include s2 in the list
                        for (int i=0; i<aliases_n; i++) {
                            if (aliases[i] == decl) {
                                assert(aliases_n < 256);
                                aliases[aliases_n++] = s2;
                                break;
                            }
                        }
                    }
                    break;
                }
                case STMT_RETURN: {
                    Type* tp = env_expr_to_type(&s2->Return);
                    if (tp->isalias) {
                        assert(s2->Return.sub == EXPR_VAR);
                        int scope;
                        Stmt* decl = env_id_to_stmt(s2->env, s2->Return.Var.id.val.s, &scope);
                        assert(decl != NULL);
                        if (scope == 0) {
                            for (int i=0; i<aliases_n; i++) {
                                if (aliases[i] == decl) {
                                    char err[1024];
                                    sprintf(err, "invalid return : cannot return alias to local \"%s\" (ln %ld)",
                                            s1->Var.id.val.s, s1->Var.id.lin);
                                    err_message(s2->Return.Var.id, err);
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

            if (s2->sub == STMT_BLOCK) {        // enter block: push state
                stack[stack_n].s = s2->seqs[0];
                stack[stack_n].state = state;
                assert(stack_n < 256);
                stack_n++;
            }

            if (s2 == stack[stack_n-1].s) {
                stack_n--;
                if (state != TRANSFERRED) {
                    state = stack[stack_n].state;   // leave block: pop state
                }
            }

            return EXEC_CONTINUE;
        }

        int fe2 (Expr* e2) {
            int isalias = 0;
            switch (e2->sub) {
                case EXPR_UNIT:
                case EXPR_NATIVE:
                case EXPR_TUPLE:
                case EXPR_CALL:
                case EXPR_CONS:
                    return EXEC_CONTINUE;

                case EXPR_INDEX:
                    if (e2->Index.tuple->sub == EXPR_VAR) {
                        return EXEC_BREAK;
                    }
                    return fe2(e2->Index.tuple);
                case EXPR_DISC:
                    if (e2->Disc.val->sub == EXPR_VAR) {
                        return EXEC_BREAK;
                    }
                    return fe2(e2->Disc.val);
                case EXPR_PRED:
                    if (e2->Pred.val->sub == EXPR_VAR) {
                        return EXEC_BREAK;
                    }
                    return fe2(e2->Pred.val);

                case EXPR_ALIAS:
                    isalias = 1;
                    e2 = e2->Alias;
                    assert(e2->sub == EXPR_VAR);
                    // continue to EXPR_VAR as an alias

                case EXPR_VAR:
                    if (strcmp(s1->Var.id.val.s,e2->Var.id.val.s)) {
                        return EXEC_CONTINUE;   // not same id
                    }

                    // ensure that EXPR_VAR is really same as STMT_VAR
                    Stmt* decl = env_id_to_stmt(e2->env, e2->Var.id.val.s, NULL);
                    assert(decl!=NULL && decl==s1);

                    // if already moved, it doesn't matter, any access is invalid

                    switch (state) {
                        case NONE:
                            break;
                        case TRANSFERRED: {  // Rule 6
                            assert(e1 != NULL);
                            char err[1024];
                            sprintf(err, "invalid access to \"%s\" : ownership was transferred (ln %ld)",
                                    e2->Var.id.val.s, e1->Var.id.lin);
                            err_message(e2->Var.id, err);
                            return EXEC_ERROR;
                        }
                        case BORROWED: {    // Rule 5
                            assert(e1 != NULL);
                            if (!isalias) {
                                char err[1024];
                                sprintf(err, "invalid transfer of \"%s\" : active alias in scope (ln %ld)",
                                        e2->Var.id.val.s, e1->Var.id.lin);
                                err_message(e2->Var.id, err);
                                return EXEC_ERROR;
                            }
                        }
                    }
                    e1 = e2;
                    if (isalias) {
                        assert(state != TRANSFERRED && "bug found");
                        state = BORROWED;
                        return EXEC_BREAK;      // do not visit EXPR_VAR again
                    } else {
                        state = TRANSFERRED;
                        return EXEC_CONTINUE;
                    }
            }
            assert(0 && "bug found");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

int env (Stmt* s) {
    set_envs(s);
    if (!check_undeclareds(s)) {
        return 0;
    }
    if (!check_types(s)) {
        return 0;
    }
    set_vars_istx(s);
    if (!check_owner_alias(s)) {
        return 0;
    }
    return 1;
}
