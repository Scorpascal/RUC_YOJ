#include <cstdio>
#include <cstdint>

using u32 = uint32_t;

const int N = 100000 + 5;
const int K = 20;
const int CD = 20;
const int LG = 17;
const int MAXE = 2 * N;
const int MAXSW = 2100005;
const int MAXCW = 200005;
const int MAXQ = 4000005;

struct FastIn {
    static const int B = 1 << 20;
    char buf[B];
    int p = 0, n = 0;

    inline char gc() {
        if (p == n) {
            n = (int)fread(buf, 1, B, stdin);
            p = 0;
            if (!n) return 0;
        }
        return buf[p++];
    }

    inline int nextInt() {
        char c = gc();
        while (c <= ' ') c = gc();

        int x = 0;
        while (c > ' ') {
            x = x * 10 + (c - '0');
            c = gc();
        }
        return x;
    }
} in;

int n, X, k, m;

int head[N], to[MAXE], nx[MAXE], idof[MAXE], ec;

int par[N], pedge[N], dep[N];
int up[LG][N];

int station[K], sid[N];
int distK[K][N];

int cen[CD][N], cdep[N];
int tpar[N], sub[N];

unsigned char blocked[N];
unsigned char good[N];
unsigned char donePlan[N];

int S[N], T[N];
int needPlan[N], planD[N];

int sHead[K * N];
int swPlan[MAXSW], swNext[MAXSW], swCnt;

int cHead[CD * N];
int cwPlan[MAXCW], cwNext[MAXCW], cwCnt;

u32 s0[N], s1[N];
u32 c0[N], c1[N];

u32 que[MAXQ];
int qh, qt;

int ans;

inline void addEdge(int u, int v, int id) {
    to[++ec] = v;
    idof[ec] = id;
    nx[ec] = head[u];
    head[u] = ec;
}

inline u32 pack(int ctx, int layer, int v) {
    return (u32(ctx) << 18)
         | (u32(layer) << 17)
         | u32(v);
}

int lca(int a, int b) {
    if (dep[a] < dep[b]) {
        int t = a;
        a = b;
        b = t;
    }

    int d = dep[a] - dep[b];

    for (int j = 0; j < LG; ++j)
        if ((d >> j) & 1)
            a = up[j][a];

    if (a == b)
        return a;

    for (int j = LG - 1; j >= 0; --j) {
        if (up[j][a] != up[j][b]) {
            a = up[j][a];
            b = up[j][b];
        }
    }

    return par[a];
}

inline int treeDist(int a, int b) {
    int c = lca(a, b);
    return dep[a] + dep[b] - 2 * dep[c];
}

inline void addSW(int r, int v, int pid) {
    int key = r * N + v;
    int z = ++swCnt;

    swPlan[z] = pid;
    swNext[z] = sHead[key];
    sHead[key] = z;

    ++needPlan[pid];
}

inline void addCW(int d, int v, int pid) {
    int key = d * N + v;
    int z = ++cwCnt;

    cwPlan[z] = pid;
    cwNext[z] = cHead[key];
    cHead[key] = z;
}

inline void fireS(int r, int v) {
    for (int e = sHead[r * N + v]; e; e = swNext[e]) {
        int p = swPlan[e];

        if (donePlan[p])
            continue;

        if (--needPlan[p] == 0) {
            donePlan[p] = 1;
            ++ans;
        }
    }
}

inline void pushS(int r, int v, int layer) {
    u32 bit = 1u << r;
    u32 &z = layer ? s1[v] : s0[v];

    if (z & bit)
        return;

    bool first = ((s0[v] | s1[v]) & bit) == 0;

    z |= bit;

    if (first)
        fireS(r, v);

    que[qt++] = pack(r, layer, v);
}

void runS() {
    while (qh < qt) {
        u32 code = que[qh++];

        int v = code & 131071u;
        int layer = (code >> 17) & 1;
        int r = code >> 18;

        for (int e = head[v]; e; e = nx[e]) {
            int u = to[e];

            if (!layer)
                pushS(r, u, 1);

            if (good[idof[e]])
                pushS(r, u, layer);
        }
    }

    qh = qt = 0;
}

