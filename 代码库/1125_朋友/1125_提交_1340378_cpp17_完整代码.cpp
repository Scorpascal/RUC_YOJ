#include <bits/stdc++.h>
using namespace std;

static const int MAXN = 128;
static const int LIMIT = 1000;

struct Solver {
    int n = 0, m = 0;
    bitset<MAXN> adj[MAXN];
    bitset<MAXN> maskAll;
    int cnt = 0;

    void bronk(bitset<MAXN> P, bitset<MAXN> X) {
        if (cnt > LIMIT) return;
        if (P.none() && X.none()) {
            ++cnt;
            return;
        }

        // 选 pivot：在 P|X 中找使 |P ∩ N(u)| 最大的 u
        bitset<MAXN> PX = (P | X);
        int pivot = -1;
        size_t best = 0;
        for (int u = 0; u < n; ++u) {
            if (!PX.test(u)) continue;
            size_t c = (P & adj[u]).count();
            if (pivot == -1 || c > best) {
                pivot = u;
                best = c;
            }
        }

        bitset<MAXN> candidates;
        if (pivot == -1) {
            candidates = P;
        } else {
            // P \ N(pivot)
            candidates = P & (maskAll ^ (adj[pivot] & maskAll));
        }

        for (int v = 0; v < n; ++v) {
            if (cnt > LIMIT) return;
            if (!candidates.test(v)) continue;

            bitset<MAXN> Pv = P & adj[v];
            bitset<MAXN> Xv = X & adj[v];
            bronk(Pv, Xv);

            P.reset(v);
            X.set(v);
        }
    }

    int solve() {
        cnt = 0;
        bitset<MAXN> P, X;
        P = maskAll;
        X.reset();
        bronk(P, X);
        return cnt;
    }
};

static bool readNonEmptyLine(istream &in, string &line) {
    while (std::getline(in, line)) {
        // 保留可能的空格行判断
        bool allSpace = true;
        for (char ch : line) if (!isspace((unsigned char)ch)) { allSpace = false; break; }
        if (!allSpace) return true;
        // 否则跳过空行
    }
    return false;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    while (true) {
        if (!readNonEmptyLine(cin, line)) break;

        // 读 n m
        int n, m;
        {
            stringstream ss(line);
            if (!(ss >> n >> m)) break;
        }

        Solver s;
        s.n = n;
        s.m = m;
        s.maskAll.reset();
        for (int i = 0; i < n; ++i) s.maskAll.set(i);
        for (int i = 0; i < n; ++i) s.adj[i].reset();

        // 读 m 条边（中间可能夹空行，稳健处理）
        int got = 0;
        while (got < m) {
            if (!std::getline(cin, line)) break;
            bool allSpace = true;
            for (char ch : line) if (!isspace((unsigned char)ch)) { allSpace = false; break; }
            if (allSpace) continue;

            int a, b;
            stringstream ss(line);
            if (!(ss >> a >> b)) continue;
            --a; --b;
            if (a >= 0 && a < n && b >= 0 && b < n && a != b) {
                s.adj[a].set(b);
                s.adj[b].set(a);
            }
            ++got;
        }

        int ans = s.solve();
        if (ans > LIMIT) {
            cout << "Too many maximal sets of friends.\n";
        } else {
            cout << ans << "\n";
        }
    }

    return 0;
}