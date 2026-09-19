#include <bits/stdc++.h>
using namespace std;

// 返回：dist 数组，并通过 farthest 输出最远点
static vector<int> bfs_farthest(int start, const vector<vector<int>>& g, int& farthest) {
    int n = (int)g.size();
    vector<int> dist(n, -1);
    queue<int> q;
    dist[start] = 0;
    q.push(start);

    farthest = start;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        if (dist[u] > dist[farthest]) farthest = u;
        for (int v : g[u]) {
            if (dist[v] == -1) {
                dist[v] = dist[u] + 1;
                q.push(v);
            }
        }
    }
    return dist;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;
    vector<vector<int>> g(n);
    for (int i = 0; i < n - 1; i++) {
        int u, v;
        cin >> u >> v;
        g[u].push_back(v);
        g[v].push_back(u);
    }

    if (n == 0) return 0;
    if (n == 1) {
        cout << 0 << "\n";
        return 0;
    }

    int A, B;
    // 先从 0 找到一个最远点 A
    bfs_farthest(0, g, A);
    // 从 A BFS，得到 distA，并找到最远点 B（A-B 为一条直径）
    vector<int> distA = bfs_farthest(A, g, B);
    int D = distA[B];
    // 从 B BFS，得到 distB
    int tmp;
    vector<int> distB = bfs_farthest(B, g, tmp);

    // 直径端点集合 P：满足 max(distA, distB) == D
    vector<char> isPeripheral(n, 0);
    for (int i = 0; i < n; i++) {
        if (max(distA[i], distB[i]) == D) isPeripheral[i] = 1;
    }

    // 剥离所有“非端点”的叶子，剩下的是连接所有端点的最小子树
    vector<int> deg(n);
    for (int i = 0; i < n; i++) deg[i] = (int)g[i].size();

    queue<int> q;
    vector<char> removed(n, 0);

    for (int i = 0; i < n; i++) {
        if (deg[i] <= 1 && !isPeripheral[i]) {
            q.push(i);
            removed[i] = 1;
        }
    }

    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : g[u]) {
            if (removed[v]) continue;
            deg[v]--;
            if (deg[v] == 1 && !isPeripheral[v]) {
                removed[v] = 1;
                q.push(v);
            }
        }
    }

    // 输出未被删除的点（即位于某条直径上的点），按编号升序
    for (int i = 0; i < n; i++) {
        if (!removed[i]) cout << i << "\n";
    }
    return 0;
}