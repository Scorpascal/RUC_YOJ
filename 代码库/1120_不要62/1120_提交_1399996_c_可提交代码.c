#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char s[32];
long long memo[32][2][2];
int vis[32][2][2];

long long dfs(int pos, int prev6, int lead, int tight) {
    if (pos == (int)strlen(s)) return 1;
    if (!tight && vis[pos][prev6][lead]) return memo[pos][prev6][lead];
    int limit = tight ? (s[pos] - '0') : 9;
    long long res = 0;
    for (int d = 0; d <= limit; ++d) {
        if (d == 4) continue;            // 禁止含 4
        if (prev6 && d == 2) continue;  // 禁止 62 连号
        int nlead = lead && (d == 0);
        int nprev6 = (!nlead && d == 6) ? 1 : 0;
        res += dfs(pos + 1, nprev6, nlead, tight && (d == limit));
    }
    if (!tight) {
        vis[pos][prev6][lead] = 1;
        memo[pos][prev6][lead] = res;
    }
    return res;
}

long long count_good(long long x) {
    if (x < 0) return 0;
    sprintf(s, "%lld", x);
    memset(vis, 0, sizeof(vis));
    return dfs(0, 0, 1, 1);
}

int main() {
    long long n, m;
    while (scanf("%lld %lld", &n, &m) == 2) {
        if (n == 0 && m == 0) break;
        if (n > m) { long long t = n; n = m; m = t; }
        long long ans = count_good(m) - count_good(n - 1);
        printf("%lld\n", ans);
    }
    return 0;
}