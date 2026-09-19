#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if (!(cin >> n)) return 0;

    vector<vector<int>> adj(n + 1);
    for (int i = 0; i < n - 1; i++) {
        int x, y;
        cin >> x >> y;
        adj[x].push_back(y);
        adj[y].push_back(x);
    }

    // 迭代 DFS：求 parent 与遍历顺序 order（用于后序计算子树大小）
    vector<int> parent(n + 1, 0);
    vector<int> order;
    order.reserve(n);

    int root = 1;
    parent[root] = -1;
    stack<int> st;
    st.push(root);

    while (!st.empty()) {
        int v = st.top();
        st.pop();
        order.push_back(v);
        for (int u : adj[v]) {
            if (u == parent[v]) continue;
            if (parent[u] != 0) continue; // 已访问
            parent[u] = v;
            st.push(u);
        }
    }

    vector<long long> sz(n + 1, 1);
    long long pairSum = 0; // sum_{u<v} dist(u,v)

    // 后序：先处理子节点，再回到父节点累加子树大小
    for (int i = (int)order.size() - 1; i >= 0; --i) {
        int v = order[i];
        for (int u : adj[v]) {
            if (u == parent[v]) continue;
            sz[v] += sz[u];
        }
        if (v != root) {
            long long s = sz[v];
            pairSum += s * (n - s); // 边(parent[v], v) 的贡献
        }
    }

    long long ans = 2LL * pairSum; // 有序点对之和 = 2 * 无序点对之和
    cout << ans << "\n";
    return 0;
}