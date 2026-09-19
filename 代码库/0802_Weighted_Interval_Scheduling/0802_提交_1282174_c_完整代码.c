#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    int32_t l, r;
    int32_t w;
} Interval;

int cmp_by_r(const void *a, const void *b) {
    const Interval *ia = (const Interval *)a;
    const Interval *ib = (const Interval *)b;
    if (ia->r < ib->r) return -1;
    if (ia->r > ib->r) return 1;
    // 若右端点相同，按左端点升序，保证稳定
    if (ia->l < ib->l) return -1;
    if (ia->l > ib->l) return 1;
    return 0;
}

// 返回已排序r数组中最后一个 <= key 的索引，若不存在返回 -1
int upper_leq(const int32_t *r, int n, int32_t key) {
    int lo = 0, hi = n - 1, ans = -1;
    while (lo <= hi) {
        int mid = lo + ((hi - lo) >> 1);
        if (r[mid] <= key) {
            ans = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return ans;
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    if (n <= 0) {
        printf("0\n");
        return 0;
    }
    Interval *a = (Interval *)malloc(sizeof(Interval) * n);
    if (!a) return 0;

    for (int i = 0; i < n; ++i) {
        int32_t l, r, w;
        if (scanf("%d %d %d", &l, &r, &w) != 3) {
            free(a);
            return 0;
        }
        a[i].l = l;
        a[i].r = r;
        a[i].w = w;
    }

    // 按右端点排序
    qsort(a, n, sizeof(Interval), cmp_by_r);

    // 预备右端点数组用于二分
    int32_t *R = (int32_t *)malloc(sizeof(int32_t) * n);
    if (!R) { free(a); return 0; }
    for (int i = 0; i < n; ++i) R[i] = a[i].r;

    // dp[i]：考虑到第 i 个区间（按排序后的第 i 个，1-based）时的最大权重和
    // 为方便索引，使用 1-based dp，dp[0] = 0
    long long *dp = (long long *)malloc(sizeof(long long) * (n + 1));
    if (!dp) { free(R); free(a); return 0; }
    dp[0] = 0;

    for (int i = 1; i <= n; ++i) {
        // 当前区间为 a[i-1]
        int32_t li = a[i - 1].l;
        int32_t wi = a[i - 1].w;
        // 找到 p：最后一个满足 rj <= li 的索引（0-based），转换到 dp 的 1-based 为 p+1
        int p = upper_leq(R, n, li);
        long long take = wi + (p >= 0 ? dp[p + 1] : 0); // 注意 p+1 对应 dp 索引
        long long skip = dp[i - 1];
        dp[i] = (take > skip) ? take : skip;
    }

    printf("%lld\n", dp[n]);

    free(dp);
    free(R);
    free(a);
    return 0;
}