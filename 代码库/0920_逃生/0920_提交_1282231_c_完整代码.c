// 逃生：Tarjan 求双连通分量，计算最少出口数与方案数
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAXN 20005     // 安全上限：题中 N<=500，但节点编号不明，动态扩容亦可
#define MAXE 100005

typedef struct { int u, v; } Edge;

typedef struct Node {
    int to;
    struct Node* next;
} Node;

static Node* adj[MAXN];
static int dfn[MAXN], low[MAXN], parent[MAXN], isArt[MAXN];
static int idxTime;
static Edge estack[MAXE];
static int estop;

typedef struct {
    int *verts;
    int vcnt;
} BCC;

static BCC bccs[ MAXE ];
static int bccCnt;

static int maxNode; // 最大节点编号（1..maxNode 都出现）

static void add_edge(int u, int v) {
    Node* a = (Node*)malloc(sizeof(Node));
    a->to = v; a->next = adj[u]; adj[u] = a;
    Node* b = (Node*)malloc(sizeof(Node));
    b->to = u; b->next = adj[v]; adj[v] = b;
}

static void push_edge(int u, int v) {
    estack[estop].u = u; estack[estop].v = v; estop++;
}

static void pop_component(int u, int v) {
    // 弹出直到 (u,v)，收集顶点集合
    // 由于可能重复顶点，使用标记数组临时去重
    static int mark[MAXN];
    int usedCount = 0;
    int start = estop - 1;
    for (;;) {
        Edge e = estack[start];
        if (!mark[e.u]) { mark[e.u] = 1; usedCount++; }
        if (!mark[e.v]) { mark[e.v] = 1; usedCount++; }
        if (e.u == u && e.v == v) break;
        start--;
    }
    // 复制顶点集合
    bccs[bccCnt].verts = (int*)malloc(sizeof(int) * usedCount);
    bccs[bccCnt].vcnt = usedCount;
    int k = 0;
    for (int i = 1; i <= maxNode; i++) {
        if (mark[i]) {
            bccs[bccCnt].verts[k++] = i;
            mark[i] = 0; // 清理
        }
    }
    // 真正弹栈
    while (1) {
        Edge e = estack[--estop];
        if (e.u == u && e.v == v) break;
    }
    bccCnt++;
}

static void tarjan(int u) {
    dfn[u] = low[u] = ++idxTime;
    int child = 0;
    for (Node* p = adj[u]; p; p = p->next) {
        int v = p->to;
        if (!dfn[v]) {
            parent[v] = u;
            push_edge(u, v);
            child++;
            tarjan(v);
            if (low[v] >= dfn[u]) {
                // u 是割点（除根需特殊处理）
                if (parent[u] != 0) isArt[u] = 1;
                pop_component(u, v);
            }
            if (low[v] < low[u]) low[u] = low[v];
        } else if (v != parent[u] && dfn[v] < dfn[u]) {
            // 后向边（仅入栈一次）
            push_edge(u, v);
            if (dfn[v] < low[u]) low[u] = dfn[v];
        }
    }
    // 根特判：根有两棵或以上子树则是割点
    if (parent[u] == 0 && child >= 2) isArt[u] = 1;
}

int main() {
    int N;
    int caseId = 1;
    while (scanf("%d", &N) == 1) {
        if (N == 0) break;

        // 清理
        for (int i = 0; i < MAXN; i++) { adj[i] = NULL; dfn[i]=low[i]=parent[i]=isArt[i]=0; }
        idxTime = 0; estop = 0; bccCnt = 0; maxNode = 0;

        // 读取边
        int u, v;
        for (int i = 0; i < N; i++) {
            if (scanf("%d %d", &u, &v) != 2) return 0;
            if (u >= MAXN || v >= MAXN) return 0; // 简易防护
            add_edge(u, v);
            if (u > maxNode) maxNode = u;
            if (v > maxNode) maxNode = v;
        }

        // 图保证连通，从 1..maxNode 都出现，找到一个起点
        int start = 1;
        // 运行 Tarjan
        tarjan(start);
        // 可能存在边栈残留（理论上无，因为连通且在 >= 触发时都会弹）
        while (estop > 0) {
            Edge e = estack[--estop];
            // 收尾成一个分量
            // 简化：若残留则按同样方式收集（这里通常不会发生）
            // 为稳健省略额外处理
        }

        // 统计：
        // 1) 若只有一个 BCC：最少 2，方案数 nC2
        // 2) 否则：最少 = 叶子分量数；方案数 = 乘积(每个叶子分量中的非割点顶点数)
        uint64_t minExits = 0;
        uint64_t ways = 1;

        if (bccCnt == 1) {
            // 计算节点数 n（题目保证 1..n 都出现）
            int n = maxNode;
            minExits = 2;
            ways = (uint64_t)n * (uint64_t)(n - 1) / 2;
        } else {
            int leafCount = 0;
            for (int i = 0; i < bccCnt; i++) {
                int artIn = 0;
                int nonArt = 0;
                for (int j = 0; j < bccs[i].vcnt; j++) {
                    int x = bccs[i].verts[j];
                    if (isArt[x]) artIn++;
                    else nonArt++;
                }
                // BCC 的度 = 其中的割点数
                if (artIn == 1) {
                    leafCount++;
                    ways *= (uint64_t)nonArt;
                }
            }
            minExits = leafCount;
        }

        printf("Case %d: %llu %llu\n", caseId++, (unsigned long long)minExits, (unsigned long long)ways);

        // 释放邻接表
        for (int i = 1; i <= maxNode; i++) {
            Node* p = adj[i];
            while (p) { Node* q = p->next; free(p); p = q; }
        }
        for (int i = 0; i < bccCnt; i++) {
            free(bccs[i].verts);
            bccs[i].verts = NULL;
        }
    }
    return 0;
}