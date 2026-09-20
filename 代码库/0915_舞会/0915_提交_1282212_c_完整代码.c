#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 1005
#define MAXM 2005

typedef struct Node {
    int v;
    struct Node* next;
} Node;

Node* adj[MAXN]; // 无向图邻接表
int color[MAXN]; // -1 未染色, 0 左部, 1 右部
int n, m;

void add_edge(int u, int v) {
    Node* a = (Node*)malloc(sizeof(Node));
    a->v = v; a->next = adj[u]; adj[u] = a;
    Node* b = (Node*)malloc(sizeof(Node));
    b->v = u; b->next = adj[v]; adj[v] = b;
}

int queue[MAXN];

int bfs_color(int s) {
    int head = 0, tail = 0;
    color[s] = 0;
    queue[tail++] = s;
    while (head < tail) {
        int u = queue[head++];
        for (Node* p = adj[u]; p; p = p->next) {
            int v = p->v;
            if (color[v] == -1) {
                color[v] = color[u] ^ 1;
                queue[tail++] = v;
            } else if (color[v] == color[u]) {
                // 若出现同色相连，说明不是二分图（题意不应发生）
                return 0;
            }
        }
    }
    return 1;
}

/* Hopcroft-Karp */
int pairU[MAXN], pairV[MAXN], distHK[MAXN];
int leftVertices[MAXN], leftCnt = 0;

int bfsHK(void) {
    int head = 0, tail = 0;
    for (int i = 0; i < leftCnt; ++i) {
        int u = leftVertices[i];
        if (pairU[u] == -1) {
            distHK[u] = 0;
            queue[tail++] = u;
        } else {
            distHK[u] = -1;
        }
    }
    int foundFree = 0;
    while (head < tail) {
        int u = queue[head++];
        for (Node* p = adj[u]; p; p = p->next) {
            int v = p->v;
            if (color[u] != 0) continue; // 仅从左部扩展
            if (pairV[v] != -1) {
                int u2 = pairV[v];
                if (distHK[u2] == -1) {
                    distHK[u2] = distHK[u] + 1;
                    queue[tail++] = u2;
                }
            } else {
                foundFree = 1;
            }
        }
    }
    return foundFree;
}

int dfsHK(int u) {
    for (Node* p = adj[u]; p; p = p->next) {
        int v = p->v;
        if (color[u] != 0) continue;
        int u2 = pairV[v];
        if (pairV[v] == -1 || (u2 != -1 && distHK[u2] == distHK[u] + 1 && dfsHK(u2))) {
            pairU[u] = v;
            pairV[v] = u;
            return 1;
        }
    }
    distHK[u] = -1;
    return 0;
}

int hopcroft_karp(void) {
    // 初始化匹配
    for (int i = 0; i < n; ++i) { pairU[i] = -1; pairV[i] = -1; }
    int matching = 0;
    while (bfsHK()) {
        for (int i = 0; i < leftCnt; ++i) {
            int u = leftVertices[i];
            if (pairU[u] == -1) {
                if (dfsHK(u)) matching++;
            }
        }
    }
    return matching;
}

int main(void) {
    if (scanf("%d %d", &n, &m) != 2) return 0;
    for (int i = 0; i < n; ++i) { adj[i] = NULL; color[i] = -1; }
    for (int i = 0; i < m; ++i) {
        int u, v;
        scanf("%d %d", &u, &v);
        if (u < 0 || v < 0 || u >= n || v >= n) continue;
        add_edge(u, v);
    }
    // 二染色，确定左右部
    for (int i = 0; i < n; ++i) {
        if (color[i] == -1) {
            if (!bfs_color(i)) {
                // 非二分图（按题意不应发生），退化输出：最大独立集未知，这里保守输出 n
                printf("%d\n", n);
                return 0;
            }
        }
    }
    leftCnt = 0;
    for (int i = 0; i < n; ++i) {
        if (color[i] == 0) leftVertices[leftCnt++] = i;
    }
    int maxMatching = hopcroft_karp();
    int maxIndependentSet = n - maxMatching;
    printf("%d\n", maxIndependentSet);
    return 0;
}