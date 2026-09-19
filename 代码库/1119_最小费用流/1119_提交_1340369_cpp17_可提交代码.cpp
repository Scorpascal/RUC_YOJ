#include <bits/stdc++.h>
using namespace std;

struct MinCostMaxFlow {
    struct Edge {
        int to, rev;
        long long cap;
        int cost;
    };

    int n;
    vector<vector<Edge>> g;
    vector<long long> dist, pot;
    vector<int> pv_v, pv_e;

    explicit MinCostMaxFlow(int n_) : n(n_), g(n_ + 1), dist(n_ + 1), pot(n_ + 1, 0), pv_v(n_ + 1), pv_e(n_ + 1) {}

    void addEdge(int u, int v, long long cap, int cost) {
        Edge a{v, (int)g[v].size(), cap, cost};
        Edge b{u, (int)g[u].size(), 0, -cost};
        g[u].push_back(a);
        g[v].push_back(b);
    }

    pair<long long, long long> run(int s, int t) {
        const long long INF = (1LL << 60);
        long long flow = 0, cost = 0;

        // costs are non-negative in input; initial pot=0 is ok.
        while (true) {
            fill(dist.begin(), dist.end(), INF);
            dist[s] = 0;

            priority_queue<pair<long long,int>, vector<pair<long long,int>>, greater<pair<long long,int>>> pq;
            pq.push({0, s});

            while (!pq.empty()) {
                auto [d, v] = pq.top();
                pq.pop();
                if (d != dist[v]) continue;

                for (int i = 0; i < (int)g[v].size(); ++i) {
                    const Edge &e = g[v][i];
                    if (e.cap <= 0) continue;

                    // reduced cost: c'(v->to) = cost + pot[v] - pot[to]
                    long long nd = d + (long long)e.cost + pot[v] - pot[e.to];
                    if (nd < dist[e.to]) {
                        dist[e.to] = nd;
                        pv_v[e.to] = v;
                        pv_e[e.to] = i;
                        pq.push({nd, e.to});
                    }
                }
            }

            if (dist[t] == INF) break; // no more augmenting path

            // shortest path cost in original costs (using old potentials)
            long long pathCost = dist[t] + pot[t] - pot[s];

            // update potentials
            for (int v = 1; v <= n; ++v) {
                if (dist[v] < INF) pot[v] += dist[v];
            }

            // find bottleneck
            long long aug = LLONG_MAX;
            for (int v = t; v != s; v = pv_v[v]) {
                const Edge &e = g[pv_v[v]][pv_e[v]];
                aug = min(aug, e.cap);
            }

            // apply augmentation
            for (int v = t; v != s; v = pv_v[v]) {
                int u = pv_v[v];
                int ei = pv_e[v];
                Edge &e = g[u][ei];
                Edge &r = g[e.to][e.rev];
                e.cap -= aug;
                r.cap += aug;
            }

            flow += aug;
            cost += aug * pathCost;
        }

        return {flow, cost};
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    MinCostMaxFlow mcmf(n);
    for (int i = 0; i < m; ++i) {
        int s, t, c, w;
        cin >> s >> t >> c >> w;
        mcmf.addEdge(s, t, (long long)c, w);
    }

    auto [maxflow, mincost] = mcmf.run(1, n);
    cout << maxflow << ' ' << mincost << "\n";
    return 0;
}