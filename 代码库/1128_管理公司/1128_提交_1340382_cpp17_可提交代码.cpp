#include <bits/stdc++.h>
using namespace std;

struct Dinic {
    struct Edge {
        int to, rev;
        double cap;
    };
    int N;
    vector<vector<Edge>> G;
    vector<int> level, it;

    Dinic(int n=0) { init(n); }
    void init(int n) {
        N = n;
        G.assign(N, {});
        level.assign(N, 0);
        it.assign(N, 0);
    }

    void addEdge(int fr, int to, double cap) {
        Edge a{to, (int)G[to].size(), cap};
        Edge b{fr, (int)G[fr].size(), 0.0};
        G[fr].push_back(a);
        G[to].push_back(b);
    }

    bool bfs(int s, int t) {
        fill(level.begin(), level.end(), -1);
        queue<int> q;
        level[s] = 0;
        q.push(s);
        const double EPS = 1e-12;
        while (!q.empty()) {
            int v = q.front(); q.pop();
            for (auto &e : G[v]) {
                if (level[e.to] < 0 && e.cap > EPS) {
                    level[e.to] = level[v] + 1;
                    q.push(e.to);
                }
            }
        }
        return level[t] >= 0;
    }

    double dfs(int v, int t, double f) {
        if (v == t) return f;
        const double EPS = 1e-12;
        for (int &i = it[v]; i < (int)G[v].size(); i++) {
            Edge &e = G[v][i];
            if (e.cap <= EPS) continue;
            if (level[e.to] != level[v] + 1) continue;
            double ret = dfs(e.to, t, min(f, e.cap));
            if (ret > EPS) {
                e.cap -= ret;
                G[e.to][e.rev].cap += ret;
                return ret;
            }
        }
        return 0.0;
    }

    double maxflow(int s, int t) {
        double flow = 0.0;
        const double INF = 1e100;
        const double EPS = 1e-12;
        while (bfs(s, t)) {
            fill(it.begin(), it.end(), 0);
            while (true) {
                double f = dfs(s, t, INF);
                if (f <= EPS) break;
                flow += f;
            }
        }
        return flow;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;
    vector<pair<int,int>> edges;
    edges.reserve(m);
    for (int i = 0; i < m; i++) {
        int u, v;
        cin >> u >> v;
        edges.push_back({u, v});
    }

    auto ok = [&](double g) -> bool {
        // nodes: s=0, t=1
        // vertex nodes: [2, 2+n-1]
        // edge nodes:   [2+n, 2+n+m-1]
        int s = 0, t = 1;
        int vBase = 2;
        int eBase = 2 + n;
        int N = 2 + n + m;

        Dinic dinic(N);
        const double INF = 1e9;

        for (int i = 0; i < n; i++) {
            int vNode = vBase + i;
            dinic.addEdge(vNode, t, g);
        }
        for (int i = 0; i < m; i++) {
            int eNode = eBase + i;
            dinic.addEdge(s, eNode, 1.0);
            int u = edges[i].first, v = edges[i].second;
            dinic.addEdge(eNode, vBase + (u - 1), INF);
            dinic.addEdge(eNode, vBase + (v - 1), INF);
        }

        double cut = dinic.maxflow(s, t); // mincut value
        // mincut = m + min_S (g|S| - |E(S)|). 若存在 S 使 |E(S)|/|S| > g，则 mincut < m
        return cut + 1e-7 < (double)m;
    };

    double lo = 0.0, hi = (double)m; // 上界用 m 足够
    for (int it = 0; it < 70; it++) {
        double mid = (lo + hi) / 2.0;
        if (ok(mid)) lo = mid;
        else hi = mid;
    }

    cout.setf(std::ios::fixed);
    cout << setprecision(6) << lo << "\n";
    return 0;
}