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

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, k;
    cin >> n >> k;

    string s;
    cin >> s;

    int N = max(n, k) + 5;

    // a[i]：有多少个 '+' 的右侧有 i 个 '*'
    vector<int> a(N, 0);

    // ans[i]：答案二进制的第 i 位
    vector<int> ans(k, 0);

    // -------------------------
    // 1. 从右往左统计
    // -------------------------
    int starCnt = 0;

    for (int i = n - 1; i >= 0; --i) {
        if (s[i] == '*') {
            ++starCnt;
        } else {
            ++a[starCnt];
        }
    }

    // -------------------------
    // 2. 二进制分组 / 进位
    //
    // 每一层最终只保留 0 / 1 / 2 个 '+'
    // -------------------------
    for (int i = 0; i <= n; ++i) {
        if (a[i] == 0)
            continue;

        int carry = (a[i] - 1) / 2;

        a[i] -= carry * 2;
        a[i + 1] += carry;
    }

    // -------------------------
    // 3. 从最高位开始贪心
    // -------------------------
    int p = k - 1;

    // 最大可能层数不会超过 n
    int i = n;

    while (p >= 0) {

        // 找当前最高的非空层
        while (i >= 0 && a[i] == 0)
            --i;

        if (i < 0)
            break;

        // 有足够多的 '*'，
        // 当前第 p 位一定可以置成 1
        if (i >= p) {
            ans[p] = 1;

            --p;
            --i;
        }

        // 最大层数已经 < p
        // 无法继续直接制造第 p 位
        else {

            // 剩余部分直接做普通二进制进位
            for (int j = 0; j < p; ++j) {

                int carry = a[j] / 2;

                a[j] %= 2;
                a[j + 1] += carry;

                ans[j] = a[j];
            }

            ans[p] = a[p];

            break;
        }
    }

    // -------------------------
    // 4. 输出，去掉前导零
    // -------------------------
    int highest = k - 1;

    while (highest >= 0 && ans[highest] == 0)
        --highest;

    if (highest < 0) {
        cout << 0;
    } else {
        for (int i = highest; i >= 0; --i)
            cout << ans[i];
    }

    cout << '\n';

    return 0;
}