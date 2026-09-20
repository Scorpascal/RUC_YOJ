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

// 思路：答案 = 最大权反链。
// 将列反转后，反链 <-> (i 严格递增, j' 严格递增) 的点集（链）。
// DP：dp[i][j] = w[i][j] + best[i-1][j-1]
// best[i][j] = max(best[i-1][j], best[i][j-1], dp[i][j])
// 用滚动数组实现 O(n*m) 时间、O(m) 内存。

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int T;
    if (!(cin >> T)) return 0;
    while (T--) {
        int n, m;
        cin >> n >> m;

        vector<long long> best_prev(m + 1, 0), best_cur(m + 1, 0);
        vector<long long> row_rev(m + 1, 0); // 1..m，存当前行按“反转列”后的权值

        for (int i = 1; i <= n; ++i) {
            // 读入一行，并按反转列放入 row_rev：原列 j -> 反转列 jr = m-j+1
            for (int j = 1; j <= m; ++j) {
                long long x;
                cin >> x;
                int jr = m - j + 1;
                row_rev[jr] = x;
            }

            best_cur[0] = 0;
            for (int j = 1; j <= m; ++j) {
                long long dp = row_rev[j] + best_prev[j - 1];
                best_cur[j] = max({best_prev[j], best_cur[j - 1], dp});
            }
            swap(best_prev, best_cur);
        }

        cout << best_prev[m] << "\n";
    }
    return 0;
}