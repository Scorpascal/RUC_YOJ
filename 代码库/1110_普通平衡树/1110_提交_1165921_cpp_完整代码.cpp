#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAXN 100005

typedef struct Node {
    int val, key, size, cnt;
    struct Node *l, *r;
} Node;

Node pool[MAXN * 2];
int pool_cnt = 0;

Node* new_node(int val) {
    Node* p = &pool[pool_cnt++];
    p->val = val;
    p->key = rand();
    p->size = p->cnt = 1;
    p->l = p->r = NULL;
    return p;
}

int node_size(Node* p) {
    return p ? p->size : 0;
}

void pushup(Node* p) {
    if (p) p->size = node_size(p->l) + node_size(p->r) + p->cnt;
}

void rotate_left(Node** p) {
    Node* q = (*p)->r;
    (*p)->r = q->l;
    q->l = *p;
    pushup(*p);
    pushup(q);
    *p = q;
}

void rotate_right(Node** p) {
    Node* q = (*p)->l;
    (*p)->l = q->r;
    q->r = *p;
    pushup(*p);
    pushup(q);
    *p = q;
}

void insert(Node** p, int val) {
    if (!*p) {
        *p = new_node(val);
        return;
    }
    if ((*p)->val == val) {
        (*p)->cnt++;
    } else if (val < (*p)->val) {
        insert(&(*p)->l, val);
        if ((*p)->l->key > (*p)->key)
            rotate_right(p);
    } else {
        insert(&(*p)->r, val);
        if ((*p)->r->key > (*p)->key)
            rotate_left(p);
    }
    pushup(*p);
}

void erase(Node** p, int val) {
    if (!*p) return;
    if ((*p)->val == val) {
        if ((*p)->cnt > 1) {
            (*p)->cnt--;
        } else {
            if (!(*p)->l || !(*p)->r) {
                *p = (*p)->l ? (*p)->l : (*p)->r;
            } else {
                if ((*p)->l->key > (*p)->r->key) {
                    rotate_right(p);
                    erase(&(*p)->r, val);
                } else {
                    rotate_left(p);
                    erase(&(*p)->l, val);
                }
            }
        }
    } else if (val < (*p)->val) {
        erase(&(*p)->l, val);
    } else {
        erase(&(*p)->r, val);
    }
    if (*p) pushup(*p);
}

int get_rank(Node* p, int val) {
    if (!p) return 1;
    if (val == p->val) {
        return node_size(p->l) + 1;
    } else if (val < p->val) {
        return get_rank(p->l, val);
    } else {
        return node_size(p->l) + p->cnt + get_rank(p->r, val);
    }
}

int get_kth(Node* p, int k) {
    if (!p) return -1;
    if (k <= node_size(p->l)) {
        return get_kth(p->l, k);
    } else if (k <= node_size(p->l) + p->cnt) {
        return p->val;
    } else {
        return get_kth(p->r, k - node_size(p->l) - p->cnt);
    }
}

int get_prev(Node* p, int val) {
    int res = -2147483647;
    while (p) {
        if (p->val < val) {
            if (p->val > res) res = p->val;
            p = p->r;
        } else {
            p = p->l;
        }
    }
    return res;
}

int get_next(Node* p, int val) {
    int res = 2147483647;
    while (p) {
        if (p->val > val) {
            if (p->val < res) res = p->val;
            p = p->l;
        } else {
            p = p->r;
        }
    }
    return res;
}

int main() {
    srand((unsigned)time(NULL));
    int n, opt, x;
    Node* root = NULL;
    scanf("%d", &n);
    while (n--) {
        scanf("%d%d", &opt, &x);
        if (opt == 1) {
            insert(&root, x);
        } else if (opt == 2) {
            erase(&root, x);
        } else if (opt == 3) {
            printf("%d\n", get_rank(root, x));
        } else if (opt == 4) {
            printf("%d\n", get_kth(root, x));
        } else if (opt == 5) {
            printf("%d\n", get_prev(root, x));
        } else if (opt == 6) {
            printf("%d\n", get_next(root, x));
        }
    }
    return 0;
}