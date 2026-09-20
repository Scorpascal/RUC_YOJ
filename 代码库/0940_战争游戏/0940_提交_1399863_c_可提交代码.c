#include <stdio.h>
#include <stdlib.h>

static int cmp_int(const void *a, const void *b) {
    int x = *(const int*)a, y = *(const int*)b;
    return (x > y) - (x < y);
}

int main(void) {
    int k, n, m;
    if (scanf("%d %d %d", &k, &n, &m) != 3) return 0;

    // b[j][i]: 第 j 个对手在第 i 座要塞派遣的士兵
    int **b = (int**)malloc(sizeof(int*) * k);
    for (int j = 0; j < k; ++j) {
        b[j] = (int*)malloc(sizeof(int) * n);
        for (int i = 0; i < n; ++i) scanf("%d", &b[j][i]);
    }

    // dp[w] = 最大收益
    int *dp = (int*)malloc(sizeof(int) * (m + 1));
    for (int w = 0; w <= m; ++w) dp[w] = 0;

    // 逐要塞做分组背包
    for (int i = 0; i < n; ++i) {
        // 收集并排序阈值 t_j = 2*b_{j,i} + 1
        int *t = (int*)malloc(sizeof(int) * k);
        for (int j = 0; j < k; ++j) t[j] = 2 * b[j][i] + 1;
        qsort(t, k, sizeof(int), cmp_int);

        // 构造该要塞的选项（成本，收益）
        // 选项 0：成本 0，收益 0；选项 s：成本 t[s-1]，收益 (i+1)*s
        // 仅保留成本 <= m 的选项
        int *cost = (int*)malloc(sizeof(int) * (k + 1));
        int *gain = (int*)malloc(sizeof(int) * (k + 1));
        int optCnt = 1; // 包含 0 选项
        cost[0] = 0; gain[0] = 0;
        for (int s = 1; s <= k; ++s) {
            int c = t[s - 1];
            if (c <= m) {
                cost[optCnt] = c;
                gain[optCnt] = (i + 1) * s; // 要塞编号为 1..n -> i+1
                ++optCnt;
            } else {
                // 后续阈值更大，直接跳出
                break;
            }
        }

        // 分组背包更新：对该要塞选择一个选项
        // 经典写法：new_dp[w] = max over options of dp[w - cost] + gain
        int *newdp = (int*)malloc(sizeof(int) * (m + 1));
        for (int w = 0; w <= m; ++w) newdp[w] = dp[w]; // 至少可选 0 选项
        for (int w = 0; w <= m; ++w) {
            // 枚举选项
            for (int o = 1; o < optCnt; ++o) {
                int c = cost[o];
                if (w >= c) {
                    int val = dp[w - c] + gain[o];
                    if (val > newdp[w]) newdp[w] = val;
                }
            }
        }
        // 赋值回 dp
        for (int w = 0; w <= m; ++w) dp[w] = newdp[w];

        free(newdp);
        free(cost);
        free(gain);
        free(t);
    }

    // 输出最大值
    int ans = 0;
    for (int w = 0; w <= m; ++w) if (dp[w] > ans) ans = dp[w];
    printf("%d\n", ans);

    // 清理
    for (int j = 0; j < k; ++j) free(b[j]);
    free(b);
    free(dp);
    return 0;
}