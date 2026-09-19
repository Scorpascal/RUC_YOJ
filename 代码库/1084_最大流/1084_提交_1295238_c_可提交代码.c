#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAXV 105
#define MAXE 10050
typedef long long ll;
const ll INF = (ll)9e18;

int head[MAXV], nxt[MAXE], to[MAXE];
ll cap[MAXE];
int E;

void add_edge(int u, int v, ll c) {
    to[E] = v; cap[E] = c; nxt[E] = head[u]; head[u] = E++;
    to[E] = u; cap[E] = 0;   nxt[E] = head[v]; head[v] = E++;
}

int level[MAXV];
int q[MAXV];

int bfs(int s, int t, int n) {
    for (int i = 1; i <= n; ++i) level[i] = -1;
    int qh = 0, qt = 0;
    q[qt++] = s; level[s] = 0;
    while (qh < qt) {
        int u = q[qh++];
        for (int ei = head[u]; ei != -1; ei = nxt[ei]) {
            int v = to[ei];
            if (cap[ei] > 0 && level[v] == -1) {
                level[v] = level[u] + 1;
                q[qt++] = v;
                if (v == t) return 1;
            }
        }
    }
    return level[t] != -1;
}

int iter[MAXV];

ll dfs(int u, int t, ll f) {
    if (u == t) return f;
    for (int ei = iter[u]; ei != -1; ei = nxt[ei]) {
        iter[u] = ei;
        int v = to[ei];
        if (cap[ei] > 0 && level[v] == level[u] + 1) {
            ll d = dfs(v, t, f < cap[ei] ? f : cap[ei]);
            if (d > 0) {
                cap[ei] -= d;
                cap[ei ^ 1] += d;
                return d;
            }
        }
    }
    iter[u] = -1;
    return 0;
}

ll dinic(int s, int t, int n) {
    ll flow = 0;
    while (bfs(s, t, n)) {
        for (int i = 1; i <= n; ++i) iter[i] = head[i];
        ll f;
        while ((f = dfs(s, t, INF)) > 0) {
            flow += f;
        }
    }
    return flow;
}

int main() {
    int n, m, s, t;
    if (scanf("%d %d %d %d", &n, &m, &s, &t) != 4) return 0;
    for (int i = 1; i <= n; ++i) head[i] = -1;
    E = 0;
    for (int i = 0; i < m; ++i) {
        int u, v;
        long long c;
        scanf("%d %d %lld", &u, &v, &c);
        add_edge(u, v, c);
    }
    ll ans = dinic(s, t, n);
    printf("%lld\n", ans);
    return 0;
}