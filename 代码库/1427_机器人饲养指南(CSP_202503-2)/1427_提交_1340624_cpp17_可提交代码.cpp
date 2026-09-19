#include <bits/stdc++.h>
using namespace std;

// 完全背包：容量 n（苹果总数），物品“每天喂 i 个苹果”，价值 A[i]，可无限次使用
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;
    vector<long long> A(m + 1, 0);
    for (int i = 1; i <= m; i++) cin >> A[i];

    const long long NEG = -(1LL << 60);
    vector<long long> dp(n + 1, NEG);
    dp[0] = 0;

    for (int x = 1; x <= n; x++) {
        for (int i = 1; i <= m && i <= x; i++) {
            if (dp[x - i] != NEG) {
                dp[x] = max(dp[x], dp[x - i] + A[i]);
            }
        }
    }

    cout << dp[n] << "\n";
    return 0;
}