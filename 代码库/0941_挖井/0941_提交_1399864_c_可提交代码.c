#include <stdio.h>
#include <string.h>

#define INF 0x3f3f3f3f
#define MAXN 505

long long pos[MAXN];      // 村庄坐标（从0开始累计距离）
long long pre[MAXN];      // 坐标前缀和
long long cost1[MAXN][MAXN]; // cost1[i][j]: 用一个井覆盖 [i..j] 的最小总代价
long long dp[MAXN][MAXN]; // dp[k][j]: 前 j 个村庄放 k 个井的最小总代价

// 计算区间 [l..r] 用一个井（设在中位数村庄）覆盖的代价
static inline long long one_cost(int l, int r) {
    int mid = (l + r) >> 1;
    // 左半段代价: mid * (mid - l + 1) - sum(pos[l..mid])
    long long leftCnt = mid - l + 1;
    long long leftSum = pre[mid] - pre[l - 1];
    long long leftCost = pos[mid] * leftCnt - leftSum;
    // 右半段代价: sum(pos[mid+1..r]) - mid * (r - mid)
    long long rightCnt = r - mid;
    long long rightSum = pre[r] - pre[mid];
    long long rightCost = rightSum - pos[mid] * rightCnt;
    return leftCost + rightCost;
}

int main(void) {
    int n, m;
    if (scanf("%d %d", &n, &m) != 2) return 0;
    // 读相邻距离，构造坐标
    pos[1] = 0;
    for (int i = 2; i <= n; ++i) {
        int d;
        scanf("%d", &d);
        pos[i] = pos[i - 1] + d;
    }
    // 前缀和
    pre[0] = 0;
    for (int i = 1; i <= n; ++i) pre[i] = pre[i - 1] + pos[i];

    // 预计算单井代价
    for (int i = 1; i <= n; ++i) {
        for (int j = i; j <= n; ++j) {
            cost1[i][j] = one_cost(i, j);
        }
    }

    // 初始化 DP
    for (int k = 0; k <= m; ++k)
        for (int j = 0; j <= n; ++j)
            dp[k][j] = (k == 0 && j == 0) ? 0 : (long long)INF * INF; // 大INF

    // k=1 的情况
    for (int j = 1; j <= n; ++j) dp[1][j] = cost1[1][j];

    // 一般情况
    for (int k = 2; k <= m; ++k) {
        for (int j = k; j <= n; ++j) { // 至少每段一个村庄
            long long best = (long long)INF * INF;
            // 最后一段为 [i..j]
            for (int i = k; i <= j; ++i) {
                long long cand = dp[k - 1][i - 1] + cost1[i][j];
                if (cand < best) best = cand;
            }
            dp[k][j] = best;
        }
    }

    printf("%lld\n", dp[m][n]);
    return 0;
}