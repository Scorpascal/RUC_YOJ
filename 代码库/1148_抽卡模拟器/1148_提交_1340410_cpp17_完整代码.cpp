#include <bits/stdc++.h>
using namespace std;

// 计算将 n 分拆成恰好 k 个正整数（不计顺序）的方案数
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, k;
    cin >> n >> k;

    vector<vector<unsigned long long>> dp(n + 1, vector<unsigned long long>(k + 1, 0));
    dp[0][0] = 1;

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= k; ++j) {
            // dp[i][j] = dp[i-1][j-1] + dp[i-j][j] (若 i >= j)
            dp[i][j] += dp[i - 1][j - 1];
            if (i >= j) dp[i][j] += dp[i - j][j];
        }
    }

    cout << dp[n][k] << "\n";
    return 0;
}