#include <stdio.h>
#include <stdlib.h>

#define MAXV 50005
#define MAXE 100000

typedef struct {
    int u, v, w, color;
    long long key;
} Edge;

typedef struct {
    long long sum;
    long long aug;
    int red;
} MSTRes;

static Edge edges[MAXE];
static int order_idx[MAXE];
static int parent[MAXV];
static unsigned char rankv[MAXV];
static int vertex_count, edge_count;

static int cmp_edge_idx(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    if (edges[ia].key < edges[ib].key) return -1;
    if (edges[ia].key > edges[ib].key) return 1;
    if (edges[ia].color != edges[ib].color) return edges[ia].color == 1 ? -1 : 1;
    if (edges[ia].w != edges[ib].w) return edges[ia].w - edges[ib].w;
    if (edges[ia].u != edges[ib].u) return edges[ia].u - edges[ib].u;
    return edges[ia].v - edges[ib].v;
}

static int dsu_find(int x) {
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }
    return x;
}

static MSTRes run_mst(int penalty) {
    for (int i = 0; i < edge_count; ++i) {
        edges[i].key = edges[i].w + (edges[i].color == 0 ? (long long)penalty : 0);
        order_idx[i] = i;
    }
    qsort(order_idx, edge_count, sizeof(int), cmp_edge_idx);

    for (int i = 0; i < vertex_count; ++i) {
        parent[i] = i;
        rankv[i] = 0;
    }

    long long total = 0;
    int red = 0, used = 0;
    for (int i = 0; i < edge_count && used < vertex_count - 1; ++i) {
        Edge *e = &edges[order_idx[i]];
        int ru = dsu_find(e->u);
        int rv = dsu_find(e->v);
        if (ru == rv) continue;
        if (rankv[ru] < rankv[rv]) {
            parent[ru] = rv;
        } else if (rankv[ru] > rankv[rv]) {
            parent[rv] = ru;
        } else {
            parent[rv] = ru;
            ++rankv[ru];
        }
        total += e->w;
        if (e->color == 0) ++red;
        ++used;
    }

    MSTRes res;
    res.sum = total;
    res.red = red;
    res.aug = total + (long long)penalty * red;
    return res;
}

int main(void) {
    int need;
    if (scanf("%d%d%d", &vertex_count, &edge_count, &need) != 3) return 0;
    for (int i = 0; i < edge_count; ++i) {
        int u, v, w, c;
        scanf("%d%d%d%d", &u, &v, &w, &c);
        edges[i].u = u;
        edges[i].v = v;
        edges[i].w = w;
        edges[i].color = (c == 0) ? 0 : 1;
    }

    int lo = -105, hi = 105, best = lo;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        MSTRes res = run_mst(mid);
        if (res.red >= need) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    MSTRes r1 = run_mst(best);
    long long ans1 = r1.sum + (long long)best * (r1.red - need);

    MSTRes r2 = run_mst(best + 1);
    long long ans2 = r2.sum + (long long)(best + 1) * (r2.red - need);

    long long ans = (ans1 > ans2) ? ans1 : ans2;

    printf("%lld\n", ans);
    return 0;
}