inline void checkCPlan(int p) {
    if (donePlan[p])
        return;

    int d = planD[p];
    u32 bit = 1u << d;

    int s = S[p];
    int t = T[p];

    bool zs = c0[s] & bit;
    bool zt = c0[t] & bit;

    bool os = (c0[s] | c1[s]) & bit;
    bool ot = (c0[t] | c1[t]) & bit;

    if ((zs && ot) || (zt && os)) {
        donePlan[p] = 1;
        ++ans;
    }
}

inline void fireC(int d, int v) {
    for (int e = cHead[d * N + v]; e; e = cwNext[e])
        checkCPlan(cwPlan[e]);
}

inline void pushC(int d, int v, int layer) {
    u32 bit = 1u << d;
    u32 &z = layer ? c1[v] : c0[v];

    if (z & bit)
        return;

    z |= bit;

    fireC(d, v);

    que[qt++] = pack(d, layer, v);
}

void runC() {
    while (qh < qt) {
        u32 code = que[qh++];

        int v = code & 131071u;
        int layer = (code >> 17) & 1;
        int d = code >> 18;

        int ctx = cen[d][v];

        for (int e = head[v]; e; e = nx[e]) {
            int u = to[e];

            if (cen[d][u] != ctx)
                continue;

            if (!layer)
                pushC(d, u, 1);

            if (good[idof[e]])
                pushC(d, u, layer);
        }
    }

    qh = qt = 0;
}

