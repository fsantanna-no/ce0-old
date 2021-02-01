#include <assert.h>
#include <stdlib.h>

#include "all.h"

///////////////////////////////////////////////////////////////////////////////

Stmt* stmt_xmost (Stmt* s, int right) {
    if (s->sub == STMT_SEQ) {
        return stmt_xmost((right ? s->Seq.s2 : s->Seq.s1), right);
    } else {
        return s;
    }
}

void set_seqs (Stmt* s, Stmt* nxt, Stmt* func, Stmt* loop) {
    s->seq = nxt;

    switch (s->sub) {
        case STMT_NONE:
        case STMT_VAR:
        case STMT_USER:
        case STMT_SET:
        case STMT_CALL:     // TODO: recurse into STMT_FUNC?
        case STMT_NATIVE:
            break;

       case STMT_SEQ: {
            Stmt* xxx = stmt_xmost(s->Seq.s2, 0);
            set_seqs(s->Seq.s1, xxx, func, loop);
            set_seqs(s->Seq.s2, nxt, func, loop);
            break;
        }

        case STMT_IF: {     // don't link true/false here
            set_seqs(s->If.true,  nxt, func, loop);
            set_seqs(s->If.false, nxt, func, loop);
            break;
        }

        case STMT_LOOP:
            set_seqs(s->Loop, NULL, func, nxt);     // NULL: only break goes to nxt
            s->seq = stmt_xmost(s->Loop, 0);
            break;
        case STMT_BREAK:
            s->seq = loop;
            break;

        case STMT_FUNC:
            if (s->Func.body != NULL) {
                set_seqs(s->Func.body, nxt, nxt, NULL); // nxt (not NULL): even w/o return
                s->seq = stmt_xmost(s->Func.body, 0);
            }
            break;
        case STMT_RETURN:
            s->seq = func;
            break;

        case STMT_BLOCK:
            set_seqs(s->Block, nxt, func, loop);
            s->seq = stmt_xmost(s->Block,0);
            break;
    }
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

int sets (Stmt* s) {
    set_seqs(s, NULL, NULL, NULL);
    assert(set_envs(s));
    return 1;
}
