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

static const double EPS = 1e-12;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if (!(cin >> n)) return 0;

    vector<vector<double>> p(n + 1, vector<double>(n));
    for (int i = 0; i <= n; i++) {
        for (int j = 0; j < n; j++) cin >> p[i][j];
    }

    // 构造增广矩阵 A|b，A 为 n*n
    vector<vector<double>> a(n, vector<double>(n + 1, 0.0));

    auto sqnorm = [&](const vector<double>& v) {
        double s = 0.0;
        for (double x : v) s += x * x;
        return s;
    };

    double p0n = sqnorm(p[0]);
    for (int i = 1; i <= n; i++) {
        double pin = sqnorm(p[i]);
        for (int j = 0; j < n; j++) {
            a[i - 1][j] = 2.0 * (p[i][j] - p[0][j]);
        }
        a[i - 1][n] = pin - p0n;
    }

    // 高斯消元（部分主元）
    for (int col = 0; col < n; col++) {
        int sel = col;
        for (int row = col; row < n; row++) {
            if (fabs(a[row][col]) > fabs(a[sel][col])) sel = row;
        }
        if (fabs(a[sel][col]) < EPS) {
            // 题目保证有解，一般不会到这里
            continue;
        }
        if (sel != col) swap(a[sel], a[col]);

        // 归一化当前行
        double piv = a[col][col];
        for (int k = col; k <= n; k++) a[col][k] /= piv;

        // 消元
        for (int row = 0; row < n; row++) {
            if (row == col) continue;
            double factor = a[row][col];
            if (fabs(factor) < EPS) continue;
            for (int k = col; k <= n; k++) {
                a[row][k] -= factor * a[col][k];
            }
        }
    }

    vector<double> c(n, 0.0);
    for (int i = 0; i < n; i++) c[i] = a[i][n];

    cout.setf(std::ios::fixed);
    cout << setprecision(3);
    for (int i = 0; i < n; i++) {
        if (i) cout << ' ';
        // 避免输出 -0.000
        double v = c[i];
        if (fabs(v) < 0.0005) v = 0.0;
        cout << v;
    }
    cout << "\n";
    return 0;
}