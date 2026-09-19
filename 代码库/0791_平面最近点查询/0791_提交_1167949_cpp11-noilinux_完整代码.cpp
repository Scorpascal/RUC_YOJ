#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAXN 100000

typedef struct {
    long long x, y;
} Point;

typedef struct {
    long long x, y;
    int left, right;
    long long minx, maxx, miny, maxy;
} Node;

static Point points[MAXN];
static Node nodes[MAXN];
static int order_idx[MAXN];
static int node_cnt = 0;

static long long coord(int idx, int dim) {
    return dim == 0 ? points[idx].x : points[idx].y;
}

static int cmp_dim(int idx_a, int idx_b, int dim) {
    long long va = coord(idx_a, dim);
    long long vb = coord(idx_b, dim);
    if (va < vb) return -1;
    if (va > vb) return 1;
    long long va2 = coord(idx_a, dim ^ 1);
    long long vb2 = coord(idx_b, dim ^ 1);
    if (va2 < vb2) return -1;
    if (va2 > vb2) return 1;
    return (idx_a < idx_b) ? -1 : (idx_a > idx_b);
}

static void nth_element_dim(int l, int r, int k, int dim) {
    while (l < r) {
        int mid = (l + r) >> 1;
        int pivot_idx = order_idx[mid];
        int i = l, j = r;
        while (i <= j) {
            while (cmp_dim(order_idx[i], pivot_idx, dim) < 0) ++i;
            while (cmp_dim(order_idx[j], pivot_idx, dim) > 0) --j;
            if (i <= j) {
                int tmp = order_idx[i];
                order_idx[i] = order_idx[j];
                order_idx[j] = tmp;
                ++i;
                --j;
            }
        }
        if (k <= j) r = j;
        else if (k >= i) l = i;
        else break;
    }
}

static int build(int l, int r, int depth) {
    if (l > r) return -1;
    int dim = depth & 1;
    int mid = (l + r) >> 1;
    nth_element_dim(l, r, mid, dim);

    int idx = order_idx[mid];
    int cur = node_cnt++;
    Node *node = &nodes[cur];
    node->x = points[idx].x;
    node->y = points[idx].y;

    node->left = build(l, mid - 1, depth + 1);
    node->right = build(mid + 1, r, depth + 1);

    node->minx = node->maxx = node->x;
    node->miny = node->maxy = node->y;

    if (node->left != -1) {
        Node *ch = &nodes[node->left];
        if (ch->minx < node->minx) node->minx = ch->minx;
        if (ch->maxx > node->maxx) node->maxx = ch->maxx;
        if (ch->miny < node->miny) node->miny = ch->miny;
        if (ch->maxy > node->maxy) node->maxy = ch->maxy;
    }
    if (node->right != -1) {
        Node *ch = &nodes[node->right];
        if (ch->minx < node->minx) node->minx = ch->minx;
        if (ch->maxx > node->maxx) node->maxx = ch->maxx;
        if (ch->miny < node->miny) node->miny = ch->miny;
        if (ch->maxy > node->maxy) node->maxy = ch->maxy;
    }
    return cur;
}

static unsigned long long rect_dist(const Node *node, long long x, long long y) {
    long long dx = 0, dy = 0;
    if (x < node->minx) dx = node->minx - x;
    else if (x > node->maxx) dx = x - node->maxx;
    if (y < node->miny) dy = node->miny - y;
    else if (y > node->maxy) dy = y - node->maxy;
    return (unsigned long long)(dx * dx + dy * dy);
}

static void query(int cur, long long x, long long y, unsigned long long *best) {
    if (cur == -1) return;
    Node *node = &nodes[cur];
    long long dx = node->x - x;
    long long dy = node->y - y;
    unsigned long long dist = (unsigned long long)(dx * dx + dy * dy);
    if (dist < *best) *best = dist;

    int left = node->left;
    int right = node->right;
    unsigned long long dl = left == -1 ? ULLONG_MAX : rect_dist(&nodes[left], x, y);
    unsigned long long dr = right == -1 ? ULLONG_MAX : rect_dist(&nodes[right], x, y);

    if (dl < dr) {
        if (dl < *best) query(left, x, y, best);
        if (dr < *best) query(right, x, y, best);
    } else {
        if (dr < *best) query(right, x, y, best);
        if (dl < *best) query(left, x, y, best);
    }
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    for (int i = 0; i < n; ++i) {
        long long x, y;
        scanf("%lld%lld", &x, &y);
        points[i].x = x;
        points[i].y = y;
        order_idx[i] = i;
    }
    node_cnt = 0;
    int root = build(0, n - 1, 0);

    int q;
    scanf("%d", &q);
    while (q--) {
        long long x, y;
        scanf("%lld%lld", &x, &y);
        unsigned long long best = ULLONG_MAX;
        query(root, x, y, &best);
        printf("%llu\n", best);
    }
    return 0;
}