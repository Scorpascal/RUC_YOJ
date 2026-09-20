#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef long long ll;
const int MAXV = 210 + 5; // n <= 200
const int MAXE = 100000;

int n, m;
int head[MAXV], to[MAXE], next_e[MAXE];
ll cap[MAXE];
int ec;

void add_edge_internal(int u, int v, ll c) {
    to[ec] = v; cap[ec] = c; next_e[ec] = head[u]; head[u] = ec++;
}
void add_edge(int u, int v, ll c) {
    add_edge_internal(u, v, c);
    add_edge_internal(v, u, 0);
}

int S, T;
int level[MAXV], it[MAXV];
int q[MAXV];

int bfs() {
    memset(level, -1, sizeof(level));
    int qh = 0, qt = 0;
    q[qt++] = S; level[S] = 0;
    while (qh < qt) {
        int u = q[qh++];
        for (int e = head[u]; e != -1; e = next_e[e]) {
            int v = to[e];
            if (cap[e] > 0 && level[v] < 0) {
                level[v] = level[u] + 1;
                q[qt++] = v;
                if (v == T) return 1;
            }
        }
    }
    return level[T] >= 0;
}

ll dfs(int u, ll f) {
    if (u == T) return f;
    int e;
    while ((e = it[u]) != -1) {
        int v = to[e];
        if (cap[e] > 0 && level[v] == level[u] + 1) {
            ll ret = dfs(v, (f < cap[e]) ? f : cap[e]);
            if (ret > 0) {
                cap[e] -= ret;
                cap[e^1] += ret;
                return ret;
            }
        }
        it[u] = next_e[e];
    }
    return 0;
}

ll maxflow() {
    ll flow = 0;
    while (bfs()) {
        for (int i = 0; i <= T; ++i) it[i] = head[i];
        while (1) {
            ll f = dfs(S, (1LL<<60));
            if (!f) break;
            flow += f;
        }
    }
    return flow;
}

int main() {
    if (scanf("%d %d", &n, &m) != 2) return 0;
    // node indices: 1..n, ss = n+1, tt = n+2
    int ss = n + 1, tt = n + 2;
    S = ss; T = tt;
    for (int i = 0; i <= tt; ++i) head[i] = -1;
    ec = 0;

    int *U = (int*)malloc(sizeof(int)* (m+5));
    int *V = (int*)malloc(sizeof(int)* (m+5));
    int *L = (int*)malloc(sizeof(int)* (m+5));
    int *R = (int*)malloc(sizeof(int)* (m+5));
    int *edge_idx = (int*)malloc(sizeof(int)* (m+5));
    ll demand[MAXV];
    for (int i = 0; i <= tt; ++i) demand[i] = 0;

    for (int i = 1; i <= m; ++i) {
        int s,t,l,u;
        scanf("%d %d %d %d", &s, &t, &l, &u);
        U[i]=s; V[i]=t; L[i]=l; R[i]=u;
        // capacity for transformed graph:
        int cap0 = u - l;
        // add edge s->t with cap0, record its forward edge index
        edge_idx[i] = ec;
        add_edge(s, t, cap0);
        // adjust demands
        demand[s] -= l;
        demand[t] += l;
    }

    ll need = 0;
    for (int i = 1; i <= n; ++i) {
        if (demand[i] > 0) {
            add_edge(ss, i, demand[i]);
            need += demand[i];
        } else if (demand[i] < 0) {
            add_edge(i, tt, -demand[i]);
        }
    }

    ll flowed = maxflow();
    if (flowed != need) {
        printf("NO\n");
        return 0;
    }

    // feasible, output flows for original edges: used = orig_cap - cap[edge_idx] + lower
    printf("YES\n");
    for (int i = 1; i <= m; ++i) {
        int e = edge_idx[i];
        ll orig_cap = (ll)(R[i] - L[i]);
        ll used = orig_cap - cap[e];
        ll ans = used + (ll)L[i];
        printf("%lld\n", ans);
    }
    return 0;
}