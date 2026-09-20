#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 205

static int n, m;
static unsigned char blocked[MAXN][MAXN];

static int dx[8] = {-2,-2,-1,-1, 1, 1, 2, 2};
static int dy[8] = { 1,-1, 2,-2, 2,-2, 1,-1};

typedef struct {
    int *data;
    int size;
    int cap;
} Vec;

static Vec *adj;    // adjacency for left partition
static int *pairU;  // match for left nodes
static int *pairV;  // match for right nodes
static int *dist;   // BFS distance for Hopcroft-Karp

static int idL[MAXN][MAXN];
static int idR[MAXN][MAXN];
static int cntL = 0, cntR = 0;

static void vec_init(Vec *v) {
    v->data = NULL; v->size = 0; v->cap = 0;
}
static void vec_push(Vec *v, int x) {
    if (v->size == v->cap) {
        v->cap = v->cap ? v->cap * 2 : 4;
        v->data = (int*)realloc(v->data, v->cap * sizeof(int));
    }
    v->data[v->size++] = x;
}

static int queue_alloc(int n, int **qptr) {
    *qptr = (int*)malloc(sizeof(int) * n);
    return *qptr != NULL;
}

// Hopcroft-Karp BFS
static int bfs(void) {
    int *q;
    if (!queue_alloc(cntL + 5, &q)) return 0;
    int qs = 0, qe = 0;
    for (int u = 1; u <= cntL; ++u) {
        if (pairU[u] == 0) {
            dist[u] = 0;
            q[qe++] = u;
        } else {
            dist[u] = -1;
        }
    }
    int foundFree = 0;
    while (qs < qe) {
        int u = q[qs++];
        Vec *edges = &adj[u];
        for (int i = 0; i < edges->size; ++i) {
            int v = edges->data[i];
            int pu = pairV[v];
            if (pu == 0) {
                foundFree = 1;
            } else if (dist[pu] == -1) {
                dist[pu] = dist[u] + 1;
                q[qe++] = pu;
            }
        }
    }
    free(q);
    return foundFree;
}

// DFS layered
static int dfs(int u) {
    Vec *edges = &adj[u];
    for (int i = 0; i < edges->size; ++i) {
        int v = edges->data[i];
        int pu = pairV[v];
        if (pu == 0 || (dist[pu] == dist[u] + 1 && dfs(pu))) {
            pairU[u] = v;
            pairV[v] = u;
            return 1;
        }
    }
    dist[u] = -1; // prune
    return 0;
}

int main(void) {
    if (scanf("%d %d", &n, &m) != 2) return 0;
    memset(blocked, 0, sizeof(blocked));
    for (int i = 0; i < m; ++i) {
        int x, y;
        scanf("%d %d", &x, &y);
        if (x >= 1 && x <= n && y >= 1 && y <= n)
            blocked[x][y] = 1;
    }

    // assign ids to partitions by parity
    cntL = cntR = 0;
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (blocked[i][j]) { idL[i][j] = idR[i][j] = 0; continue; }
            if (((i + j) & 1) == 0)
                idL[i][j] = ++cntL, idR[i][j] = 0;
            else
                idR[i][j] = ++cntR, idL[i][j] = 0;
        }
    }

    adj = (Vec*)malloc((cntL + 1) * sizeof(Vec));
    for (int u = 0; u <= cntL; ++u) vec_init(&adj[u]);

    // build edges from left (even parity) to right (odd parity)
    for (int x = 1; x <= n; ++x) {
        for (int y = 1; y <= n; ++y) {
            int u = idL[x][y];
            if (u == 0) continue;
            for (int k = 0; k < 8; ++k) {
                int nx = x + dx[k], ny = y + dy[k];
                if (nx < 1 || nx > n || ny < 1 || ny > n) continue;
                if (blocked[nx][ny]) continue;
                int v = idR[nx][ny];
                if (v) vec_push(&adj[u], v);
            }
        }
    }

    pairU = (int*)calloc(cntL + 1, sizeof(int));
    pairV = (int*)calloc(cntR + 1, sizeof(int));
    dist  = (int*)malloc((cntL + 1) * sizeof(int));

    int matching = 0;
    while (bfs()) {
        for (int u = 1; u <= cntL; ++u) {
            if (pairU[u] == 0) matching += dfs(u);
        }
    }

    int totalAvailable = 0;
    for (int i = 1; i <= n; ++i)
        for (int j = 1; j <= n; ++j)
            if (!blocked[i][j]) totalAvailable++;

    int result = totalAvailable - matching;
    printf("%d\n", result);

    // cleanup
    for (int u = 0; u <= cntL; ++u) free(adj[u].data);
    free(adj); free(pairU); free(pairV); free(dist);
    return 0;
}