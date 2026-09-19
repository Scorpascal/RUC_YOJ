#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m, hx, hy;
    cin >> n >> m >> hx >> hy;

    auto inside = [&](int x, int y) {
        return 0 <= x && x <= n && 0 <= y && y <= m;
    };

    vector<vector<bool>> blocked(n + 1, vector<bool>(m + 1, false));
    const int dx[8] = {1, 2, 2, 1, -1, -2, -2, -1};
    const int dy[8] = {2, 1, -1, -2, -2, -1, 1, 2};

    for (int k = 0; k < 8; k++) {
        int x = hx + dx[k], y = hy + dy[k];
        if (inside(x, y)) blocked[x][y] = true;
    }

    vector<vector<long long>> dp0(n + 1, vector<long long>(m + 1, 0));
    vector<vector<long long>> dp1(n + 1, vector<long long>(m + 1, 0));

    // 起点如果被马控制，则起点非法，答案为 0
    dp0[0][0] = blocked[0][0] ? 0LL : 1LL;

    for (int x = 0; x <= n; x++) {
        for (int y = 0; y <= m; y++) {
            if (x == 0 && y == 0) continue;

            long long from0 = 0, from1 = 0;
            if (x > 0) { from0 += dp0[x - 1][y]; from1 += dp1[x - 1][y]; }
            if (y > 0) { from0 += dp0[x][y - 1]; from1 += dp1[x][y - 1]; }

            if (x == hx && y == hy) {
                dp0[x][y] = 0;
                dp1[x][y] = from0; // 走到马格并吃马
            } else {
                dp0[x][y] = blocked[x][y] ? 0 : from0; // 未吃马：不能进控制点
                dp1[x][y] = from1;                    // 已吃马：不受限制
            }
        }
    }

    cout << (dp0[n][m] + dp1[n][m]) << "\n";
    return 0;
}