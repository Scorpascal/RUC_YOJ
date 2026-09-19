#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    vector<int> a(n + 1);
    vector<int> pos(n);

    for (int i = 1; i <= n; ++i) {
        cin >> a[i];
        pos[a[i]] = i;
    }

    vector<vector<int>> g(n + 1);

    for (int i = 1; i < n; ++i) {
        int u, v;
        cin >> u >> v;

        g[u].push_back(v);
        g[v].push_back(u);
    }

    // ------------------------------------------------------------
    // 1. 非递归 DFS
    //    计算 parent / depth / tin / tout
    // ------------------------------------------------------------

    vector<int> parent(n + 1);
    vector<int> depth(n + 1);
    vector<int> tin(n + 1);
    vector<int> tout(n + 1);

    struct Event {
        int u;
        int p;
        bool exit;
    };

    vector<Event> st;
    st.reserve(2 * n);

    st.push_back({1, 1, false});

    int timer = 0;

    while (!st.empty()) {
        auto [u, p, exit] = st.back();
        st.pop_back();

        if (!exit) {
            parent[u] = p;

            if (u == p)
                depth[u] = 0;
            else
                depth[u] = depth[p] + 1;

            tin[u] = ++timer;

            // 退出事件先压栈，
            // 保证所有儿子处理完成后才执行
            st.push_back({u, p, true});

            for (int i = (int)g[u].size() - 1; i >= 0; --i) {
                int v = g[u][i];

                if (v == p)
                    continue;

                st.push_back({v, u, false});
            }
        } else {
            // 当前 timer 就是该子树 DFS 序最大编号
            tout[u] = timer;
        }
    }

    // ------------------------------------------------------------
    // 2. Binary Lifting LCA
    // ------------------------------------------------------------

    int LOG = 1;
    while ((1 << LOG) <= n)
        ++LOG;

    vector<vector<int>> up(LOG, vector<int>(n + 1));

    for (int i = 1; i <= n; ++i)
        up[0][i] = parent[i];

    for (int j = 1; j < LOG; ++j) {
        for (int i = 1; i <= n; ++i) {
            up[j][i] = up[j - 1][up[j - 1][i]];
        }
    }

    auto isAncestor = [&](int u, int v) -> bool {
        return tin[u] <= tin[v] && tin[v] <= tout[u];
    };

    auto lca = [&](int u, int v) -> int {
        if (isAncestor(u, v))
            return u;

        if (isAncestor(v, u))
            return v;

        for (int j = LOG - 1; j >= 0; --j) {
            if (!isAncestor(up[j][u], v))
                u = up[j][u];
        }

        return up[0][u];
    };

    auto dist = [&](int u, int v) -> int {
        int w = lca(u, v);

        return depth[u] + depth[v] - 2 * depth[w];
    };

    // ------------------------------------------------------------
    // 3. 预处理每个合法前缀对应路径的两个端点
    //
    // leftEnd[k], rightEnd[k]:
    // 权值 0..k-1 对应节点全部位于
    // leftEnd[k] -> rightEnd[k] 上
    // ------------------------------------------------------------

    vector<int> leftEnd(n + 1);
    vector<int> rightEnd(n + 1);

    int K = 1;

    int s = pos[0];
    int t = pos[0];

    leftEnd[1] = s;
    rightEnd[1] = t;

    int diameter = 0;

    for (int len = 2; len <= n; ++len) {
        int w = pos[len - 1];

        int dsw = dist(s, w);
        int dtw = dist(t, w);
        int dst = diameter;

        /*
         * 三点共线的三种情况：
         *
         * 1. w 在 s-t 之间
         *    d(s,w) + d(w,t) = d(s,t)
         *
         * 2. t 在 s-w 之间
         *    d(s,t) + d(t,w) = d(s,w)
         *
         * 3. s 在 t-w 之间
         *    d(s,t) + d(s,w) = d(t,w)
         */

        if (dsw + dtw == dst) {
            // w 在当前路径内部
            // 端点不变
        }
        else if (dst + dtw == dsw) {
            // t 在 s-w 之间
            t = w;
            diameter = dsw;
        }
        else if (dst + dsw == dtw) {
            // s 在 t-w 之间
            s = w;
            diameter = dtw;
        }
        else {
            // 三个方向发生分叉
            // 当前前缀不可能被任何简单路径完整包含
            break;
        }

        K = len;

        leftEnd[len] = s;
        rightEnd[len] = t;
    }

    // ------------------------------------------------------------
    // 4. 回答询问
    // ------------------------------------------------------------

    while (m--) {
        int x, y;
        cin >> x >> y;

        int r = lca(x, y);

        // 判断 z 是否位于 x-y 路径上
        //
        // z 必须在 r 的子树中，
        // 同时 z 必须是 x 或 y 的祖先。
        auto onQueryPath = [&](int z) -> bool {
            return isAncestor(r, z) &&
                   (isAncestor(z, x) || isAncestor(z, y));
        };

        auto check = [&](int k) -> bool {
            if (k == 0)
                return true;

            return onQueryPath(leftEnd[k]) &&
                   onQueryPath(rightEnd[k]);
        };

        // 最大化合法的 k
        int lo = 0;
        int hi = K;

        while (lo < hi) {
            int mid = (lo + hi + 1) >> 1;

            if (check(mid))
                lo = mid;
            else
                hi = mid - 1;
        }

        cout << lo << '\n';
    }

    return 0;
}