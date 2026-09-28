#include <algorithm>
#include <iostream>
#include <vector>
using namespace std;

struct PersistentSeg {
    struct Node { int l = 0, r = 0, sum = 0; };
    vector<Node> tr;
    int n;
    PersistentSeg(int n_, int reserveNodes) : n(n_) {
        tr.reserve(reserveNodes);
        tr.push_back(Node());
    }
    int add(int old, int L, int R, int pos) {
        int cur = (int)tr.size();
        tr.push_back(tr[old]);
        tr[cur].sum++;
        if (L != R) {
            int M = (L + R) >> 1;
            if (pos <= M) tr[cur].l = add(tr[old].l, L, M, pos);
            else tr[cur].r = add(tr[old].r, M + 1, R, pos);
        }
        return cur;
    }
    int mex(int ru, int rv, int rw, int rp, int L, int R) const {
        if (L == R) {
            int c = tr[ru].sum + tr[rv].sum - tr[rw].sum - tr[rp].sum;
            return c ? L + 1 : L;
        }
        int M = (L + R) >> 1;
        int lu = tr[ru].l, lv = tr[rv].l, lw = tr[rw].l, lp = tr[rp].l;
        int leftCount = tr[lu].sum + tr[lv].sum - tr[lw].sum - tr[lp].sum;
        if (leftCount < M - L + 1)
            return mex(lu, lv, lw, lp, L, M);
        return mex(tr[ru].r, tr[rv].r, tr[rw].r, tr[rp].r, M + 1, R);
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, m;
    if (!(cin >> n >> m)) return 0;
    vector<int> a(n + 1);
    for (int i = 1; i <= n; ++i) cin >> a[i];
    vector<vector<int>> g(n + 1);
    for (int i = 1, u, v; i < n; ++i) {
        cin >> u >> v;
        g[u].push_back(v);
        g[v].push_back(u);
    }

    int LOG = 1;
    while ((1 << LOG) <= n) ++LOG;
    vector<vector<int>> up(LOG, vector<int>(n + 1));
    vector<int> dep(n + 1), order;
    order.reserve(n);
    up[0][1] = 0;
    vector<int> st{1};
    while (!st.empty()) {
        int u = st.back(); st.pop_back();
        order.push_back(u);
        for (int v : g[u]) if (v != up[0][u]) {
            up[0][v] = u;
            dep[v] = dep[u] + 1;
            st.push_back(v);
        }
    }
    for (int k = 1; k < LOG; ++k)
        for (int v = 1; v <= n; ++v)
            up[k][v] = up[k - 1][up[k - 1][v]];

    auto lca = [&](int u, int v) {
        if (dep[u] < dep[v]) swap(u, v);
        int d = dep[u] - dep[v];
        for (int k = 0; k < LOG; ++k) if ((d >> k) & 1) u = up[k][u];
        if (u == v) return u;
        for (int k = LOG - 1; k >= 0; --k)
            if (up[k][u] != up[k][v]) {
                u = up[k][u];
                v = up[k][v];
            }
        return up[0][u];
    };

    PersistentSeg pst(n, max(1, n * 20 + 5));
    vector<int> root(n + 1, 0);
    for (int u : order) root[u] = pst.add(root[up[0][u]], 0, n - 1, a[u]);

    while (m--) {
        int x, y;
        cin >> x >> y;
        int z = lca(x, y);
        int pz = up[0][z];
        cout << pst.mex(root[x], root[y], root[z], root[pz], 0, n - 1) << '\n';
    }
    return 0;
}