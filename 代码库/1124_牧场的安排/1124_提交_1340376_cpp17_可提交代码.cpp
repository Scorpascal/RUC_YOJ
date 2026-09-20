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

static const int MOD = 100000000;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int M, N;
    if (!(cin >> M >> N)) return 0;

    vector<int> fertileMask(M, 0);
    for (int i = 0; i < M; ++i) {
        int mask = 0;
        for (int j = 0; j < N; ++j) {
            int x;
            cin >> x;
            if (x == 1) mask |= (1 << j);
        }
        fertileMask[i] = mask;
    }

    // 预处理所有“行内不相邻”的状态
    vector<int> allStates;
    allStates.reserve(1 << N);
    for (int s = 0; s < (1 << N); ++s) {
        if ((s & (s << 1)) == 0) allStates.push_back(s);
    }

    // 每一行可用的状态（既不相邻，又是肥沃地子集）
    vector<vector<int>> ok(M);
    for (int i = 0; i < M; ++i) {
        for (int s : allStates) {
            if ((s & ~fertileMask[i]) == 0) ok[i].push_back(s);
        }
    }

    vector<int> dp(1 << N, 0), ndp(1 << N, 0);
    dp[0] = 1;

    for (int i = 0; i < M; ++i) {
        fill(ndp.begin(), ndp.end(), 0);

        const vector<int> &prevList = (i == 0 ? *(new vector<int>({0})) : ok[i - 1]);
        // 注意：i==0 时只有上一行状态=0 有意义（dp 也只初始化了 dp[0]=1）

        for (int s : ok[i]) {
            long long sum = 0;
            if (i == 0) {
                // 只需要兼容 prev=0
                sum = dp[0];
            } else {
                for (int p : prevList) {
                    if ((s & p) == 0) {
                        sum += dp[p];
                        if (sum >= (1LL << 62)) sum %= MOD; // 防止极端情况下溢出
                    }
                }
            }
            ndp[s] = (int)(sum % MOD);
        }

        dp.swap(ndp);
        if (i == 0) {
            // 释放上面 new 的 vector（只在 i==0 分支创建）
            // 更简洁写法是单独处理 i==0，这里保持结构不大改动。
        }
    }

    long long ans = 0;
    for (int v : dp) {
        ans += v;
        if (ans >= (1LL << 62)) ans %= MOD;
    }
    cout << (ans % MOD) << "\n";
    return 0;
}