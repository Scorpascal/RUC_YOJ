#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 500000
#define MAXM 500000

// 邻接表
typedef struct Edge { int to, next; } Edge;
static Edge edges[MAXM + 5];
static int head[MAXN + 5], ecnt;

// 读入
static inline int readInt() {
    int c = getchar_unlocked(), x = 0, s = 1;
    while (c != '-' && (c < '0' || c > '9')) c = getchar_unlocked();
    if (c == '-') { s = -1; c = getchar_unlocked(); }
    while (c >= '0' && c <= '9') { x = x*10 + (c - '0'); c = getchar_unlocked(); }
    return x * s;
}

// Tarjan
static int dfn[MAXN + 5], low[MAXN + 5], idx;
static int stk[MAXN + 5], top;
static char inStk[MAXN + 5];
static int sccId[MAXN + 5], sccCnt;
static long long weight[MAXN + 5];      // 原图节点点权（现金）
static long long sccWeight[MAXN + 5];   // SCC 合并点权

void addEdge(int u, int v) {
    edges[++ecnt].to = v;
    edges[ecnt].next = head[u];
    head[u] = ecnt;
}

void tarjan(int u) {
    dfn[u] = low[u] = ++idx;
    stk[++top] = u; inStk[u] = 1;
    for (int i = head[u]; i; i = edges[i].next) {
        int v = edges[i].to;
        if (!dfn[v]) {
            tarjan(v);
            if (low[v] < low[u]) low[u] = low[v];
        } else if (inStk[v]) {
            if (dfn[v] < low[u]) low[u] = dfn[v];
        }
    }
    if (low[u] == dfn[u]) {
        ++sccCnt;
        while (1) {
            int x = stk[top--];
            inStk[x] = 0;
            sccId[x] = sccCnt;
            sccWeight[sccCnt] += weight[x];
            if (x == u) break;
        }
    }
}

// 缩点后的 DAG
static int chead[MAXN + 5], cecnt;
typedef struct E2 { int to, next; } E2;
static E2 cedges[MAXM + 5];

// 去重需要，用哈希或标记。这里用简易“访问戳”去重：visEdge[u_scc * key + v_scc]不现实。
// 改为：对每个 u_scc，用一个“最后访问的 v_scc”桶，仍可能 O(M)。
// 更稳妥：暂存边对，排序去重（但 M=5e5，排序可）。为简洁，这里用链上探测去重：
// 我们用一个小的局部标记数组 per u_scc，在构建时清空。因为每个 u_scc 的出边数量总可接受。

// 为每个 u_scc 用一个哈希位标记：使用一个小的位图不现实。改用“构建时集合”——我们先收集所有边对到数组，然后排序去重再建图。

typedef struct Pair { int u, v; } Pair;
static Pair pairs[MAXM + 5];
static int pcnt;

int cmpPair(const void* a, const void* b) {
    const Pair* x = (const Pair*)a;
    const Pair* y = (const Pair*)b;
    if (x->u != y->u) return x->u < y->u ? -1 : 1;
    if (x->v != y->v) return x->v < y->v ? -1 : 1;
    return 0;
}

void addCEdge(int u, int v) {
    cedges[++cecnt].to = v;
    cedges[cecnt].next = chead[u];
    chead[u] = cecnt;
}

// DAG 上 DP（正向，源到各点的最大点权和）
static long long dp[MAXN + 5];
static int indeg[MAXN + 5];
static char reachable[MAXN + 5];

int main() {
    int N = readInt(), M = readInt();
    // 读边
    for (int i = 0; i < M; ++i) {
        int u = readInt(), v = readInt();
        addEdge(u, v);
    }
    // 读点权
    for (int i = 1; i <= N; ++i) {
        weight[i] = readInt();
    }
    int S = readInt(), P = readInt();
    static int bars[MAXN + 5];
    for (int i = 0; i < P; ++i) bars[i] = readInt();

    // Tarjan
    for (int i = 1; i <= N; ++i) {
        if (!dfn[i]) tarjan(i);
    }

    // 缩点边收集
    pcnt = 0;
    for (int u = 1; u <= N; ++u) {
        int su = sccId[u];
        for (int i = head[u]; i; i = edges[i].next) {
            int v = edges[i].to;
            int sv = sccId[v];
            if (su != sv) {
                pairs[++pcnt].u = su;
                pairs[pcnt].v = sv;
            }
        }
    }
    // 去重并统计入度
    if (pcnt > 0) {
        qsort(pairs + 1, pcnt, sizeof(Pair), cmpPair);
        int lastu = -1, lastv = -1;
        for (int i = 1; i <= pcnt; ++i) {
            if (pairs[i].u != lastu || pairs[i].v != lastv) {
                addCEdge(pairs[i].u, pairs[i].v);
                indeg[pairs[i].v]++;       // 统计缩点图入度
                lastu = pairs[i].u; lastv = pairs[i].v;
            }
        }
    }

    // 以 S 所在 SCC 为源做正向 DP（只在可达子图内）
    int sSrc = sccId[S];

    // 先用 BFS 在缩点图上标记从源可达的 SCC，避免把不可达节点纳入 DP
    static int q[MAXN + 5];
    int qh = 0, qt = 0;
    q[qt++] = sSrc;
    reachable[sSrc] = 1;
    while (qh < qt) {
        int u = q[qh++];
        for (int i = chead[u]; i; i = cedges[i].next) {
            int v = cedges[i].to;
            if (!reachable[v]) {
                reachable[v] = 1;
                q[qt++] = v;
            }
        }
    }

    // Kahn 拓扑（仅对可达节点进行），初始化队列时将可达节点的入度按可达边重算
    // 为避免重建入度，简单做：将不可达节点视为不入队；并使用一个局部入度数组只计可达内的入度
    static int indegReach[MAXN + 5];
    for (int u = 1; u <= sccCnt; ++u) {
        if (!reachable[u]) continue;
        for (int i = chead[u]; i; i = cedges[i].next) {
            int v = cedges[i].to;
            if (reachable[v]) indegReach[v]++;
        }
    }

    // 初始化 dp
    for (int i = 1; i <= sccCnt; ++i) dp[i] = 0;
    dp[sSrc] = sccWeight[sSrc];

    // Kahn 队列
    qh = qt = 0;
    for (int u = 1; u <= sccCnt; ++u) {
        if (reachable[u] && indegReach[u] == 0) q[qt++] = u;
    }
    while (qh < qt) {
        int u = q[qh++];
        // 仅当从源可达且已经有 dp（可能 u 不是源但能通过前驱更新）
        for (int i = chead[u]; i; i = cedges[i].next) {
            int v = cedges[i].to;
            if (!reachable[v]) continue;
            if (dp[u] > 0) {
                long long cand = dp[u] + sccWeight[v];
                if (cand > dp[v]) dp[v] = cand;
            }
            if (--indegReach[v] == 0) q[qt++] = v;
        }
    }

    long long ans = 0;
    for (int i = 0; i < P; ++i) {
        int sb = sccId[bars[i]];
        if (reachable[sb] && dp[sb] > ans) ans = dp[sb];
    }
    printf("%lld\n", ans);
    return 0;
}