#include <stdio.h>
#include <string.h>

#define INF 0x3f3f3f3f
#define MAXN 205

// 计算区间和：pre[r] - pre[l-1]
static inline int range_sum(const int *pre, int l, int r) {
    return pre[r] - pre[l - 1];
}

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    int a[2*MAXN] = {0};
    for (int i = 1; i <= n; ++i) {
        scanf("%d", &a[i]);
        a[i + n] = a[i]; // 展开为 2n
    }

    int m = 2 * n;
    // 前缀和
    int pre[2*MAXN] = {0};
    for (int i = 1; i <= m; ++i) pre[i] = pre[i-1] + a[i];

    // DP 数组
    int dpMin[2*MAXN][2*MAXN];
    int dpMax[2*MAXN][2*MAXN];
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= m; ++j) {
            dpMin[i][j] = (i == j) ? 0 : INF;
            dpMax[i][j] = (i == j) ? 0 : 0;
        }
    }

    // 区间 DP
    for (int len = 2; len <= n; ++len) {
        for (int i = 1; i + len - 1 <= m; ++i) {
            int j = i + len - 1;
            int s = range_sum(pre, i, j);
            int mn = INF, mx = 0;
            for (int k = i; k < j; ++k) {
                if (dpMin[i][k] + dpMin[k+1][j] + s < mn)
                    mn = dpMin[i][k] + dpMin[k+1][j] + s;
                if (dpMax[i][k] + dpMax[k+1][j] + s > mx)
                    mx = dpMax[i][k] + dpMax[k+1][j] + s;
            }
            dpMin[i][j] = mn;
            dpMax[i][j] = mx;
        }
    }

    int ansMin = INF, ansMax = 0;
    for (int i = 1; i <= n; ++i) {
        if (dpMin[i][i+n-1] < ansMin) ansMin = dpMin[i][i+n-1];
        if (dpMax[i][i+n-1] > ansMax) ansMax = dpMax[i][i+n-1];
    }

    printf("%d\n%d\n", ansMin, ansMax);
    return 0;
}