int main() {
    n = in.nextInt();
    X = in.nextInt();

    for (int i = 1; i < n; ++i) {
        int u = in.nextInt();
        int v = in.nextInt();

        addEdge(u, v, i);
        addEdge(v, u, i);
    }

    static int st[N];
    int top = 0;

    st[top++] = 1;

    while (top) {
        int v = st[--top];

        for (int e = head[v]; e; e = nx[e]) {
            int u = to[e];

            if (u == par[v])
                continue;

            par[u] = v;
            pedge[u] = idof[e];
            dep[u] = dep[v] + 1;

            st[top++] = u;
        }
    }

    for (int v = 1; v <= n; ++v)
        up[0][v] = par[v];

    for (int j = 1; j < LG; ++j)
        for (int v = 1; v <= n; ++v)
            up[j][v] = up[j - 1][up[j - 1][v]];

    k = in.nextInt();

    for (int i = 1; i <= n; ++i)
        sid[i] = -1;

    for (int r = 0; r < k; ++r) {
        station[r] = in.nextInt();
        sid[station[r]] = r;
    }

    static int sp[N], sv[N];

    for (int r = 0; r < k; ++r) {
        int tp = 0;

        sv[tp] = station[r];
        sp[tp++] = 0;

        distK[r][station[r]] = 0;

        while (tp) {
            --tp;

            int v = sv[tp];
            int p = sp[tp];

            for (int e = head[v]; e; e = nx[e]) {
                int u = to[e];

                if (u == p)
                    continue;

                distK[r][u] = distK[r][v] + 1;

                sv[tp] = u;
                sp[tp++] = v;
            }
        }
    }

    static int taskV[N], taskD[N];
    static int nodes[N], ws[N];

    int tc = 0;

    taskV[tc] = 1;
    taskD[tc++] = 0;

    while (tc) {
        --tc;

        int entry = taskV[tc];
        int d = taskD[tc];

        int nc = 0;
        int wc = 0;

        tpar[entry] = 0;
        ws[wc++] = entry;

        while (wc) {
            int v = ws[--wc];

            nodes[nc++] = v;

            for (int e = head[v]; e; e = nx[e]) {
                int u = to[e];

                if (blocked[u] || u == tpar[v])
                    continue;

                tpar[u] = v;
                ws[wc++] = u;
            }
        }

        for (int i = nc - 1; i >= 0; --i) {
            int v = nodes[i];
            int z = 1;

            for (int e = head[v]; e; e = nx[e]) {
                int u = to[e];

                if (!blocked[u] && tpar[u] == v)
                    z += sub[u];
            }

            sub[v] = z;
        }

        int c = 0;

        for (int i = 0; i < nc && !c; ++i) {
            int v = nodes[i];
            int mx = nc - sub[v];

            for (int e = head[v]; e; e = nx[e]) {
                int u = to[e];

                if (!blocked[u] &&
                    tpar[u] == v &&
                    sub[u] > mx)
                    mx = sub[u];
            }

            if (mx * 2 <= nc)
                c = v;
        }

        for (int i = 0; i < nc; ++i)
            cen[d][nodes[i]] = c;

        cdep[c] = d;
        blocked[c] = 1;

        for (int e = head[c]; e; e = nx[e]) {
            int u = to[e];

            if (!blocked[u]) {
                taskV[tc] = u;
                taskD[tc++] = d + 1;
            }
        }
    }

    m = in.nextInt();

    for (int p = 1; p <= m; ++p) {
        S[p] = in.nextInt();
        T[p] = in.nextInt();
    }

    for (int p = 1; p <= m; ++p) {
        int s = S[p];
        int t = T[p];

        int len = treeDist(s, t);

        int ordR[K];
        int ordD[K];

        int cnt = 0;

        for (int r = 0; r < k; ++r) {
            if (distK[r][s] + distK[r][t] == len) {
                int ds = distK[r][s];

                int pos = cnt;

                while (pos && ordD[pos - 1] > ds) {
                    ordD[pos] = ordD[pos - 1];
                    ordR[pos] = ordR[pos - 1];
                    --pos;
                }

                ordD[pos] = ds;
                ordR[pos] = r;

                ++cnt;
            }
        }

        if (cnt) {
            int prev = s;

            for (int z = 0; z < cnt; ++z) {
                int r = ordR[z];
                int cur = station[r];

                if (cur == prev)
                    continue;

                if (sid[prev] >= 0)
                    addSW(sid[prev], cur, p);
                else
                    addSW(r, prev, p);

                prev = cur;
            }

            if (prev != t)
                addSW(sid[prev], t, p);
        }
        else {
            int lim =
                cdep[s] < cdep[t]
                    ? cdep[s]
                    : cdep[t];

            int d = 0;

            while (d < lim &&
                   cen[d + 1][s] == cen[d + 1][t])
                ++d;

            planD[p] = d;

            addCW(d, s, p);
            addCW(d, t, p);
        }
    }

    for (int r = 0; r < k; ++r)
        pushS(r, station[r], 0);

    runS();

    for (int c = 1; c <= n; ++c)
        pushC(cdep[c], c, 0);

    runC();

    int q = in.nextInt();
    int lastans = 0;

    while (q--) {
        int op = in.nextInt();

        if (op == 2) {
            printf("%d\n", ans);
            lastans = ans;
            continue;
        }

        int u =
            in.nextInt() ^ (X * lastans);

        int v =
            in.nextInt() ^ (X * lastans);

        int eid =
            (par[u] == v)
                ? pedge[u]
                : pedge[v];

        good[eid] = 1;

        for (int r = 0; r < k; ++r) {
            u32 b = 1u << r;

            if (s0[u] & b)
                pushS(r, v, 0);

            if (s0[v] & b)
                pushS(r, u, 0);

            if (s1[u] & b)
                pushS(r, v, 1);

            if (s1[v] & b)
                pushS(r, u, 1);
        }

        runS();

        for (int d = 0; d < CD; ++d) {
            int a = cen[d][u];
            int bctx = cen[d][v];

            if (!a || !bctx || a != bctx)
                break;

            u32 bit = 1u << d;

            if (c0[u] & bit)
                pushC(d, v, 0);

            if (c0[v] & bit)
                pushC(d, u, 0);

            if (c1[u] & bit)
                pushC(d, v, 1);

            if (c1[v] & bit)
                pushC(d, u, 1);
        }

        runC();
    }

    return 0;
}