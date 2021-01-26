#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "all.h"

int err (Tk* tk, const char* v) {
    sprintf(ALL.err, "(ln %d, col %d): %s", tk->lin, tk->col, v);
    return 0;
}

int err_expected (const char* v) {
    sprintf(ALL.err, "(ln %d, col %d): expected %s : have %s",
        ALL.tk1.lin, ALL.tk1.col, v, lexer_tk2str(&ALL.tk1));
    return 0;
}

int err_unexpected (const char* v) {
    sprintf(ALL.err, "(ln %d, col %d): unexpected %s : wanted %s",
        ALL.tk0.lin, ALL.tk0.col, lexer_tk2str(&ALL.tk0), v);
    return 0;
}

int accept (TK enu) {
    if (ALL.tk1.enu == enu) {
        lexer();
        return 1;
    } else {
        return 0;
    }
}

int accept_err (TK enu) {
    int ret = accept(enu);
    if (ret == 0) {
        err_expected(lexer_tk2err(enu));
        //puts(ALL.err);
    }
    return ret;
}

int check (TK enu) {
    return (ALL.tk1.enu == enu);
}

Expr* expr_leftmost (Expr* e) {
    switch (e->sub) {
        case EXPR_UPREF:
            return expr_leftmost(e->Upref);
        case EXPR_DNREF:
            return expr_leftmost(e->Dnref);
        case EXPR_INDEX:
            return expr_leftmost(e->Index.val);
        case EXPR_DISC:
            return expr_leftmost(e->Disc.val);
        default:
            return e;
    }
}

Expr* expr_leftmost_n (Expr* e, int* n) {
    Expr* ret;
    switch (e->sub) {
        case EXPR_UPREF:
            ret = expr_leftmost_n(e->Upref, n);
            break;
        case EXPR_DNREF:
            ret = expr_leftmost_n(e->Dnref, n);
            break;
        case EXPR_INDEX:
            ret = expr_leftmost_n(e->Index.val, n);
            break;
        case EXPR_DISC:
            ret = expr_leftmost_n(e->Disc.val, n);
            break;
        default:
            ret = e;
            break;
    }
    if (*n == 0) {
        return ret;
    }
    (*n)--;
    return e;
}

///////////////////////////////////////////////////////////////////////////////

