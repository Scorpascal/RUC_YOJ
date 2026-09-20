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

// 矩形匈牙利：n 行选 m 列（n<=m），最小化总成本
// 这里成本是 0/1：cost(i,j) = (A[i][j] > T)
static int hungarian01(const vector<vector<long long>>& A, int n, int m, long long T) {
    const int INF = 1e9;

    vector<int> u(n + 1, 0), v(m + 1, 0), p(m + 1, 0), way(m + 1, 0);
    vector<int> minv(m + 1);
    vector<char> used(m + 1);

    for (int i = 1; i <= n; i++) {
        p[0] = i;
        int j0 = 0;
        fill(minv.begin(), minv.end(), INF);
        fill(used.begin(), used.end(), 0);

        do {
            used[j0] = 1;
            int i0 = p[j0];
            int delta = INF, j1 = 0;

            for (int j = 1; j <= m; j++) if (!used[j]) {
                int cij = (A[i0 - 1][j - 1] > T) ? 1 : 0;
                int cur = cij - u[i0] - v[j];
                if (cur < minv[j]) {
                    minv[j] = cur;
                    way[j] = j0;
                }
                if (minv[j] < delta) {
                    delta = minv[j];
                    j1 = j;
                }
            }

            for (int j = 0; j <= m; j++) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);

        // 增广
        do {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    // 从匹配结果计算真实 0/1 代价（坏边数）
    vector<int> matchRow(n + 1, 0);
    for (int j = 1; j <= m; j++) {
        if (p[j]) matchRow[p[j]] = j;
    }

    int bad = 0;
    for (int i = 1; i <= n; i++) {
        int j = matchRow[i];
        bad += (A[i - 1][j - 1] > T) ? 1 : 0;
    }
    return bad;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M, K;
    cin >> N >> M >> K;

    vector<vector<long long>> A(N, vector<long long>(M));
    vector<long long> vals;
    vals.reserve(1LL * N * M);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            cin >> A[i][j];
            vals.push_back(A[i][j]);
        }
    }

    sort(vals.begin(), vals.end());
    vals.erase(unique(vals.begin(), vals.end()), vals.end());

    auto ok = [&](long long T) -> bool {
        int bad = hungarian01(A, N, M, T);
        return bad <= K - 1;
    };

    int l = 0, r = (int)vals.size() - 1;
    while (l < r) {
        int mid = (l + r) >> 1;
        if (ok(vals[mid])) r = mid;
        else l = mid + 1;
    }

    cout << vals[l] << "\n";
    return 0;
}