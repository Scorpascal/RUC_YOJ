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

class FastScanner {
    static constexpr size_t BUFSIZE = 1 << 20;
    char buf[BUFSIZE];
    size_t idx = 0, size = 0;

    inline char readChar() {
        if (idx >= size) {
            size = fread(buf, 1, BUFSIZE, stdin);
            idx = 0;
            if (size == 0) return 0;
        }
        return buf[idx++];
    }

public:
    template <class T>
    bool readInt(T &out) {
        char c;
        do {
            c = readChar();
            if (!c) return false;
        } while (c <= ' ');

        bool neg = false;
        if (c == '-') {
            neg = true;
            c = readChar();
        }

        T val = 0;
        while (c > ' ') {
            val = val * 10 + (c - '0');
            c = readChar();
        }
        out = neg ? -val : val;
        return true;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    FastScanner fs;

    int n, k;
    if (!fs.readInt(n)) return 0;
    fs.readInt(k);

    vector<int> c(n + 1);
    for (int i = 1; i <= n; i++) fs.readInt(c[i]);

    const long long NEG = -(1LL << 60);
    vector<long long> best(k + 1, NEG);

    long long dpPrev = 0;   // dp[i-1]
    long long prefPrev = 0; // S[i-1]
    long long pref = 0;     // S[i]

    for (int i = 1; i <= n; i++) {
        long long v;
        fs.readInt(v);
        pref = prefPrev + v;

        int col = c[i];

        long long dpCur = dpPrev;
        if (best[col] > NEG / 2) {
            long long cand = pref + best[col];
            if (cand > dpCur) dpCur = cand;
        }

        // 更新 best[col]：把当前位置 i 作为未来区间左端点 l
        long long startVal = dpPrev - prefPrev;
        if (startVal > best[col]) best[col] = startVal;

        dpPrev = dpCur;
        prefPrev = pref;
    }

    cout << dpPrev << "\n";
    return 0;
}
/*思路（DP + 按花色维护最优转移）
设前缀和 S[i]=v1+...+vi。

令 dp[i] 表示：只考虑前 i 张牌，最多能得到的分数。

处理第 i 张牌（花色 c[i]）时：

不删任何段：dp[i] = dp[i-1]
若选择删掉一段 以 i 结尾 的区间 [l, i]，要求 c[l]=c[i]，且为了与之前操作不冲突，这段必须与之前删的段不重叠，因此前面最多得分是 dp[l-1]，本次得分是 S[i]-S[l-1]：
[
dp[i] = \max\Big(dp[i-1],\ \max_{l<i,\ c[l]=c[i]} \big(dp[l-1] + S[i]-S[l-1]\big)\Big)
]

把式子改写：

[
dp[l-1] + S[i]-S[l-1] = S[i] + (dp[l-1]-S[l-1])
]

所以对每个花色 col 维护：

[
best[col] = \max_{l,\ c[l]=col} (dp[l-1]-S[l-1])
]

那么转移就是：

candidate = S[i] + best[c[i]]
dp[i] = max(dp[i-1], candidate)
更新 best 时，当前位置 i 作为未来区间的左端点 l，对应值是 dp[i-1]-S[i-1]：

best[c[i]] = max(best[c[i]], dp[i-1]-S[i-1])
答案为 dp[n]。

GPT-5.2 • 1x*/