int parser_type (Type** ret) {
    Type* tp = malloc(sizeof(Type));
    assert(tp != NULL);
    *ret = tp;

    int isalias = accept('\\');

// TYPE_UNIT
    if (accept(TK_UNIT)) {
        *tp = (Type) { TYPE_UNIT, isalias };

// TYPE_PARENS / TYPE_TUPLE
    } else if (accept('(')) {
        if (!parser_type(ret)) {
            return 0;
        }

// TYPE_PARENS
        if (accept(')')) {
            free(tp);
            return 1;
        }

// TYPE_TUPLE
        if (!accept_err(',')) {
            return 0;
        }

        int n = 1;
        Type** vec = malloc(n*sizeof(Type*));
        assert(vec != NULL);
        vec[n-1] = *ret;

        do {
            Type* tp2;
            if (!parser_type(&tp2)) {
                return 0;
            }
            n++;
            vec = realloc(vec, n*sizeof(Type*));
            vec[n-1] = tp2;
        } while (accept(','));

        if (!accept_err(')')) {
            return 0;
        }
        *tp = (Type) { TYPE_TUPLE, isalias, .Tuple={n,vec} };
        *ret = tp;

// TYPE_NATIVE
    } else if (accept(TX_NATIVE)) {
        *tp = (Type) { TYPE_NATIVE, isalias, .Native=ALL.tk0 };

// TYPE_USER
    } else if (accept(TX_USER)) {
        *tp = (Type) { TYPE_USER, isalias, .User=ALL.tk0 };

    } else {
        return err_expected("type");
    }

// TYPE_FUNC
    if (accept(TK_ARROW)) {
        Type* tp2;
        if (!parser_type(&tp2)) {
            return 0;
        }

        Type* func = malloc(sizeof(Type));
        assert(func != NULL);

        *func = (Type) { TYPE_FUNC, 0, .Func={tp,tp2} };
        *ret = func;
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_expr_ (Expr** ret)
{
    Expr* e = malloc(sizeof(Expr));
    assert(e != NULL);
    *ret = e;

// EXPR_UNIT
    if (accept(TK_UNIT)) {
        *e = (Expr) { ALL.nn++, EXPR_UNIT, .Unit=ALL.tk0 };

// EXPR_UNK
    } else if (accept('?')) {
        *e = (Expr) { ALL.nn++, EXPR_UNK, .Unk=ALL.tk0 };

// EXPR_NULL
    } else if (accept(TX_NULL)) {   // $Nat
        *e = (Expr) { ALL.nn++, EXPR_NULL, .Null=ALL.tk0 };

// EXPR_INT
    } else if (accept(TX_NUM)) {    // 10
        *e = (Expr) { ALL.nn++, EXPR_INT, .Int=ALL.tk0 };

// EXPR_NATIVE
    } else if (accept(TX_NATIVE)) {
        *e = (Expr) { ALL.nn++, EXPR_NATIVE, .Native=ALL.tk0 };

// EXPR_VAR
    } else if (accept(TX_VAR)) {
        *e = (Expr) { ALL.nn++, EXPR_VAR, .Var={ALL.tk0,0,0} };

// EXPR_UPREF
    } else if (accept('\\')) {
        Expr* val;
        Tk tk = ALL.tk0;
        if (!parser_expr(&val,0)) {
            return 0;
        }
        if (val->sub==EXPR_VAR || val->sub==EXPR_INDEX || val->sub==EXPR_DISC) {
            *e = (Expr) { ALL.nn++, EXPR_UPREF, .Upref=val };
        } else {
            return err(&tk, "unexpected operand to `\\´");
        }

// EXPR_PARENS / EXPR_TUPLE
    } else if (accept('(')) {
        if (!parser_expr(ret,0)) {
            return 0;
        }

// EXPR_PARENS
        if (accept(')')) {
            free(e);
            return 1;
        }

// EXPR_TUPLE
        if (!accept_err(',')) {
            return 0;
        }

        int n = 1;

        Expr** vec = malloc(n*sizeof(Expr*));
        assert(vec != NULL);
        vec[n-1] = *ret;

        do {
            Expr* e2;
            if (!parser_expr(&e2,0)) {
                return 0;
            }
            n++;
            vec = realloc(vec, n*sizeof(Expr*));
            vec[n-1] = e2;
        } while (accept(','));

        if (!accept_err(')')) {
            return 0;
        }

        *e = (Expr) { ALL.nn++, EXPR_TUPLE, .Tuple={n,vec} };
        *ret = e;

// EXPR_CONS
    } else if (accept(TX_USER)) {  // True
        Tk sub = ALL.tk0;

        Expr* arg;
        if (!parser_expr(&arg,0)) {   // ()
            arg = malloc(sizeof(Expr));
            assert(arg != NULL);
            *arg = (Expr) { ALL.nn++, EXPR_UNIT, .Unit={TK_UNIT,{},0,ALL.nn++} };
        }

        *e = (Expr) { ALL.nn++, EXPR_CONS, .Cons={sub,arg} };

    } else {
        return err_expected("expression");
    }
    return 1;
}

int parser_expr (Expr** ret, int canpre) {
    int ispre = canpre && (accept(TK_CALL) || accept(TK_INPUT) || accept(TK_OUTPUT));
    Tk pre = ALL.tk0;

    Expr* cur;
    if (!parser_expr_(&cur)) {
        return 0;
    }
    *ret = cur;

    int isvarnat = (cur->sub==EXPR_VAR || cur->sub==EXPR_NATIVE);

    if (ispre) {
        if (!isvarnat) {
            return err_unexpected("variable identifier");
        }
        if (pre.enu != TK_CALL) {
            char tmp[256];
            strcpy(tmp, pre.val.s);
            strcat(tmp, "_");
            strcat(tmp, cur->Var.tk.val.s);
            strcpy(cur->Var.tk.val.s, tmp);
        }
    }

    while (1)
    {
        Expr* new = malloc(sizeof(Expr));
        assert(new != NULL);
        Expr* arg;

        Tk tk_bef = ALL.tk0;

        if (accept('.')) {
// EXPR_INDEX
            if (accept(TX_NUM)) {
                *new = (Expr) { ALL.nn++, EXPR_INDEX, .Index={cur,ALL.tk0} };

// EXPR_DISC / EXPR_PRED
            } else if (accept(TX_USER) || accept(TX_NULL)) {
                Tk tk = ALL.tk0;

                if (accept('?')) {
                    *new = (Expr) { ALL.nn++, EXPR_PRED, .Pred={cur,tk} };
                } else if (accept('!')) {
                    *new = (Expr) { ALL.nn++, EXPR_DISC, .Disc={cur,tk} };
                } else {
                    return err_expected("`?´ or `!´");
                }

            } else {
                return err_expected("index or subtype");
            }

        } else if (accept('\\')) {
// EXPR_DNREF
            if (cur->sub==EXPR_NATIVE || cur->sub==EXPR_VAR   ||
                cur->sub==EXPR_UPREF  || cur->sub==EXPR_DNREF ||
                cur->sub==EXPR_INDEX  || cur->sub==EXPR_CALL  ||
                cur->sub==EXPR_DISC) {
                *new = (Expr) { ALL.nn++, EXPR_DNREF, .Dnref=cur };
            } else {
                return err(&ALL.tk0, "unexpected operand to `\\´");
            }

// EXPR_CALL
        } else if (isvarnat && parser_expr(&arg,0)) {
            *new = (Expr) { ALL.nn++, EXPR_CALL, .Call={cur,arg} };
        } else if (isvarnat && ispre) {
            arg = malloc(sizeof(Expr));
            assert(arg != NULL);
            *arg = (Expr) { ALL.nn++, EXPR_UNIT, .Unit={TK_UNIT,{},0,ALL.nn++} };
            *new = (Expr) { ALL.nn++, EXPR_CALL, .Call={cur,arg} };
            break;

        } else {
            free(new);
            if (tk_bef.lin==ALL.tk0.lin && tk_bef.col==ALL.tk0.col) {
                break;      // nothing extra read: OK!
            } else {
                return 0;   // extra token read:   NO!
            }
        }

        isvarnat = 0; // only on first iteration
        *ret = new;
        cur = new;
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_stmt_sub (Sub* ret) {
    if (!accept_err(TX_USER)) {
        return 0;
    }
    Tk id = ALL.tk0;                // True

    if (!accept_err(':')) {         // :
        return 0;
    }

    Type* tp;
    if (!parser_type(&tp)) {        // ()
        return 0;
    }

    *ret = (Sub) { id, tp };
    return 1;
}

int parser_stmts (TK opt, Stmt** ret);

int parser_stmt (Stmt** ret) {
    Stmt* s = malloc(sizeof(Stmt));
    assert(s != NULL);
    *ret = s;

    int parser_block (Stmt** ret) {
        if (!accept('{')) {
            return 0;
        }
        Tk tk = ALL.tk0;

        Stmt* blk;
        if (!parser_stmts('}',&blk)) {
            return 0;
        }

        if (!accept_err('}')) { return 0; }

        Stmt* block = malloc(sizeof(Stmt));
        assert(block != NULL);
        *block = (Stmt) { ALL.nn++, STMT_BLOCK, NULL, {NULL,NULL}, tk, .Block=blk };
        *ret = block;
        return 1;
    }

    // STMT_VAR
    if (accept(TK_VAR)) {
        Tk tk = ALL.tk0;
        if (!accept_err(TX_VAR)) {
            return 0;
        }
        Tk id = ALL.tk0;

        if (!accept_err(':')) {
            return 0;
        }
        Type* tp;
        if (!parser_type(&tp)) {
            return 0;
        }
        if (!accept_err('=')) {
            return 0;
        }

        Expr* e;
        if (!parser_expr(&e,1)) {
            return 0;
        }

        *s = (Stmt) { ALL.nn++, STMT_VAR, NULL, {NULL,NULL}, tk, .Var={id,tp,e,NULL,-1} };

    // STMT_USER
    } else if (accept(TK_TYPE)) {       // type
        Tk tk = ALL.tk0;

        int ispre = 0;
        int isrec = 0;
        if (accept(TK_PRE)) {           // pre
            ispre = 1;
        } else if (accept(TK_REC)) {    // rec
            isrec = 1;
        }
        if (!accept_err(TX_USER)) {
            return 0;
        }
        Tk id = ALL.tk0;                // Bool

        if (ispre) {
            *s = (Stmt) { ALL.nn++, STMT_USER, NULL, {NULL,NULL}, tk, .User={1,id,0,NULL} };
            return 1;
        }

        if (!accept_err('{')) {
            return 0;
        }

        Sub sub;
        if (!parser_stmt_sub(&sub)) {     // False ()
            return 0;
        }
        int n = 1;
        Sub* vec = malloc(n*sizeof(Sub));
        assert(vec != NULL);
        vec[n-1] = sub;

        while (1) {
            accept(';');    // optional
            Sub sub;
            if (!parser_stmt_sub(&sub)) { // True ()
                break;
            }
            n++;
            vec = realloc(vec, n*sizeof(Sub));
            vec[n-1] = sub;
        }

        if (!accept_err('}')) {
            return 0;
        }

        *s = (Stmt) { ALL.nn++, STMT_USER, NULL, {NULL,NULL}, tk, .User={isrec,id,n,vec} };

    // STMT_SET
    } else if (accept(TK_SET)) {
        Tk tk = ALL.tk0;

        Expr* dst;
        if (!parser_expr(&dst,0)) {
            return 0;
        }

        if (!accept_err('=')) {
            return 0;
        }

        Expr* src;
        if (!parser_expr(&src,1)) {
            return 0;
        }

        *s = (Stmt) { ALL.nn++, STMT_SET, NULL, {NULL,NULL}, tk, .Set={dst,src} };

    // STMT_CALL
    } else if (check(TK_CALL) || check(TK_INPUT) || check(TK_OUTPUT)) {
        Tk tk = ALL.tk0;
        Expr* e;
        if (!parser_expr(&e,1)) {
            return 0;
        }
        assert(e->sub == EXPR_CALL);
        *s = (Stmt) { ALL.nn++, STMT_CALL, NULL, {NULL,NULL}, tk, .Call=e };

    // STMT_IF
    } else if (accept(TK_IF)) {         // if
        Tk tk = ALL.tk0;

        Expr* e;
        if (!parser_expr(&e,0)) {         // x
            return 0;
        }

        Stmt *t,*f;

        if (!parser_block(&t)) {         // true()
            return 0;
        }

        if (accept(TK_ELSE)) {
            if (!parser_block(&f)) {     // false()
                return 0;
            }
        } else {
            Stmt* none = malloc(sizeof(Stmt));
            assert(none != NULL);
            *none = (Stmt) { ALL.nn++, STMT_NONE, NULL, {NULL,NULL} };

            f = malloc(sizeof(Stmt));
            assert(f != NULL);
            *f = (Stmt) { ALL.nn++, STMT_BLOCK, NULL, {NULL,NULL}, ALL.tk0, .Block=none };
        }

        *s = (Stmt) { ALL.nn++, STMT_IF, NULL, {NULL,NULL}, tk, .If={e,t,f} };

    // STMT_BREAK
    } else if (accept(TK_BREAK)) {      // break
        *s = (Stmt) { ALL.nn++, STMT_BREAK, NULL, {NULL,NULL}, ALL.tk0 };

    // STMT_LOOP
    } else if (accept(TK_LOOP)) {       // loop
        Tk tk = ALL.tk0;
        Stmt* body;
        if (!parser_block(&body)) {
            return 0;
        }
        *s = (Stmt) { ALL.nn++, STMT_LOOP, NULL, {NULL,NULL}, tk, .Loop=body };

    // STMT_FUNC
    } else if (accept(TK_FUNC)) {       // func
        Tk tk = ALL.tk0;

        int ispre = 0;
        if (accept(TK_PRE)) {           // pre
            ispre = 1;
        }

        if (!accept(TX_VAR)) {          // f
            return 0;
        }
        Tk id = ALL.tk0;
        if (!accept(':')) {             // :
            return 0;
        }
        Type* tp;
        if (!parser_type(&tp)) {        // () -> ()
            return 0;
        }

        if (ispre) {
            *s = (Stmt) { ALL.nn++, STMT_FUNC, NULL, {NULL,NULL}, tk, .Func={id,tp,NULL} };
            return 1;
        }

        Stmt* blk2;
        if (!parser_block(&blk2)) {
            return 0;
        }

        Stmt* blk1 = malloc(sizeof(Stmt));
        Stmt* seq1 = malloc(sizeof(Stmt));
        Stmt* arg  = malloc(sizeof(Stmt));
        Expr* expr = malloc(sizeof(Expr));
        assert(blk1!=NULL && seq1!=NULL && arg!=NULL && expr!=NULL);

        *blk1 = (Stmt) { ALL.nn++, STMT_BLOCK, NULL, {NULL,NULL}, tk, .Block=seq1 };
        *seq1 = (Stmt) { ALL.nn++, STMT_SEQ,   NULL, {NULL,NULL}, tk, .Seq={arg,blk2} };
        *expr = (Expr) { ALL.nn++, EXPR_NATIVE, {.Native={TX_NATIVE,{.s="_arg_"},id.lin,id.col}} };
        *arg  = (Stmt) { ALL.nn++, STMT_VAR,   NULL, {NULL,NULL}, tk,
            .Var = {
                { TX_VAR, {.s="arg"}, id.lin, id.col },
                tp->Func.inp,
                expr,
                NULL,-1 // ptr_deepest of "arg" is the scope of its enclosing function
            }
        };

        *s = (Stmt) { ALL.nn++, STMT_FUNC, NULL, {NULL,NULL}, tk, .Func={id,tp,blk1} };

    // STMT_RETURN
    } else if (accept(TK_RETURN)) {
        Tk tk = ALL.tk0;
        Expr* e;
        if (!parser_expr(&e,0)) {
            return 0;
        }
        *s = (Stmt) { ALL.nn++, STMT_RETURN, NULL, {NULL,NULL}, tk, .Return=e };

    // STMT_BLOCK
    } else if (check('{')) {
        return parser_block(ret);

    // STMT_NATIVE
    } else if (accept(TK_NATIVE)) {
        Tk tk = ALL.tk0;
        int ispre = accept(TK_PRE);
        if (!accept_err(TX_NATIVE)) {
            return 0;
        }
        *s = (Stmt) { ALL.nn++, STMT_NATIVE, NULL, {NULL,NULL}, tk, .Native={ispre,ALL.tk0} };

    } else {
        return err_expected("statement");
    }

    return 1;
}

Stmt* enseq (Stmt* s1, Stmt* s2) {
    if (s1 == NULL) {
        return s2;
    } else if (s2 == NULL) {
        return s1;
    } else if (s1->sub == STMT_NONE) {
        free(s1);
        return s2;
    } else if (s2->sub == STMT_NONE) {
        free(s2);
        return s1;
    } else {
        Stmt* ret = malloc(sizeof(Stmt));
        assert(ret != NULL);
        *ret = (Stmt) { ALL.nn++, STMT_SEQ,   NULL, {NULL,NULL}, ALL.tk0, .Seq={s1,s2} };
        return ret;
    }
}

int parser_stmts (TK opt, Stmt** ret) {
    Stmt* none = malloc(sizeof(Stmt));
    assert(none != NULL);
    *none = (Stmt) { ALL.nn++, STMT_NONE, NULL, {NULL,NULL} };
    *ret = none;

    while (1) {
        accept(';');    // optional
        Stmt* p;
        Tk tk_bef = ALL.tk0;
        if (!parser_stmt(&p)) {
            if (tk_bef.lin!=ALL.tk0.lin || tk_bef.col!=ALL.tk0.col) {
                return 0;   // extra token read:   NO!
            }
            if (check(opt)) {
                break;
            } else {
                return 0;
            }
        }
        *ret = enseq(*ret,p);
        accept(';');    // optional
    }
    return 1;
}

int parser (Stmt** ret) {
    static Type Type_Unit  = { TYPE_UNIT, 0 };
    static Type tp_any     = { TYPE_ANY,  0 };
    static Type tp_alias   = { TYPE_ANY,  1 };

    *ret = NULL;

    // Int, std, clone
    {
        static Stmt Int = (Stmt) {   // Int
            0, STMT_USER, NULL, {NULL,NULL},
            .User = { 0, {TX_USER,{.s="Int"},0,__COUNTER__}, 0, NULL }
        };
        *ret = enseq(*ret, &Int);
    }
    {
        static Type tp_stdo  = { TYPE_FUNC, .Func={&tp_alias,&Type_Unit} };
        static Stmt stdo = (Stmt) {   // std ()
            0, STMT_FUNC, NULL, {NULL,NULL},
            .Func = { {TX_VAR,{.s="output_std"},0,__COUNTER__}, &tp_stdo, NULL }
        };
        *ret = enseq(*ret, &stdo);
    }
    {
        static Type tp_clone = { TYPE_FUNC, .Func={&tp_alias,&tp_any} };
        static Stmt clone = (Stmt) {   // clone ()
            0, STMT_FUNC, NULL, {NULL,NULL},
            .Func = { {TX_VAR,{.s="clone"},0,__COUNTER__}, &tp_clone, NULL }
        };
        *ret = enseq(*ret, &clone);
    }

    Stmt* tmp;
    if (!parser_stmts(TK_EOF,&tmp)) {
        return 0;
    }

    *ret = enseq(*ret, tmp);

    if (!accept_err(TK_EOF)) {
        return 0;
    }
    return 1;
}
