#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <utility>
#include <algorithm>
#include <functional>
#include <cstring>
using namespace std;

using i64 = long long;
const int ALPHA = 26;
const int INF_INT = 0x3f3f3f3f;
const i64 INF64 = (i64)4e18;

struct RouteRec {
    int u, v;        // 起点字母，终点字母 [0..25]
    int w;           // |s|-1
    int mask;        // t 字母子集掩码
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int N;
    string t;
    if (!(cin >> N >> t)) return 0;
    int K = (int)t.size();
    int M = 1 << K;
    // t 中每个字母映射到 bit 位置
    int mapBit[26]; fill(begin(mapBit), end(mapBit), -1);
    for (int i = 0; i < K; ++i) mapBit[t[i]-'a'] = i;

    // 精确掩码的最小费用：exact[(a*26+b)*M + me]
    vector<int> exact(ALPHA*ALPHA*M, INF_INT);
    auto EX = [&](int a, int b, int me)->int& {
        return exact[(a*ALPHA + b)*M + me];
    };
    // 记录每个 (a,b) 是否存在任意边
    bool hasAB[ALPHA][ALPHA]; memset(hasAB, 0, sizeof(hasAB));

    vector<RouteRec> recs;
    recs.reserve(N);
    vector<char> usedU(ALPHA, 0);

    // 读取 N 条航线
    string s;
    for (int i = 0; i < N; ++i) {
        cin >> s;
        int L = (int)s.size();
        int a = s.front() - 'a';
        int b = s.back()  - 'a';
        int w = L - 1;
        int m = 0;
        for (char c : s) { int mb = mapBit[c-'a']; if (mb >= 0) m |= (1<<mb); }
        // 更新精确掩码的最小费用
        int &cell = EX(a,b,m);
        if (w < cell) cell = w;
        hasAB[a][b] = true;

        recs.push_back({a,b,w,m});
        usedU[a] = 1;
    }

    // 预计算每个 S 的子掩码序列（压缩索引 j -> 原始掩码）
    int Mtot = 1 << K;
    vector<int> pop(Mtot,0), len(Mtot,0), off(Mtot+1,0);
    for (int S = 0; S < Mtot; ++S) pop[S] = __builtin_popcount((unsigned)S);
    for (int S = 0; S < Mtot; ++S) { len[S] = 1 << pop[S]; off[S+1] = off[S] + len[S]; }
    const int TOTL = off[Mtot]; // = sum 2^{popcount(S)} = 3^K

    vector<int> subMasks(TOTL); // 拼接存放，每段长度 len[S]，偏移 off[S]
    for (int S = 0; S < Mtot; ++S) {
        int bits[10], m = pop[S], p = 0;
        for (int j = 0; j < K; ++j) if (S & (1<<j)) bits[p++] = j;
        for (int j = 0; j < len[S]; ++j) {
            int full = 0;
            for (int tbit = 0; tbit < m; ++tbit)
                if (j & (1<<tbit)) full |= (1 << bits[tbit]);
            subMasks[off[S] + j] = full;
        }
    }

    // 为反向松弛准备：对每个 (a,b) 和每个 S，预处理
    // H[(a*26+b)*TOTL + off[S] + Xc] = min 代价，满足 me ⊇ X 且 me ⊆ S
    vector<int> H(ALPHA*ALPHA*TOTL, INF_INT);
    auto HPTR = [&](int a, int b, int S)->int* {
        return &H[(a*ALPHA + b)*TOTL + off[S]];
    };

    for (int a = 0; a < ALPHA; ++a) {
        for (int b = 0; b < ALPHA; ++b) {
            if (!hasAB[a][b]) continue; // 无边则整段保持 INF
            for (int S = 0; S < M; ++S) {
                int Ls = len[S];
                int *dp = HPTR(a,b,S);
                // 初值：dp[me_c] = exact[a][b][me] (me ⊆ S)
                for (int j = 0; j < Ls; ++j) {
                    int me = subMasks[off[S] + j];
                    dp[j] = EX(a,b,me);
                }
                // 受限超集最小化（只在 S 的位上做 superset-min）
                int m = pop[S];
                for (int tb = 0; tb < m; ++tb) {
                    for (int j = 0; j < Ls; ++j) {
                        if ((j & (1<<tb)) == 0) {
                            int v = dp[j | (1<<tb)];
                            if (v < dp[j]) dp[j] = v;
                        }
                    }
                }
            }
        }
    }

    // 对每个终点字母 b，列出有哪些 a 能到 b（加速）
    vector<int> incoming[ALPHA];
    for (int b = 0; b < ALPHA; ++b) {
        for (int a = 0; a < ALPHA; ++a) if (hasAB[a][b]) incoming[b].push_back(a);
    }

    // 仅对出现过的起点字符 u 执行一遍 Dijkstra（反向）
    vector<vector<i64>> allDist(ALPHA); // allDist[u] 大小 26*M
    for (int u = 0; u < ALPHA; ++u) {
        if (!usedU[u]) continue;

        vector<i64> dist(ALPHA * M, INF64);
        auto ID = [&](int node, int S)->int { return node * M + S; };

        using Node = pair<i64,int>;
        priority_queue<Node, vector<Node>, greater<Node>> pq;

        int full = M - 1;
        dist[ID(u, full)] = 0;
        pq.emplace(0, ID(u, full));

        while (!pq.empty()) {
            auto [dcur, id] = pq.top(); pq.pop();
            if (dcur != dist[id]) continue;
            int b = id / M;
            int S = id % M;

            if (incoming[b].empty()) continue;

            int baseOff = off[S];
            int Ls = len[S];

            for (int a : incoming[b]) {
                int *dp = HPTR(a,b,S); // 长度 Ls
                for (int j = 0; j < Ls; ++j) {
                    int w = dp[j];
                    if (w == INF_INT) continue;
                    int Xfull = subMasks[baseOff + j];
                    int newMask = S ^ Xfull;
                    int nid = ID(a, newMask);
                    i64 nd = dcur + (i64)w;
                    if (nd < dist[nid]) {
                        dist[nid] = nd;
                        pq.emplace(nd, nid);
                    }
                }
            }
        }
        allDist[u] = std::move(dist);
    }

    for (const auto &r : recs) {
        if (!usedU[r.u]) { cout << -1 << '\n'; continue; }
        const auto &dist = allDist[r.u];
        i64 add = dist[r.v * M + r.mask];
        if (add >= INF64/2) cout << -1 << '\n';
        else cout << (r.w + add) << '\n';
    }
    return 0;
}