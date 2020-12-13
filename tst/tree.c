#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

enum {
    TREE_LEFT,
    TREE_RIGHT
};

typedef struct Tree {
    int N;
    struct Tree* L;
    struct Tree* R;
} Tree;

typedef struct State {
    int size;
    int vec[100];
    int cur;
} State;

void traverse_init (State* st) {
    st->size = 0;
}

int traverse (State* st, Tree* t) {
    st->cur = 0;
    auto int aux (State* st, Tree* t);
    int ret = aux(st, t);
    puts("");
    return ret;

    int aux (State* st, Tree* t) {
        // cheguei a uma folha
        if (t == NULL) {
            // anda pra cima enquanto todas as ultimas escolhas foram DIR
            while (st->size>0 && st->vec[st->size-1]==TREE_RIGHT) {
                st->size--;
            }
            // muda a ultima escolha de ESQ -> DIR e retorna 1 que ainda tem o que percorrer
            if (st->size>0 && st->vec[st->size-1]==TREE_LEFT) {
                st->vec[st->size-1] = TREE_RIGHT;
                return 1;
            }
            // retorna 0 que o percorrimento acabou
            return 0;
        }

        // cheguei em um galho
        printf("%d ", t->N);

        // ainda nao explorei esse galho
        if (st->cur == st->size) {
            st->cur++;
            st->vec[st->size++] = TREE_LEFT;  // escolho esquerda
            return aux(st, t->L);
        } else if (st->vec[st->cur] == TREE_LEFT) {
            st->cur++;
            return aux(st, t->L);
        } else if (st->vec[st->cur] == TREE_RIGHT) {
            st->cur++;
            return aux(st, t->R);
        } else {
            assert(0);
        }
    }
}

void main (void) {
    Tree x7 = { 7, NULL, NULL };
    Tree x6 = { 6, NULL, NULL };
    Tree x5 = { 5, NULL, NULL };
    Tree x4 = { 4, NULL, NULL };
    Tree x3 = { 3, &x4, &x5 };
    Tree x2 = { 2, &x3, &x6 };
    Tree x1 = { 1, &x2, &x7 };

    State st;
    traverse_init(&st);

    while (traverse(&st,&x1)) {
        puts("========");
    }
}
