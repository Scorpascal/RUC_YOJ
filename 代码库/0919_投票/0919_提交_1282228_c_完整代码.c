#include <stdio.h>
#include <stdlib.h>

#define MAXN 10005
// 调大边容量，避免溢出（m<=5e4，谨慎给到 m+n 的上限）
#define MAXM 120000

typedef struct Edge {
    int to, next;
} Edge;

Edge edges[MAXM];
int head[MAXN], edge_cnt = 0;

int dfn[MAXN], low[MAXN], in_stack[MAXN], stk[MAXN], top = 0, ts = 0;
int comp_id[MAXN], comp_sz[MAXN], comp_cnt = 0;

void add_edge(int u, int v) {
    edges[edge_cnt].to = v;
    edges[edge_cnt].next = head[u];
    head[u] = edge_cnt++;
}

void tarjan(int u) {
    dfn[u] = low[u] = ++ts;
    stk[top++] = u;
    in_stack[u] = 1;
    for (int i = head[u]; i != -1; i = edges[i].next) {
        int v = edges[i].to;
        if (!dfn[v]) {
            tarjan(v);
            if (low[v] < low[u]) low[u] = low[v];
        } else if (in_stack[v]) {
            if (dfn[v] < low[u]) low[u] = dfn[v];
        }
    }
    if (low[u] == dfn[u]) {
        ++comp_cnt;
        while (1) {
            int x = stk[--top];
            in_stack[x] = 0;
            comp_id[x] = comp_cnt;
            comp_sz[comp_cnt]++;
            if (x == u) break;
        }
    }
}

int main() {
    int n, m;
    if (scanf("%d %d", &n, &m) != 2) return 0;

    for (int i = 1; i <= n; ++i) head[i] = -1;

    for (int i = 0; i < m; ++i) {
        int a, b;
        scanf("%d %d", &a, &b);
        add_edge(a, b);
    }

    // 去掉自环：SCC 自身可达，不需要额外加边
    // for (int i = 1; i <= n; ++i) add_edge(i, i);

    for (int i = 1; i <= n; ++i)
        if (!dfn[i]) tarjan(i);

    // 统计每个SCC在凝聚图中的出度
    int *outdeg = (int*)calloc(comp_cnt + 1, sizeof(int));
    for (int u = 1; u <= n; ++u) {
        for (int i = head[u]; i != -1; i = edges[i].next) {
            int v = edges[i].to;
            if (comp_id[u] != comp_id[v]) {
                outdeg[comp_id[u]]++;
            }
        }
    }

    int sink_comp = -1, sink_count = 0;
    for (int c = 1; c <= comp_cnt; ++c) {
        if (outdeg[c] == 0) {
            sink_count++;
            sink_comp = c;
        }
    }

    if (sink_count == 1) {
        printf("%d\n", comp_sz[sink_comp]);
    } else {
        printf("0\n");
    }

    free(outdeg);
    return 0;
}