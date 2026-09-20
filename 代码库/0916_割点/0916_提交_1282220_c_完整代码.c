#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int u;
} StackItem;

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    // 最大边数 <= 5e5，存储为无向边，需要双向存储 => 至多 1e6
    int maxE = 1000000 + 5;

    int *head = (int*)malloc((n + 1) * sizeof(int));
    int *to   = (int*)malloc((maxE) * sizeof(int));
    int *next = (int*)malloc((maxE) * sizeof(int));
    if (!head || !to || !next) return 0;
    for (int i = 1; i <= n; ++i) head[i] = 0;

    int u, v;
    int m = 0;
    while (scanf("%d %d", &u, &v) == 2) {
        if (u == 0 && v == 0) break;
        // 忽略非法点
        if (u < 1 || u > n || v < 1 || v > n) continue;
        // 添加双向边
        ++m; to[m] = v; next[m] = head[u]; head[u] = m;
        ++m; to[m] = u; next[m] = head[v]; head[v] = m;
        if (m >= maxE - 2) { /* 防止越界 */ }
    }

    int a, b;
    if (scanf("%d %d", &a, &b) != 2) return 0;

    // 迭代式 DFS 所需数组
    int *visited = (int*)calloc(n + 1, sizeof(int));
    int *tin     = (int*)calloc(n + 1, sizeof(int));
    int *low     = (int*)calloc(n + 1, sizeof(int));
    int *parent  = (int*)calloc(n + 1, sizeof(int));
    int *curEdge = (int*)malloc((n + 1) * sizeof(int));
    if (!visited || !tin || !low || !parent || !curEdge) return 0;

    for (int i = 1; i <= n; ++i) {
        parent[i] = 0;
        curEdge[i] = head[i];
    }

    // 栈用于模拟递归
    StackItem *stack = (StackItem*)malloc((n + 5) * sizeof(StackItem));
    int top = 0;
    int timer = 0;

    // 以 a 为根进行一次 DFS（图保证连通，路径存在）
    parent[a] = 0;
    stack[top++].u = a;

    while (top > 0) {
        int x = stack[top - 1].u;
        if (!visited[x]) {
            visited[x] = 1;
            tin[x] = low[x] = ++timer;
        }
        int e = curEdge[x];
        if (e) {
            curEdge[x] = next[e];
            int y = to[e];
            if (!visited[y]) {
                parent[y] = x;
                stack[top++].u = y;
            } else if (y != parent[x]) {
                if (tin[y] < low[x]) low[x] = tin[y];
            }
        } else {
            // 回溯
            top--;
            if (parent[x] != 0) {
                int p = parent[x];
                if (low[x] < low[p]) low[p] = low[x];
            }
        }
    }

    // 从 b 沿父指针回到 a，检查路径上的割点条件
    int ans = -1;
    int curr = b;
    // 保护：若 b 未被访问（意外不连通），则无解
    if (!visited[a] || !visited[b]) {
        printf("No solution\n");
        // 释放资源
        free(head); free(to); free(next);
        free(visited); free(tin); free(low); free(parent); free(curEdge); free(stack);
        return 0;
    }

    while (curr != a) {
        int p = parent[curr];
        if (p == 0) break; // 理论上不应发生（连通、从 a 开始 DFS）
        // p 在 a-b 路径上，curr 是通往 b 的孩子
        if (p != a && p != b) {
            if (low[curr] >= tin[p]) {
                if (ans == -1 || p < ans) ans = p;
            }
        }
        curr = p;
    }

    if (ans == -1) {
        printf("No solution\n");
    } else {
        printf("%d\n", ans);
    }

    // 释放资源
    free(head); free(to); free(next);
    free(visited); free(tin); free(low); free(parent); free(curEdge); free(stack);
    return 0;
}