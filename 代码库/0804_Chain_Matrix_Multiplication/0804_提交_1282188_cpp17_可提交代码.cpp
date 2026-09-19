// Hu–Shing algorithm for Matrix Chain Multiplication (O(n log n))
#include <bits/stdc++.h>
using namespace std;
using i64 = long long;
using i128 = __int128_t;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if (!(cin >> n)) return 0;
    if (n <= 0) { cout << 0 << "\n"; return 0; }

    // 构造维度数组 p[0..n]，Ai: p[i-1] x p[i]
    vector<i64> p(n + 1);
    i64 l, r;
    for (int i = 0; i < n; ++i) {
        cin >> l >> r;
        if (i == 0) p[0] = l;
        p[i + 1] = r;
    }

    if (n == 1) { cout << 0 << "\n"; return 0; }
    if (n == 2) {
        i128 ans = (i128)p[0] * p[1] * p[2];
        cout << (long long)ans << "\n";
        return 0;
    }

    // dp[i][j]: 计算 Ai..Aj 的最小乘法次数（1-based）
    vector<vector<long long>> dp(n + 1, vector<long long>(n + 1, 0));

    for (int len = 2; len <= n; ++len) {
        for (int i = 1; i + len - 1 <= n; ++i) {
            int j = i + len - 1;
            long long best = LLONG_MAX;
            for (int k = i; k < j; ++k) {
                i128 cost = (i128)dp[i][k] + dp[k + 1][j] + (i128)p[i - 1] * p[k] * p[j];
                if (cost < best) best = (long long)cost;
            }
            dp[i][j] = best;
        }
    }

    cout << dp[1][n] << "\n";
    return 0;
}