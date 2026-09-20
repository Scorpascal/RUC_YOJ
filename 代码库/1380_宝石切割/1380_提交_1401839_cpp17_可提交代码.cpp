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

struct FastScanner {
    static constexpr size_t BUFSZ = 1 << 20;
    unsigned char buf[BUFSZ];
    size_t idx = 0, size = 0;

    inline unsigned char read() {
        if (idx >= size) {
            size = fread(buf, 1, BUFSZ, stdin);
            idx = 0;
            if (size == 0) return 0;
        }
        return buf[idx++];
    }

    template <class T>
    bool readInt(T &out) {
        unsigned char c;
        do {
            c = read();
            if (!c) return false;
        } while (c <= ' ');

        T sign = 1;
        if (c == '-') { sign = -1; c = read(); }

        T x = 0;
        while (c > ' ') {
            x = x * 10 + (c - '0');
            c = read();
        }
        out = x * sign;
        return true;
    }
};

static constexpr uint32_t MOD1 = 1000000007u;
static constexpr uint32_t MOD2 = 1000000009u;
static constexpr uint32_t B1 = 911382323u;
static constexpr uint32_t B2 = 972663749u;
static constexpr uint32_t C  = 1000003u;

static inline uint32_t addmod(uint32_t a, uint32_t b, uint32_t mod) {
    uint64_t s = (uint64_t)a + b;
    return (uint32_t)(s >= mod ? s - mod : s);
}
static inline uint32_t submod(uint32_t a, uint32_t b, uint32_t mod) {
    return (uint32_t)(a >= b ? a - b : a + mod - b);
}
static inline uint32_t mulmod(uint32_t a, uint32_t b, uint32_t mod) {
    return (uint32_t)((uint64_t)a * b % mod);
}

static inline uint32_t normVal1(int x) {
    return (uint32_t)((((uint64_t)(uint32_t)x) % MOD1 + C) % MOD1);
}
static inline uint32_t normVal2(int x) {
    return (uint32_t)((((uint64_t)(uint32_t)x) % MOD2 + C) % MOD2);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    FastScanner fs;

    int m;
    if (!fs.readInt(m)) return 0;

    vector<int> patLen(m);
    vector<uint64_t> patKey(m);
    int maxL = 0;

    // 先读所有模式并计算其双模hash
    for (int i = 0; i < m; i++) {
        int L;
        fs.readInt(L);
        patLen[i] = L;
        maxL = max(maxL, L);

        uint32_t h1 = 0, h2 = 0;
        for (int k = 0; k < L; k++) {
            int x; fs.readInt(x);
            h1 = addmod(mulmod(h1, B1, MOD1), normVal1(x), MOD1);
            h2 = addmod(mulmod(h2, B2, MOD2), normVal2(x), MOD2);
        }
        patKey[i] = (uint64_t(h1) << 32) | uint64_t(h2);
    }

    int n;
    fs.readInt(n);

    // 只需要 pow 到 maxL（因为只会取长度<=maxL的子串hash）
    vector<uint32_t> pow1(maxL + 1), pow2(maxL + 1);
    pow1[0] = 1; pow2[0] = 1;
    for (int i = 1; i <= maxL; i++) {
        pow1[i] = mulmod(pow1[i - 1], B1, MOD1);
        pow2[i] = mulmod(pow2[i - 1], B2, MOD2);
    }

    // 前缀hash：H[i] 表示前 i 个元素（0..i-1）的hash
    vector<uint32_t> pref1(n + 1, 0), pref2(n + 1, 0);
    for (int i = 0; i < n; i++) {
        int x; fs.readInt(x);
        pref1[i + 1] = addmod(mulmod(pref1[i], B1, MOD1), normVal1(x), MOD1);
        pref2[i + 1] = addmod(mulmod(pref2[i], B2, MOD2), normVal2(x), MOD2);
    }

    auto getKey = [&](int l, int L) -> uint64_t {
        int r = l + L;
        uint32_t a1 = pref1[r];
        uint32_t b1 = mulmod(pref1[l], pow1[L], MOD1);
        uint32_t h1 = submod(a1, b1, MOD1);

        uint32_t a2 = pref2[r];
        uint32_t b2 = mulmod(pref2[l], pow2[L], MOD2);
        uint32_t h2 = submod(a2, b2, MOD2);

        return (uint64_t(h1) << 32) | uint64_t(h2);
    };

    // maxStart[end] = 在所有“以 end 结尾”的匹配中，最大的 start（用于贪心判定是否能接上）
    vector<int> maxStart(n, -1);

    // 枚举每个起点 i，尝试匹配每个模式 j（m<=10，所以 O(n*m) 可行）
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            int L = patLen[j];
            if (i + L > n) continue;
            uint64_t key = getKey(i, L);
            if (key == patKey[j]) {
                int end = i + L - 1;
                if (i > maxStart[end]) maxStart[end] = i;
            }
        }
    }

    // 按 end 从小到大做“最早结束优先”贪心
    int ans = 0;
    int lastEnd = -1;
    for (int end = 0; end < n; end++) {
        if (maxStart[end] > lastEnd) {
            ans++;
            lastEnd = end;
        }
    }

    printf("%d\n", ans);
    return 0;
}