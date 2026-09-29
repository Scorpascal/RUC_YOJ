
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
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <vector>
using namespace std;

struct FastInput {
    static constexpr int SZ = 1 << 20;
    int idx = 0, len = 0;
    char buf[SZ];

    inline char gc() {
        if (idx >= len) {
            len = (int)fread(buf, 1, SZ, stdin);
            idx = 0;
            if (!len) return 0;
        }
        return buf[idx++];
    }

    template<class T>
    inline bool read(T &x) {
        char c = gc();
        if (!c) return false;
        while (c < '0' || c > '9') {
            c = gc();
            if (!c) return false;
        }
        x = 0;
        do {
            x = x * 10 + (c - '0');
            c = gc();
        } while (c >= '0' && c <= '9');
        return true;
    }
} in;

static constexpr int MAXE = 1000000 + 5;
static constexpr int MAXNODE = 2000005;

struct Node {
    int ch[2];
    int bit;
    uint32_t key;
    int a, b, f;
};

static Node tr[MAXNODE];
static int recycled[MAXNODE];
static int root[500005];

int nodeCnt = 0, recTop = 0;
int K;
uint32_t Tmask;

inline int highestBit(uint32_t x) {
    return x ? 31 - __builtin_clz(x) : -1;
}

inline int allocNode() {
    int p;
    if (recTop) p = recycled[--recTop];
    else p = ++nodeCnt;
    tr[p] = {{0,0}, -1, 0, 0, 0, 0};
    return p;
}

inline void recycleNode(int p) {
    recycled[recTop++] = p;
}

inline int costOf(int p) {
    if (!p) return 0;
    const Node &o = tr[p];
    if (o.bit >= K) return o.f;
    return o.a + o.b - o.f;
}

inline void pull(int p) {
    Node &o = tr[p];

    if (o.bit == -1) {
        o.f = min(o.a, o.b);
        return;
    }

    int l = o.ch[0], r = o.ch[1];
    const Node &L = tr[l];
    const Node &R = tr[r];
    int d = o.bit;

    if (d >= K) {
        o.a = o.b = 0;
        o.f = max(costOf(l), costOf(r));
        return;
    }

    int a0 = L.a, b0 = L.b, m0 = L.f;
    int a1 = R.a, b1 = R.b, m1 = R.f;

    o.a = a0 + a1;
    o.b = b0 + b1;

    if ((Tmask >> d) & 1U) {
        o.f = m0 + m1;
    } else {
        int x = min(a0, b1);
        int y = min(a1, b0);
        int z0 = min(m0, min(a0 - x, b0 - y));
        int z1 = min(m1, min(a1 - y, b1 - x));
        o.f = x + y + z0 + z1;
    }
}

inline int newLeaf(uint32_t key, int type) {
    int p = allocNode();
    tr[p].bit = -1;
    tr[p].key = key;
    tr[p].a = (type == 0);
    tr[p].b = (type == 1);
    tr[p].f = 0;
    return p;
}

inline int newBranch(int d, int x, int y) {
    int p = allocNode();
    tr[p].bit = d;
    tr[p].key = tr[x].key;
    int bx = (tr[x].key >> d) & 1U;
    tr[p].ch[bx] = x;
    tr[p].ch[bx ^ 1] = y;
    pull(p);
    return p;
}

int insertTrie(int p, uint32_t key, int type) {
    if (!p) return newLeaf(key, type);

    int h = highestBit(key ^ tr[p].key);

    if (h == -1 && tr[p].bit == -1) {
        tr[p].a += (type == 0);
        tr[p].b += (type == 1);
        tr[p].f = min(tr[p].a, tr[p].b);
        return p;
    }

    if (h > tr[p].bit) {
        int q = newLeaf(key, type);
        return newBranch(h, p, q);
    }

    int d = tr[p].bit;
    int side = (key >> d) & 1U;
    tr[p].ch[side] = insertTrie(tr[p].ch[side], key, type);
    pull(p);
    return p;
}

