#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef long long ll;

int main() {
    int N;
    ll M;
    if (scanf("%d%lld", &N, &M) != 2) return 0;
    int K[128];
    ll C[128];
    for (int i = 0; i < N; ++i) scanf("%d", &K[i]);
    for (int i = 0; i < N; ++i) scanf("%lld", &C[i]);

    if (M <= 1) { puts("0"); return 0; }

    // 计算最大可能花费
    int maxCost = 0;
    for (int i = 0; i < N; ++i) {
        // K[i] ≤ 10, C[i] ≤ 199（题目给定）
        maxCost += K[i] * (int)C[i];
    }

    ll *dp  = (ll*)malloc((size_t)(maxCost + 1) * sizeof(ll));
    ll *ndp = (ll*)malloc((size_t)(maxCost + 1) * sizeof(ll));
    if (!dp || !ndp) {
        fprintf(stderr, "memory alloc fail\n");
        return 0;
    }
    for (int c = 0; c <= maxCost; ++c) dp[c] = 0;
    dp[0] = 1;

    int maxReach = 0; // 当前 dp 中非 0 状态的最大花费（初始 0）

    for (int i = 0; i < N; ++i) {
        // 计算本轮花费增量最大可能，用于更新 maxReach
        int heroMaxAdd = K[i] * (int)C[i];
        int newMaxReach = maxReach + heroMaxAdd;
        if (newMaxReach > maxCost) newMaxReach = maxCost;

        // 复制原状态（选择 j=0）
        size_t bytes = (size_t)(newMaxReach + 1) * sizeof(ll);
        memset(ndp, 0, bytes);
        memcpy(ndp, dp, (size_t)(maxReach + 1) * sizeof(ll));

        // 遍历该英雄可选 j（跳过 1）
        for (int j = 2; j <= K[i]; ++j) {
            int addCost = j * (int)C[i];
            if (addCost > newMaxReach) break; // 花费超过范围
            // c 只需遍历到 maxReach（上一轮最大达成）且 c+addCost ≤ newMaxReach
            int up = maxReach;
            if (up > newMaxReach - addCost) up = newMaxReach - addCost;
            for (int c = 0; c <= up; ++c) {
                ll base = dp[c];
                if (!base) continue;
                ll prod;
                if (base > M / j) prod = M;
                else {
                    prod = base * j;
                    if (prod > M) prod = M;
                }
                ll *slot = &ndp[c + addCost];
                if (prod > *slot) *slot = prod;
            }
        }

        // 交换
        ll *tmp = dp; dp = ndp; ndp = tmp;
        maxReach = newMaxReach;
    }

    // 寻找最小花费
    for (int c = 0; c <= maxReach; ++c) {
        if (dp[c] >= M) {
            printf("%d\n", c);
            free(dp); free(ndp);
            return 0;
        }
    }

    // 题目保证有解
    puts("-1");
    free(dp); free(ndp);
    return 0;
}