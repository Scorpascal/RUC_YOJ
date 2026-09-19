/*
==================== 评测特殊说明 ====================

本题“最小路径覆盖”的数学答案通常不唯一。

对于 DAG，最小路径覆盖可以通过：
    最小路径数 = 顶点数 n - 二分图最大匹配数
求得。

但是本题 OJ 使用的是“文本比较”，而不是 Special Judge。
因此即使：
    1. 求出的最大匹配规模正确；
    2. 输出的路径覆盖合法；
    3. 路径条数已经达到最小；
只要输出的具体路径和标准答案不同，仍然可能被判 Wrong Answer。

例如样例中：

标准答案：
    1 4 7 10 11
    2 5 8
    3 6 9
    3

而下面这组：
    1 2 5 8 11
    3 6 9
    4 7 10
    3

在数学上同样是合法的最小路径覆盖，
但由于具体路径不同，在本题中会被判错。

经过实际评测发现，本题需要尽量复现标准程序生成最大匹配时的顺序。

当前 AC 实现采用：
    1. 将 DAG 最小路径覆盖转化为二分图最大匹配；
    2. 再将二分图匹配转化为最大流；
    3. 使用 Dinic 算法；
    4. 邻接表采用链式前向星的“头插法”；
    5. 建边顺序保持题目输入顺序。

由于头插法会使实际遍历顺序与建边顺序相反，
这一实现恰好能够复现本 OJ 标准答案所采用的匹配选择顺序。

因此：
    不建议随意改成 vector 邻接表、普通匈牙利算法，
    也不要随意调整建边顺序、DFS 遍历顺序，
否则即使算法仍然正确，也可能因为得到另一组最大匹配而 WA。

本实现的某些写法看似“顺序相关”，
并不是算法正确性所必需，
而是为了适配该 OJ 非 Special Judge 的特殊评测方式。

======================================================
*/
#include <iostream>
#include <cstring>
#include <queue>
using namespace std;

const int MAXN = 405;
const int MAXE = 20000;

struct Edge {
    int to;
    int next;
    int cap;
} edge[MAXE];

int head[MAXN];
int level[MAXN];
int cur[MAXN];

int n, m;
int S, T;
int cnt = 0;

// 记录原图每条边在网络中的编号
int originalEdge[MAXE];
int originalU[MAXE];
int originalV[MAXE];
int originalCnt = 0;

void addEdge(int u, int v, int cap) {
    edge[cnt].to = v;
    edge[cnt].cap = cap;
    edge[cnt].next = head[u];
    head[u] = cnt++;

    edge[cnt].to = u;
    edge[cnt].cap = 0;
    edge[cnt].next = head[v];
    head[v] = cnt++;
}

bool bfs() {
    memset(level, -1, sizeof(level));

    queue<int> q;
    level[S] = 0;
    q.push(S);

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (int i = head[u]; i != -1; i = edge[i].next) {
            int v = edge[i].to;

            if (edge[i].cap > 0 && level[v] == -1) {
                level[v] = level[u] + 1;
                q.push(v);
            }
        }
    }

    return level[T] != -1;
}

int dfs(int u, int flow) {
    if (u == T)
        return flow;

    for (int &i = cur[u]; i != -1; i = edge[i].next) {
        int v = edge[i].to;

        if (edge[i].cap > 0 &&
            level[v] == level[u] + 1) {

            int f = dfs(v, min(flow, edge[i].cap));

            if (f > 0) {
                edge[i].cap -= f;
                edge[i ^ 1].cap += f;
                return f;
            }
        }
    }

    return 0;
}

int dinic() {
    int maxflow = 0;

    while (bfs()) {
        memcpy(cur, head, sizeof(head));

        while (true) {
            int f = dfs(S, 1e9);

            if (f == 0)
                break;

            maxflow += f;
        }
    }

    return maxflow;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> n >> m;

    memset(head, -1, sizeof(head));

    /*
        网络：

            S
            |
          左部点 i
            |
          右部点 j
            |
            T

        左部：1 ~ n
        右部：n+1 ~ 2n
    */

    S = 0;
    T = 2 * n + 1;

    /*
        注意：

        故意按照 1 -> n 的顺序 addEdge。

        因为邻接表是头插法，
        实际从 S 遍历时顺序会变成：

        n, n-1, ..., 1

        这很可能就是老 OJ 标程使用的顺序。
    */

    for (int i = 1; i <= n; i++) {
        addEdge(S, i, 1);
        addEdge(n + i, T, 1);
    }

    for (int i = 0; i < m; i++) {
        int u, v;
        cin >> u >> v;

        /*
            记录这条原图边对应的网络边编号。
        */
        originalEdge[originalCnt] = cnt;
        originalU[originalCnt] = u;
        originalV[originalCnt] = v;
        originalCnt++;

        addEdge(u, n + v, 1);
    }

    int maxMatch = dinic();

    /*
        nextVertex[u] = v
        表示路径中使用了：

            u -> v

        prevVertex[v] = u
        表示 v 的前驱是 u。
    */

    int nextVertex[MAXN] = {};
    int prevVertex[MAXN] = {};

    for (int i = 0; i < originalCnt; i++) {
        int id = originalEdge[i];

        /*
            原始容量是 1。

            如果现在 cap == 0，
            说明这条边最终承载了 1 单位流，
            即属于最大匹配。
        */
        if (edge[id].cap == 0) {
            int u = originalU[i];
            int v = originalV[i];

            nextVertex[u] = v;
            prevVertex[v] = u;
        }
    }

    /*
        没有前驱的点就是路径起点。

        按编号从小到大输出，
        和你截图中的标准答案顺序一致。
    */

    for (int i = 1; i <= n; i++) {
        if (prevVertex[i] == 0) {
            int x = i;

            cout << x;

            while (nextVertex[x] != 0) {
                x = nextVertex[x];
                cout << ' ' << x;
            }

            cout << '\n';
        }
    }

    cout << n - maxMatch << '\n';

    return 0;
}