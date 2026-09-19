#include <bits/stdc++.h>
using namespace std;

using ll = long long;
static const ll INF = (ll)4e18;

struct Edge {
    int u, v;
    ll w;
};

static vector<ll> dijkstra(int n, int src, const vector<vector<pair<int,ll>>>& g) {
    vector<ll> dist(n + 1, INF);
    priority_queue<pair<ll,int>, vector<pair<ll,int>>, greater<pair<ll,int>>> pq;
    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {
        auto [d, x] = pq.top();
        pq.pop();
        if (d != dist[x]) continue;
        for (auto [y, w] : g[x]) {
            ll nd = d + w;
            if (nd < dist[y]) {
                dist[y] = nd;
                pq.push({nd, y});
            }
        }
    }
    return dist;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    vector<vector<pair<int,ll>>> g(n + 1);
    vector<Edge> edges;
    edges.reserve(m);

    for (int i = 0; i < m; i++) {
        int x, y;
        ll z;
        cin >> x >> y >> z;
        edges.push_back({x, y, z});
        g[x].push_back({y, z});
        g[y].push_back({x, z}); // 无向图；自环也没问题（会加两次但不影响正确性）
    }

    auto dist1 = dijkstra(n, 1, g);
    auto distn = dijkstra(n, n, g);

    ll ans = dist1[n]; // 不做任何修改
    for (const auto& e : edges) {
        // 把这条边权改为0
        if (dist1[e.u] < INF && distn[e.v] < INF) ans = min(ans, dist1[e.u] + distn[e.v]);
        if (dist1[e.v] < INF && distn[e.u] < INF) ans = min(ans, dist1[e.v] + distn[e.u]);
    }

    cout << ans << "\n";
    return 0;
}