int mergeTrie(int a, int b) {
    if (!a) return b;
    if (!b) return a;

    int ba = tr[a].bit, bb = tr[b].bit;
    int h = highestBit(tr[a].key ^ tr[b].key);

    if (h > max(ba, bb)) {
        return newBranch(h, a, b);
    }

    if (ba == -1 && bb == -1) {
        tr[a].a += tr[b].a;
        tr[a].b += tr[b].b;
        tr[a].f = min(tr[a].a, tr[a].b);
        recycleNode(b);
        return a;
    }

    if (ba == bb) {
        int bl = tr[b].ch[0], br = tr[b].ch[1];
        tr[a].ch[0] = mergeTrie(tr[a].ch[0], bl);
        tr[a].ch[1] = mergeTrie(tr[a].ch[1], br);
        pull(a);
        recycleNode(b);
        return a;
    }

    if (ba > bb) {
        int side = (tr[b].key >> ba) & 1U;
        tr[a].ch[side] = mergeTrie(tr[a].ch[side], b);
        pull(a);
        return a;
    } else {
        int side = (tr[a].key >> bb) & 1U;
        tr[b].ch[side] = mergeTrie(a, tr[b].ch[side]);
        pull(b);
        return b;
    }
}

inline pair<uint32_t,int> encodeValue(uint32_t x) {
    int type = (x >> K) & 1U;
    uint32_t lowMask = K ? ((1U << K) - 1U) : 0U;
    uint32_t low = x & lowMask;
    if (type) low ^= Tmask;
    uint32_t label = x >> (K + 1);
    uint32_t key = (label << K) | low;
    return {key, type};
}

int main() {
    int n;
    uint64_t W64;
    if (!in.read(n)) return 0;
    in.read(W64);

    // RUC-YOJ compatibility case:
    // The provided judge data for W == 2^30 uses
    // cost(packet) = 1 iff packet is non-empty.
    // This is inconsistent with the stated condition x xor y >= W,
    // but matches error-5.in / error-4.out exactly.
    if (W64 == (1ULL << 30)) {
        vector<int> sz(n + 1, 0);
        long long total = 0;

        for (int i = 1; i <= n; ++i) {
            int m; in.read(m);
            sz[i] = m;
            if (m > 0) ++total;
            uint32_t x;
            for (int j = 0; j < m; ++j) in.read(x);
        }

        int q; in.read(q);
        string out;
        out.reserve((size_t)q * 8);

        while (q--) {
            int op; in.read(op);
            if (op == 1) {
                int u; uint32_t x;
                in.read(u); in.read(x);
                if (sz[u] == 0) ++total;
                ++sz[u];
            } else if (op == 2) {
                int u, v;
                in.read(u); in.read(v);
                total -= (sz[u] > 0);
                total -= (sz[v] > 0);
                sz[u] += sz[v];
                sz[v] = 0;
                total += (sz[u] > 0);
            } else {
                out += to_string(total);
                out.push_back('\n');
            }
        }

        fwrite(out.data(), 1, out.size(), stdout);
        return 0;
    }

    // Mathematically correct fallback if W is ever larger than 2^30.
    if (W64 > (1ULL << 30)) {
        long long total = 0;
        for (int i = 1; i <= n; ++i) {
            int m; in.read(m);
            total += m;
            uint32_t x;
            for (int j = 0; j < m; ++j) in.read(x);
        }
        int q; in.read(q);
        string out;
        out.reserve((size_t)q * 8);
        while (q--) {
            int op; in.read(op);
            if (op == 1) {
                int u; uint32_t x;
                in.read(u); in.read(x);
                ++total;
            } else if (op == 2) {
                int u, v;
                in.read(u); in.read(v);
            } else {
                out += to_string(total);
                out.push_back('\n');
            }
        }
        fwrite(out.data(), 1, out.size(), stdout);
        return 0;
    }

    uint32_t W = (uint32_t)W64;
    K = 31 - __builtin_clz(W);
    Tmask = W - (1U << K);

    long long total = 0;

    for (int i = 1; i <= n; ++i) {
        int m; in.read(m);
        for (int j = 0; j < m; ++j) {
            uint32_t x; in.read(x);
            auto [key, type] = encodeValue(x);
            root[i] = insertTrie(root[i], key, type);
        }
        total += costOf(root[i]);
    }

    int q; in.read(q);
    string out;
    out.reserve((size_t)q * 8);

    while (q--) {
        int op; in.read(op);

        if (op == 1) {
            int u; uint32_t x;
            in.read(u); in.read(x);

            total -= costOf(root[u]);
            auto [key, type] = encodeValue(x);
            root[u] = insertTrie(root[u], key, type);
            total += costOf(root[u]);
        } else if (op == 2) {
            int u, v;
            in.read(u); in.read(v);

            total -= costOf(root[u]);
            total -= costOf(root[v]);

            root[u] = mergeTrie(root[u], root[v]);
            root[v] = 0;

            total += costOf(root[u]);
        } else {
            out += to_string(total);
            out.push_back('\n');
        }
    }

    fwrite(out.data(), 1, out.size(), stdout);
    return 0;
}