#include <stdio.h>
#include <stdint.h>

#define MAXN 35

static uint64_t dp[MAXN][MAXN];
static int rootIdx[MAXN][MAXN];
static uint64_t d[MAXN];
static int n;
// 先序输出计数器：已输出的节点数
static int printed = 0;

uint64_t solve(int l, int r) {
    if (l > r) return 1;         // 空子树分数为 1
    if (l == r) {                // 叶节点分数为自身
        rootIdx[l][r] = l;
        return dp[l][r] = d[l];
    }
    if (dp[l][r] != 0) return dp[l][r];

    uint64_t best = 0;
    int bestRoot = l;
    for (int k = l; k <= r; ++k) {
        uint64_t leftScore  = (k-1 >= l) ? solve(l, k-1) : 1;
        uint64_t rightScore = (k+1 <= r) ? solve(k+1, r) : 1;
        uint64_t score = leftScore * rightScore + d[k];
        if (score > best) {
            best = score;
            bestRoot = k;
        }
    }
    rootIdx[l][r] = bestRoot;
    dp[l][r] = best;
    return best;
}

void preorder(int l, int r) {
    if (l > r) return;
    int k = rootIdx[l][r];
    // 除第一个节点外，其余节点前打印空格
    if (printed++) printf(" ");
    printf("%d", k);
    // 递归左右子树（不再在父层处理空格）
    preorder(l, k-1);
    preorder(k+1, r);
}

int main(void) {
    if (scanf("%d", &n) != 1) return 0;
    for (int i = 1; i <= n; ++i) {
        long long tmp;
        scanf("%lld", &tmp);
        d[i] = (uint64_t)tmp;
    }

    uint64_t ans = solve(1, n);
    printf("%llu\n", (unsigned long long)ans);
    printed = 0;            // 重置计数器
    preorder(1, n);
    printf("\n");
    return 0;
}