#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
using namespace std;

struct MinCostMaxFlow {
    struct Edge {
        int to, rev;
        long long cap;
        long long cost;
    };

    int N;
    vector<vector<Edge>> G;

    explicit MinCostMaxFlow(int n) : N(n), G(n) {}

    void addEdge(int u, int v, long long cap, long long cost) {
        Edge a{v, (int)G[v].size(), cap, cost};
        Edge b{u, (int)G[u].size(), 0, -cost};
        G[u].push_back(a);
        G[v].push_back(b);
    }

    // 返回 {flow, cost}
    pair<long long, long long> minCostMaxFlow(int s, int t, long long maxf) {
        const long long INF = (1LL << 62);

        long long flow = 0, cost = 0;
        vector<long long> pot(N, 0), dist(N);
        vector<int> pv(N), pe(N);

        // 由于可能存在负边（求最大费用时），先用 SPFA 求初始势能
        {
            fill(dist.begin(), dist.end(), INF);
            deque<int> q;
            vector<bool> inq(N, false);
            dist[s] = 0;
            q.push_back(s);
            inq[s] = true;

            while (!q.empty()) {
                int u = q.front();
                q.pop_front();
                inq[u] = false;
                for (int i = 0; i < (int)G[u].size(); i++) {
                    auto &e = G[u][i];
                    if (e.cap <= 0) continue;
                    if (dist[e.to] > dist[u] + e.cost) {
                        dist[e.to] = dist[u] + e.cost;
                        if (!inq[e.to]) {
                            inq[e.to] = true;
                            // SLF 优化
                            if (!q.empty() && dist[e.to] < dist[q.front()]) q.push_front(e.to);
                            else q.push_back(e.to);
                        }
                    }
                }
            }
            for (int i = 0; i < N; i++) {
                if (dist[i] < INF) pot[i] = dist[i];
            }
        }

        while (flow < maxf) {
            // Dijkstra on reduced costs
            fill(dist.begin(), dist.end(), INF);
            dist[s] = 0;
            pv[s] = -1; pe[s] = -1;

            using P = pair<long long, int>;
            priority_queue<P, vector<P>, greater<P>> pq;
            pq.push({0, s});

            while (!pq.empty()) {
                auto [d, u] = pq.top();
                pq.pop();
                if (d != dist[u]) continue;
                for (int i = 0; i < (int)G[u].size(); i++) {
                    const auto &e = G[u][i];
                    if (e.cap <= 0) continue;
                    long long nd = d + (e.cost + pot[u] - pot[e.to]);
                    if (nd < dist[e.to]) {
                        dist[e.to] = nd;
                        pv[e.to] = u;
                        pe[e.to] = i;
                        pq.push({nd, e.to});
                    }
                }
            }

            if (dist[t] == INF) break;

            for (int i = 0; i < N; i++) {
                if (dist[i] < INF) pot[i] += dist[i];
            }

            long long addf = maxf - flow;
            for (int v = t; v != s; v = pv[v]) {
                const auto &e = G[pv[v]][pe[v]];
                addf = min(addf, e.cap);
            }

            for (int v = t; v != s; v = pv[v]) {
                auto &e = G[pv[v]][pe[v]];
                auto &re = G[v][e.rev];
                e.cap -= addf;
                re.cap += addf;
                cost += addf * e.cost;
            }
            flow += addf;
        }

        return {flow, cost};
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int m, n;
    if (!(cin >> m >> n)) return 0;

    vector<long long> a(m), b(n);
    for (int i = 0; i < m; i++) cin >> a[i];
    for (int j = 0; j < n; j++) cin >> b[j];

    vector<vector<long long>> c(m, vector<long long>(n));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) cin >> c[i][j];
    }

    long long total = 0;
    for (auto x : a) total += x;

    int S = 0;
    int W0 = 1;          // warehouses: [1 .. m]
    int R0 = 1 + m;      // retailers:  [m+1 .. m+n]
    int T = 1 + m + n;   // sink
    int N = T + 1;

    auto solve = [&](int sign) -> long long {
        // sign = +1 求最小费用；sign = -1 求最大费用（费用取反后求最小，再取反）
        MinCostMaxFlow mf(N);
        for (int i = 0; i < m; i++) mf.addEdge(S, W0 + i, a[i], 0);
        for (int j = 0; j < n; j++) mf.addEdge(R0 + j, T, b[j], 0);

        const long long INF_CAP = (1LL << 60);
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                mf.addEdge(W0 + i, R0 + j, INF_CAP, (long long)sign * c[i][j]);
            }
        }

        auto [flow, cost] = mf.minCostMaxFlow(S, T, total);
        // 题目保证供需平衡；这里简单防御
        // if (flow != total) cerr << "Flow not enough\n";
        return cost;
    };

    long long minCost = solve(+1);
    long long maxCost = -solve(-1);

    cout << minCost << "\n" << maxCost << "\n";
    return 0;
}