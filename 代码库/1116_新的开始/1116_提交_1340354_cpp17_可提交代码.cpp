#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if (!(cin >> n)) return 0;

    vector<long long> v(n + 1);
    for (int i = 1; i <= n; i++) cin >> v[i];

    vector<vector<int>> p(n + 1, vector<int>(n + 1, 0));
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= n; j++) cin >> p[i][j];
    }

    const long long INF = (1LL << 60);
    vector<long long> dist(n + 1, INF);   // 0..n
    vector<char> vis(n + 1, 0);

    dist[0] = 0;
    for (int i = 1; i <= n; i++) dist[i] = v[i];

    long long ans = 0;
    for (int iter = 0; iter <= n; iter++) {
        int t = -1;
        for (int i = 0; i <= n; i++) {
            if (!vis[i] && (t == -1 || dist[i] < dist[t])) t = i;
        }

        vis[t] = 1;
        ans += dist[t];

        if (t == 0) continue; // 超级点不需要用矩阵松弛
        for (int j = 1; j <= n; j++) {
            if (!vis[j]) dist[j] = min(dist[j], (long long)p[t][j]);
        }
    }

    cout << ans << "\n";
    return 0;
}