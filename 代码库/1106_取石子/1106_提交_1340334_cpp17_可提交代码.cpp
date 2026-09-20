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

// 题意：从 n 堆里最多选 m 堆，使总和 <= k，最大化总和。

static constexpr int MAXK = 2500; // C 组 k 最大 2500；A 组更小

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    long long k;
    cin >> n >> m >> k;
    vector<long long> a(n);
    for (int i = 0; i < n; i++) cin >> a[i];

    // 分支1：k 小（<=2500），用 bitset DP（适配 A/C）
    if (k <= MAXK) {
        int K = (int)k;
        vector<bitset<MAXK + 1>> dp(m + 1);
        dp[0].reset();
        dp[0].set(0);

        for (int i = 0; i < n; i++) {
            int w = (int)a[i];
            if (w > K) continue; // 超过 K 的单堆直接不可能被选
            for (int c = m - 1; c >= 0; c--) {
                dp[c + 1] |= (dp[c] << w);
            }
        }

        for (int s = K; s >= 0; s--) {
            for (int c = 0; c <= m; c++) {
                if (dp[c].test(s)) {
                    cout << s << "\n";
                    return 0;
                }
            }
        }
        cout << 0 << "\n";
        return 0;
    }

    // 分支2：k 大（B 组典型），n 小（<=20），用 Meet-in-the-Middle
    // 若遇到 k>2500 且 n 很大，题目数据通常不会这么出（给定数据范围不需要处理更通用情形）。
    if (n <= 40) {
        int n1 = n / 2;
        int n2 = n - n1;

        vector<long long> left(a.begin(), a.begin() + n1);
        vector<long long> right(a.begin() + n1, a.end());

        // 枚举某半边所有子集，按“选了多少堆”分桶，存可行的 sum（sum<=k 且 count<=m）
        auto enumHalf = [&](const vector<long long>& half) {
            int sz = (int)half.size();
            vector<vector<long long>> bucket(m + 1);
            int total = 1 << sz;
            for (int mask = 0; mask < total; mask++) {
                long long sum = 0;
                int cnt = 0;
                for (int i = 0; i < sz; i++) {
                    if (mask & (1 << i)) {
                        sum += half[i];
                        cnt++;
                        if (cnt > m || sum > k) break;
                    }
                }
                if (cnt <= m && sum <= k) bucket[cnt].push_back(sum);
            }
            return bucket;
        };

        auto L = enumHalf(left);
        auto R = enumHalf(right);

        for (int c = 0; c <= m; c++) {
            auto &v = R[c];
            sort(v.begin(), v.end());
            // 可选：去重减少二分负担
            v.erase(unique(v.begin(), v.end()), v.end());
        }

        long long ans = 0;
        for (int c1 = 0; c1 <= m; c1++) {
            for (long long s1 : L[c1]) {
                int remain = m - c1;
                long long need = k - s1;
                for (int c2 = 0; c2 <= remain; c2++) {
                    const auto &v = R[c2];
                    if (v.empty()) continue;
                    auto it = upper_bound(v.begin(), v.end(), need);
                    if (it == v.begin()) continue;
                    --it;
                    ans = max(ans, s1 + *it);
                }
            }
        }

        cout << ans << "\n";
        return 0;
    }

    // 理论兜底（不在题目给定数据范围的组合内）
    // 这里输出 0，避免未定义行为。
    cout << 0 << "\n";
    return 0;
}